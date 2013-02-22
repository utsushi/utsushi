//  compound-scanner.cpp -- devices that handle compound commands
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This package is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License or, at
//  your option, any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//  You ought to have received a copy of the GNU General Public License
//  along with this package.  If not, see <http://www.gnu.org/licenses/>.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>

#include "compound-scanner.hpp"

#define for_each BOOST_FOREACH

namespace utsushi {
namespace _drv_ {
namespace esci {

using std::domain_error;
using std::logic_error;
using std::runtime_error;

static std::string create_message (const quad& part, const quad& what);

// Disable the restriction checking for now to work around limitations
// in the option::map support for this.
#define ENABLE_RESTRICTIONS 0

//! \todo Don't use non-standard map::at() accessor
static bool
duplex_needs_adf (const value::map& vm)
{
  if (!vm.count ("duplex")) return true;

  toggle t = vm.at ("duplex");
  return !t || value ("ADF") == vm.at ("doc-source");
}

//! \todo Merge with duplex_needs_adf in a function object
static bool
double_feed_needs_adf (const value::map& vm)
{
  if (!vm.count ("double-feed-detection")) return true;

  const value& v (vm.at ("double-feed-detection"));

  return (value (toggle ()) == v)
    ||   (value ("Off") == v)
    || value ("ADF") == vm.at ("doc-source");
}

//! Convert a \a color_matrix into a protocol compliant vector
static boost::optional< std::vector< byte > >
vectorize (const quad& token, const matrix< double, 3 >& color_matrix)
{
  using namespace code_token::parameter;
  using std::swap;
  using std::min;
  using std::abs;

  int norm = 0;
  if (cmx::UM08 == token) norm =   32;
  if (cmx::UM16 == token) norm = 8192;

  if (0 == norm)
    {
      log::error ("unsupported color matrix conversion: %1%")
        % str (token);
      return boost::none;
    }

  matrix< double, 3 > mat (color_matrix);

  mat *= norm;

  // FIXME tweak mat so each row sum equals norm

  // adjust from RGB row based order to GRB column based
  swap (mat[0][0], mat[1][1]);
  swap (mat[0][2], mat[2][1]);
  swap (mat[1][2], mat[2][0]);

  std::vector< byte > result;
  if (cmx::UM08 == token)
    {
      result.reserve (mat.size ());

      for (size_t i = 0; i < mat.rows (); ++i)
        {
          for (size_t j = 0; j < mat.cols (); ++j)
            {
              int8_t magnitude = min (abs (mat[i][j]), double (0x7f));
              int8_t sign      = 0 > mat[i][j] ? 0x80 : 0x00;

              result.push_back (sign | magnitude);
            }
        }
    }
  if (cmx::UM16 == token)
    {
      result.reserve (2 * mat.size ());

      for (size_t i = 0; i < mat.rows (); ++i)
        {
          for (size_t j = 0; j < mat.cols (); ++j)
            {
              int16_t magnitude = min (abs (mat[i][j]), double (0x7fff));
              int8_t  sign      = 0 > mat[i][j] ? 0x80 : 0x00;

              result.push_back (sign | int8_t (0x7f & (magnitude >> 8)));
              result.push_back (       int8_t (0xff &  magnitude));
            }
        }
    }

  return result;
}

compound_scanner::compound_scanner (const connexion::ptr& cnx)
  : scanner (cnx)
  , info_()                     // initialize reference data
  , caps_() , caps_flip_()
  , defs_() , defs_flip_()
  , min_width_(0.05)
  , min_height_(0.05)
  , read_back_(true)
  , streaming_flip_side_image_(false)
  , image_count_(0)
  , cancelled_(false)
{
  scanner_control cmd;          // get *default* parameter settings

  *cnx_ << cmd.get (const_cast< information&  > (info_));
  *cnx_ << cmd.get (const_cast< capabilities& > (caps_));
  *cnx_ << cmd.get (const_cast< capabilities& > (caps_flip_), true);
  *cnx_ << cmd.get (const_cast< parameters&   > (defs_));
  *cnx_ << cmd.get (const_cast< parameters&   > (defs_flip_), true);

  // Initialize private protocol extension bits
  // These capabilities don't make sense for the flip-side only so
  // there's no need to set them for caps_flip_.

  if (!caps_.bsz)
    {
      capabilities& caps (const_cast< capabilities& > (caps_));

      caps.bsz = capabilities::range (1, esci_hex_max);
    }
  if (!caps_.pag)
    {
      capabilities& caps (const_cast< capabilities& > (caps_));

      caps.pag = capabilities::range (esci_dec_min, esci_dec_max);
    }
}

void
compound_scanner::configure ()
{
  configure_flatbed_options ();         // order dependency
  configure_adf_options ();
  configure_tpu_options ();

  {
    constraint::ptr cp (caps_.document_sources (defs_.source ()));

    if (cp)
      {
        add_options ()
          ("doc-source", cp,
           attributes (tag::general)(level::standard),
           N_("Document Source")
           )
          ;
      }
    insert (doc_source_options (defs_.source ()));
  }
  {
    constraint::ptr cp (caps_.formats (defs_.fmt));

    if (cp)
      {
        add_options ()
          ("transfer-format", cp,
           attributes (level::standard),
           N_("Transfer Format")
           )
          ;
      }
  }
  {
    constraint::ptr cp (caps_.jpeg_quality (defs_.jpg));

    if (cp)
      {
        add_options ()
          ("jpeg-quality", cp,
           attributes (),
           N_("JPEG Quality")
           )
          ;
      }
  }
  {
    constraint::ptr cp (caps_.gamma (defs_.gmm));

    if (cp)
      {
        add_options ()
          ("gamma", cp,
           attributes (),
           N_("Gamma")
           )
          ;
      }
    // FIXME Should we check the gmt vector content?  I would expect
    //       to need MONO or a {RED,GRN,BLU} triplet.  What if
    //       e.g. only RED is present?  Too unlikely to contemplate?
    // TODO Add a test for this to tests/compound-protocol.cpp so we
    //      will notice when a device is "weird".
    if (caps_.gmt && !caps_.gmt->empty ())
      {
        add_options ()
          ("brightness", (from< range > ()
                          -> lower (-1.0)
                          -> upper ( 1.0)
                          -> default_value (0.0)),
           attributes (tag::enhancement)(level::standard),
           N_("Brightness")
           )
          ("contrast", (from< range > ()
                        -> lower (-1.0)
                        -> upper ( 1.0)
                        -> default_value (0.0)),
           attributes (tag::enhancement)(level::standard),
           N_("Contrast")
           )
          ;
      }
  }
  {
    constraint::ptr cp (caps_.image_types (defs_.col));

    if (cp)
      {
        add_options ()
          ("image-type", cp,
           attributes (tag::general)(level::standard),
           N_("Image Type")
           )
          ;
      }
  }
  {
    constraint::ptr cp (caps_.dropouts ());

    if (cp)
      {
        add_options ()
          ("dropout", cp,
           attributes (tag::enhancement)(level::standard),
           N_("Dropout")
           )
          ;
      }
  }
  {
    constraint::ptr cp (caps_.threshold (defs_.thr));

    if (cp)
      {
        add_options ()
          ("threshold", cp,
           attributes (tag::enhancement)(level::standard),
           N_("Threshold")
           )
          ;
      }
  }
  if (caps_.adf)
    {
      constraint::ptr cp_f (caps_.border_fill ());
      constraint::ptr cp_s (caps_.border_size ());

      if (cp_f && cp_s)
        {
          adf_.add_options ()
            ("border-fill", cp_f,
             attributes (),
             N_("Border Fill")
             );

          // Create *separate* constraints, one for each border, so that
          // we can set independent defaults.  Note that passing return
          // values of default_value() called on an already constructed
          // constraint::ptr is a recipe for disaster.  It leads to two
          // constraint::ptr objects that, completely unaware of one and
          // other, attempt to manage the same underlying object.

          quantity default_border = cp_s->default_value ();

          adf_.add_options ()
            ("border-left", (caps_.border_size
                             (defs_.border_left (default_border))),
             attributes (),
             N_("Left Border")
             )
            ("border-right", (caps_.border_size
                              (defs_.border_right (default_border))),
             attributes (),
             N_("Right Border")
             )
            ("border-top", (caps_.border_size
                            (defs_.border_top (default_border))),
             attributes (),
             N_("Top Border")
             )
            ("border-bottom", (caps_.border_size
                               (defs_.border_bottom (default_border))),
             attributes (),
             N_("Bottom Border")
             )
            ;
        }
    }
  //! \todo Change units from bytes to kilobytes
  {
    constraint::ptr cp (caps_.buffer_size (defs_.bsz));

    if (cp)
      {
        add_options ()
          ("transfer-size", cp,
           attributes (),
           N_("Transfer Size")
           )
          ;
      }
  }
  /*! \todo Remove this ugly hack.  It is only here to allow scan-cli
   *        to process all the options that might possibly be given on
   *        the command-line.  Its option parser only does a single
   *        pass on the options and chokes if there's anything that
   *        wasn't recognized (e.g. a `--duplex` option when given in
   *        combination with `--doc-source=ADF`).  At least, with the
   *        hack below all options are added to the CLI option parser.
   *        The content of the first added map takes precedence and
   *        later maps only add what is not there yet.
   */
  if (caps_.fb ) insert (flatbed_);
  if (caps_.adf) insert (adf_);
  if (caps_.tpu) insert (tpu_);

  if (!validate (values ()))
    {
      BOOST_THROW_EXCEPTION
        (logic_error
         (_("esci::compound_scanner(): internal inconsistency")));
    }
  finalize (values ());
}

bool
compound_scanner::is_single_image () const
{
  return value ("ADF") != *values_["doc-source"];
}

bool
compound_scanner::is_consecutive () const
{
  return (parm_.adf || parm_flip_.adf);
}

bool
compound_scanner::obtain_media ()
{
  buffer_.clear ();
  offset_ = 0;

  if (acquire_.is_duplexing ())
    streaming_flip_side_image_ = (1 == image_count_ % 2);

  fill_buffer_();

  return !acquire_.media_out ();
}

bool
compound_scanner::set_up_image ()
{
  using namespace code_token::parameter;

  if (cancelled_) return false;

  ctx_ = context (pixel_width (), pixel_height (), pixel_type ());
  ctx_.resolution (*parm_.rsm, *parm_.rss);

  if (fmt::JPG == parm_.fmt)
    ctx_.media_type ("image/jpeg");

  if (buffer_.pst)
    {
      ctx_.width (buffer_.pst->width, buffer_.pst->padding);
      ctx_.height (buffer_.pst->height);
    }
  else
    {
      log::alert ("device reported neither image dimensions nor padding"
                  ", relying on driver computations instead");
    }

  return true;
}

void
compound_scanner::finish_image ()
{
  ++image_count_;
}

streamsize
compound_scanner::sgetn (octet *data, streamsize n)
{
  data_buffer::size_type sz (n);

  if (offset_ == buffer_.size ())
    {
      fill_buffer_();
      if (cancelled_) return traits::eof ();
    }

  streamsize rv = std::min (buffer_.size () - offset_, sz);

  traits::copy (data, reinterpret_cast< const octet * >
                (buffer_.data () + offset_), rv);
  offset_ += rv;

  return rv;
}

void
compound_scanner::cancel_()
{
  acquire_.cancel ();
}

void
compound_scanner::set_up_initialize ()
{
  parm_      = defs_;
  parm_flip_ = defs_flip_;

  streaming_flip_side_image_ = false;
  face_.clear ();
  rear_.clear ();

  image_count_ = 0;
  cancelled_ = false;
}

bool
compound_scanner::set_up_hardware ()
{
  *cnx_ << acquire_.set (parm_);
  if (read_back_)
    {
      parameters requested (parm_);
      *cnx_ << acquire_.get (parm_);
      if (requested != parm_)
        log::alert ("scan parameters not set as requested");
    }

  if (caps_flip_)
    {
      *cnx_ << acquire_.set (parm_flip_, true);
      if (read_back_)
        {
          parameters requested (parm_flip_);
          *cnx_ << acquire_.get (parm_flip_, true);
          if (requested != parm_flip_)
            log::alert ("flip side scan parameters not set as requested");
        }
    }
  else
    {
      parm_flip_ = parm_;
    }

  *cnx_ << acquire_.get (stat_);

  if (stat_.error)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error
         (create_message (stat_.error->part_, stat_.error->what_)));
    }

  *cnx_ << acquire_.start ();

  return true;
}

void
compound_scanner::set_up_color_matrices ()
{
  if (!caps_.cmx) return;

  using namespace code_token::parameter;

  if (val_.count ("color-correction")
      && (value (toggle (false)) == val_["color-correction"]))
    {
      parm_.cmx->type = cmx::UNIT;
      log::debug ("disabling color correction");
    }
  else
    {
      quad cmx (cmx::UNIT);

      if (col::C024 == parm_.col) cmx = cmx::UM08;
      if (col::C048 == parm_.col) cmx = cmx::UM16;

      if (cmx::UNIT != cmx)
        {
          parm_.cmx->type   = cmx;
          parm_.cmx->matrix = vectorize (cmx, profile_matrix_);
        }

      if (!parm_.cmx->matrix)
        {
          log::brief ("falling back to unit profile");
          parm_.cmx->type = cmx::UNIT;
        }
      log::debug ("using %1% profile") % str (parm_.cmx->type);
    }
}

void
compound_scanner::set_up_dithering ()
{
}

/*! \note The crop, deskew and overscan option adder functions already
 *        make sure, at compile-time, that the relevant code tokens
 *        are identical for all document sources.  The implementation
 *        simply uses those for a flatbed document source.
 */
void
compound_scanner::set_up_doc_source ()
{
  using namespace code_token::parameter;

  std::vector< quad > src_opts;

  if (val_.count ("crop")
      && (value (toggle (true)) == val_["crop"]))
    {
      src_opts.push_back (fb::CRP);
    }
  if (val_.count ("deskew")
      && (value (toggle (true)) == val_["deskew"]))
    {
      src_opts.push_back (fb::SKEW);
    }
  if (val_.count ("overscan")
      && (value (toggle (true)) == val_["overscan"]))
    {
      src_opts.push_back (fb::OVSN);
    }
  if (val_.count ("duplex")
      && (value (toggle (true)) == val_["duplex"]))
    {
      src_opts.push_back (adf::DPLX);
    }
  if (val_.count ("double-feed-detection"))
    {
      const value& v (val_["double-feed-detection"]);

      if (v == value (toggle ()) || v == value ("Off"))
        {
          // nothing to be done
        }
      else if (v == value (toggle (true)) || v == value ("Normal"))
        {
          src_opts.push_back (adf::DFL1);
        }
      else if (v == value ("Sensitive"))
        {
          src_opts.push_back (adf::DFL2);
        }
      else
        log::error
          ("double-feed:detection: unsupported value '%1%'")
          % v
          ;
    }
  if (val_.count ("alternative"))
    {
      toggle t = val_["alternative"];
      src_opts.push_back (!t ? tpu::ARE1 : tpu::ARE2);
    }

  parm_.adf = boost::none;
  parm_.tpu = boost::none;
  parm_.fb  = boost::none;

  if (val_.count ("doc-source"))
    {
      string src = val_["doc-source"];

      if (src == "Flatbed")  parm_.fb  = src_opts;
      else if (src == "ADF") parm_.adf = src_opts;
      else if (src == "TPU") parm_.tpu = src_opts;
    }
  else                          // only one document source
    {
      if (caps_.fb ) parm_.fb  = src_opts;
      if (caps_.adf) parm_.adf = src_opts;
      if (caps_.tpu) parm_.tpu = src_opts;
    }
}

void
compound_scanner::set_up_gamma_tables ()
{
  using namespace code_token::parameter;

  if (val_.count ("gamma"))
    {
      string gamma = val_["gamma"];

      /**/ if (gamma == "1.0") parm_.gmm = gmm::UG10;
      else if (gamma == "1.8") parm_.gmm = gmm::UG18;
      else if (gamma == "2.2") parm_.gmm = gmm::UG22;
      else
        log::error
          ("unknown user gamma value: %1%, using default")
          % gamma
          ;
    }

  if (caps_.gmt)
    {
      quantity brightness;
      if (val_.count ("brightness")) brightness = val_["brightness"];

      quantity contrast;
      if (val_.count ("contrast")) contrast = val_["contrast"];

      std::vector< byte > table (256);
      quantity cap (table.size () - 1.0);

      brightness *= cap / 2;
      contrast   *= cap / 2;

      for (std::vector< byte >::size_type i = 0; i < table.size (); ++i)
        {
          quantity val = ((cap * (quantity::non_integer_type (i) - contrast))
                          / (cap - 2 * contrast)) + brightness;

          val = std::min (cap, std::max (val, quantity ()));
          table[i] = val.amount< quantity::non_integer_type > ();
        }

      parm_.gmt = std::vector< parameters::gamma_table > ();
      BOOST_FOREACH (quad token, *caps_.gmt)
        {
          parameters::gamma_table gmt;
          gmt.component = token;
          gmt.table     = table;
          parm_.gmt->push_back (gmt);
        }
    }
}

void
compound_scanner::set_up_image_mode ()
{
  using namespace code_token::parameter;

  if (val_.count ("image-type"))
    {
      string type = val_["image-type"];

      /**/ if (type == "Color (1 bit)" ) parm_.col = col::C003;
      else if (type == "Color (8 bit)" ) parm_.col = col::C024;
      else if (type == "Color (16 bit)") parm_.col = col::C048;
      else if (type == "Gray (1 bit)"  ) parm_.col = col::M001;
      else if (type == "Gray (8 bit)"  ) parm_.col = col::M008;
      else if (type == "Gray (16 bit)" ) parm_.col = col::M016;
      else
        log::error
          ("unknown image type value: %1%, using default")
          % type
          ;
    }

  if (parm_.is_monochrome ())
    {
      if (val_.count ("dropout"))
        {
          string dropout = val_["dropout"];

          /**/ if (dropout == "None") ;
          else if (dropout == "Red (1 bit)")    parm_.col = col::R001;
          else if (dropout == "Red (8 bit)")    parm_.col = col::R008;
          else if (dropout == "Red (16 bit)")   parm_.col = col::R016;
          else if (dropout == "Green (1 bit)")  parm_.col = col::G001;
          else if (dropout == "Green (8 bit)")  parm_.col = col::G008;
          else if (dropout == "Green (16 bit)") parm_.col = col::G016;
          else if (dropout == "Blue (1 bit)")   parm_.col = col::B001;
          else if (dropout == "Blue (8 bit)")   parm_.col = col::B008;
          else if (dropout == "Blue (16 bit)")  parm_.col = col::B016;
          else
            log::error
              ("unknown dropout value: %1%, ignoring value")
              % dropout
              ;
        }
    }

  string fmt = val_["transfer-format"];

  /**/ if (fmt == "RAW")  parm_.fmt = fmt::RAW;
  else if (fmt == "JPEG") parm_.fmt = fmt::JPG;
  else
    log::error ("unknown transfer format value: %1%, using default")
      % fmt
      ;

  if (parm_.is_bilevel ()) parm_.fmt = fmt::RAW;

  if (fmt::JPG == parm_.fmt)
    {
      if (val_.count ("jpeg-quality"))
        {
          quantity q = val_["jpeg-quality"];

          parm_.jpg = q.amount< integer > ();
        }
    }
}

void
compound_scanner::set_up_mirroring ()
{
}

void
compound_scanner::set_up_resolution ()
{
  quantity x_res;
  quantity y_res;

  if (!caps_.rss)               // coupled resolutions
    {
      x_res = val_["resolution"];
      y_res = x_res;
    }
  else
    {
      x_res = val_["resolution-x"];
      y_res = val_["resolution-y"];
    }

  parm_.rsm = x_res.amount< integer > ();
  parm_.rss = y_res.amount< integer > ();
}

void
compound_scanner::set_up_scan_area ()
{
  quantity tl_x = val_["tl-x"];
  quantity tl_y = val_["tl-y"];
  quantity br_x = val_["br-x"];
  quantity br_y = val_["br-y"];

  if (br_x < tl_x) swap (tl_x, br_x);
  if (br_y < tl_y) swap (tl_y, br_y);

  if (br_x - tl_x < min_width_ || br_y - tl_y < min_height_)
    BOOST_THROW_EXCEPTION
      (domain_error
       ((format (_("Scan area too small.\n"
                   "The area needs to be larger than %1% by %2%."))
         % min_width_ % min_height_).str ()));

  parm_.acq = std::vector< integer > ();
  parm_.acq->push_back ((*parm_.rsm *  tl_x        ).amount< integer > ());
  parm_.acq->push_back ((*parm_.rss *  tl_y        ).amount< integer > ());
  parm_.acq->push_back ((*parm_.rsm * (br_x - tl_x)).amount< integer > ());
  parm_.acq->push_back ((*parm_.rss * (br_y - tl_y)).amount< integer > ());

  if (val_.count ("crop")
      && value (toggle (true)) == val_["crop"])
    {
      if (val_.count ("crop-adjust"))
        {
          quantity q = val_["crop-adjust"];
          parm_.crp = (100 * q).amount< integer > ();
        }
    }

  if (val_.count ("doc-source")
      && string ("ADF") == val_["doc-source"])
    {
      if (val_.count ("border-fill"))
        {
          namespace flc = code_token::parameter::flc;

          string s = val_["border-fill"];

          /**/ if (s == "None" ) ;          // will use zero borders
          else if (s == "White") parm_.flc = flc::WH;
          else if (s == "Black") parm_.flc = flc::BK;
          else
            log::error ("unknown border-fill value: %1%, ignoring value")
              % s
              ;
        }

      std::vector< integer > border (4);

      if (val_.count ("border-fill")
          && value ("None") != val_["border-fill"])
        {
          if (val_.count ("border-left"))
            {
              quantity q = val_["border-left"];
              border[0] = (100 * q).amount< integer > ();
            }
          if (val_.count ("border-right"))
            {
              quantity q = val_["border-right"];
              border[1] = (100 * q).amount< integer > ();
            }
          if (val_.count ("border-top"))
            {
              quantity q = val_["border-top"];
              border[2] = (100 * q).amount< integer > ();
            }
          if (val_.count ("border-bottom"))
            {
              quantity q = val_["border-bottom"];
              border[3] = (100 * q).amount< integer > ();
            }
        }

      parm_.fla = border;
    }
}

void
compound_scanner::set_up_scan_count ()
{
  if (!val_.count ("image-count")) return;

  quantity q  = val_["image-count"];
  integer cnt = q.amount< integer > ();

  if (val_.count ("duplex")
      && value (toggle (true)) == val_["duplex"])
    {
      cnt = 2 * ((cnt + 1) / 2);        // next even integer
    }

  parm_.pag = cnt;
}

void
compound_scanner::set_up_scan_speed ()
{
}

void
compound_scanner::set_up_sharpness ()
{
}

void
compound_scanner::set_up_threshold ()
{
  if (!val_.count ("threshold")) return;

  quantity thr = val_["threshold"];
  parm_.thr = thr.amount< integer > ();
}

void
compound_scanner::set_up_transfer_size ()
{
  if (!val_.count ("transfer-size")) return;

  quantity bsz = val_["transfer-size"];
  parm_.bsz = bsz.amount< integer > ();
}

void
compound_scanner::fill_buffer_()
{
  std::deque< data_buffer >& q
    (streaming_flip_side_image_ ? rear_ : face_);

  while (q.empty ())
    {
      data_buffer buf = ++acquire_;

      cancelled_ = (buf.empty()
                    && (do_cancel_ || buf.is_cancel_requested ()));

      if (buf.is_flip_side ())
        rear_.push_back (buf);
      else
        face_.push_back (buf);

      if (acquire_.fatal_error ())
        BOOST_THROW_EXCEPTION
          (runtime_error
           (create_message (acquire_.fatal_error ()->part,
                            acquire_.fatal_error ()->what)));
    }

  buffer_ = q.front ();
  q.pop_front ();

  offset_ = 0;
}

//! \todo Don't use non-standard map::at() accessor
bool
compound_scanner::validate (const value::map& vm) const
{
  const option::map& om (doc_source_options (vm.at ("doc-source")));

  bool satisfied = true;
  for_each (value::map::value_type p, vm)
    {
      option::map::iterator it = const_cast< option::map& > (om).find (p.first);
      if (const_cast< option::map& > (om).end () != it)
        {
          if (it->constraint ())
            {
              value okay = (*it->constraint ()) (p.second);
              satisfied &= (p.second == okay);
            }
        }
      else
        {
          if (constraints_[p.first])
            {
              value okay = (*constraints_[p.first]) (p.second);
              satisfied &= (p.second == okay);
            }
        }
    }

  std::vector< restriction >::const_iterator rit;
  for (rit = restrictions_.begin (); restrictions_.end () != rit; ++rit)
    {
      satisfied &= (*rit) (vm);
    }

  return satisfied;
}

//! \todo Don't use non-standard map::at() accessor
void
compound_scanner::finalize (const value::map& vm)
{
  value::map final_vm (vm);

  if (vm.at ("doc-source") != *values_["doc-source"])
    {
      option::map& old_opts (doc_source_options (*values_["doc-source"]));
      option::map& new_opts (doc_source_options (vm.at ("doc-source")));

      remove (old_opts, final_vm);
      insert (new_opts, final_vm);
    }

  option::map::finalize (final_vm);

  // Update best effort estimate for the context at time of scan.
  // While not a *hard* requirement, this does make for a better
  // sane_get_parameters() experience.

  val_ = final_vm;
  set_up_resolution ();
  set_up_scan_area ();
  set_up_image_mode ();

  using namespace code_token::parameter;

  ctx_ = context (pixel_width (), pixel_height (), pixel_type ());

  if (fmt::JPG == parm_.fmt)
    ctx_.media_type ("image/jpeg");
}

//! \todo clarify intent of AMIN and AMAX info_ fields
void
compound_scanner::configure_adf_options ()
{
  if (!info_.adf) return;

  add_doc_source_options (adf_, *info_.adf, caps_.adf, caps_);

  if (caps_.has_duplex ())
    {
      adf_.add_options ()
        ("duplex", toggle (),
         attributes (tag::general)(level::standard),
         N_("Duplex")
         );
      if (ENABLE_RESTRICTIONS) impose (duplex_needs_adf);
    }
  {
    constraint::ptr cp (caps_.image_count (defs_.pag));

    if (cp)
      {
        adf_.add_options ()
          ("image-count", cp,
           attributes (),
           N_("Image Count")
           );
      }
  }
  {
    constraint::ptr s (caps_.double_feed ());

    if (s)
      {
        adf_.add_options ()
          ("double-feed-detection", s,
           attributes (level::standard),
           N_("Detect Double Feed")
           );
        if (ENABLE_RESTRICTIONS) impose (double_feed_needs_adf);
      }
  }

  if (info_.flatbed) flatbed_.share_values (adf_);
}

void
compound_scanner::configure_flatbed_options ()
{
  if (!info_.flatbed) return;

  add_doc_source_options (flatbed_, *info_.flatbed, caps_.fb, caps_);
}

//! \todo add alternative area option (needs own scan-area constraint)
void
compound_scanner::configure_tpu_options ()
{
  if (!info_.tpu) return;

  source_capabilities src_caps;
  add_doc_source_options (tpu_, *info_.tpu,
                          caps_.tpu ? caps_.tpu->other : src_caps,
                          caps_);

  if (info_.flatbed) flatbed_.share_values (tpu_);
  if (info_.adf) adf_.share_values (tpu_);
}

void
compound_scanner::add_doc_source_options (option::map& opts,
                                          const information::source& src,
                                          const source_capabilities& src_caps,
                                          const capabilities& caps) const
{
  add_resolution_options (opts, src);
  add_scan_area_options (opts, src);
  add_crop_option (opts, src_caps, caps);
  add_deskew_option (opts, src_caps);
  add_overscan_option (opts, src_caps);
}

void
compound_scanner::add_resolution_options (option::map& opts,
                                          const information::source& src) const
{
  if (!caps_.rsm) return;

  using namespace code_token::capability;

  integer max = src.resolution;

  if (!max)
    max = std::numeric_limits< integer >::max ();

  constraint::ptr cp_x (caps_.resolutions (RSM, defs_.rsm, max));
  constraint::ptr cp_y (caps_.resolutions (RSS, defs_.rss, max));

  if (!cp_x) return;
  if (!cp_y)                    // coupled resolutions
    {
      opts.add_options ()
        ("resolution", cp_x,
         attributes (tag::general)(level::standard),
         N_("Resolution")
         );
    }
  else
    {
      opts.add_options ()
        ("resolution-x", cp_x,
         attributes (tag::general),
         N_("Resolution X")
         )
        ("resolution-y", cp_y,
         attributes (tag::general),
         N_("Resolution Y")
         );
    }
}

/*! \todo Add a restriction to take IMX into account
 *        This will depend on scan-area and resolution
 */
void
compound_scanner::add_scan_area_options (option::map& opts,
                                         const information::source& src) const
{
  if (src.area.empty ()) return;

  const std::vector< integer >& area (src.area);

  opts.add_options ()
    ("tl-x", (from< utsushi::range > ()
              -> lower (0.)
              -> upper (double (area[0]) / 100)
              -> default_value (0.)
              ),
     attributes (tag::geometry)(level::standard),
     N_("Top Left X")
     )
    ("tl-y", (from< utsushi::range > ()
              -> lower (0.)
              -> upper (double (area[1]) / 100)
              -> default_value (0.)
              ),
     attributes (tag::geometry)(level::standard),
     N_("Top Left Y")
     )
    ("br-x", (from< utsushi::range > ()
              -> lower (0.)
              -> upper (double (area[0]) / 100)
              -> default_value (double (area[0]) / 100)
              ),
     attributes (tag::geometry)(level::standard),
     N_("Bottom Right X")
     )
    ("br-y", (from< utsushi::range > ()
              -> lower (0.)
              -> upper (double (area[1]) / 100)
              -> default_value (double (area[1]) / 100)
              ),
     attributes (tag::geometry)(level::standard),
     N_("Bottom Right Y")
     );
}

void
compound_scanner::add_crop_option (option::map& opts,
                                   const source_capabilities& src_caps,
                                   const capabilities& caps) const
{
  using namespace code_token::capability;

  BOOST_STATIC_ASSERT ((adf::CRP == fb ::CRP));
  BOOST_STATIC_ASSERT ((fb ::CRP == tpu::CRP));
  BOOST_STATIC_ASSERT ((adf::CRP == tpu::CRP));

  if (!src_caps) return;
  if (src_caps->end ()
      == std::find (src_caps->begin (), src_caps->end (), adf::CRP))
    return;

  opts.add_options ()
    ("crop", toggle (),
     attributes (tag::enhancement)(level::standard),
     N_("Crop")
     )
    ;

  utsushi::constraint::ptr cp (caps.crop_adjustment ());

  if (cp)
    {
      opts.add_options ()
        ("crop-adjust", cp,
         attributes (),
         N_("Crop Adjustment")
         )
        ;
    }
}

void
compound_scanner::add_deskew_option (option::map& opts,
                                     const source_capabilities& src_caps) const
{
  using namespace code_token::capability;

  BOOST_STATIC_ASSERT ((adf::SKEW == fb ::SKEW));
  BOOST_STATIC_ASSERT ((fb ::SKEW == tpu::SKEW));
  BOOST_STATIC_ASSERT ((adf::SKEW == tpu::SKEW));

  if (!src_caps) return;
  if (src_caps->end ()
      == std::find (src_caps->begin (), src_caps->end (), adf::SKEW))
    return;

  opts.add_options ()
    ("deskew", toggle (),
     attributes (tag::enhancement)(level::standard),
     N_("Deskew")
     )
    ;
}

void
compound_scanner::add_overscan_option (option::map& opts,
                                       const source_capabilities& src_caps) const
{
  using namespace code_token::capability;

  BOOST_STATIC_ASSERT ((adf::OVSN == fb ::OVSN));
  BOOST_STATIC_ASSERT ((fb ::OVSN == tpu::OVSN));
  BOOST_STATIC_ASSERT ((adf::OVSN == tpu::OVSN));

  if (!src_caps) return;
  if (src_caps->end ()
      == std::find (src_caps->begin (), src_caps->end (), adf::OVSN))
    return;

  /*! \todo Add doc-source specific overscan INFO to the text.  That
   *        way people get an idea of how much bigger the image will
   *        become.  There is no parameter to control the amount of
   *        overscanning.
   */
  opts.add_options ()
    ("overscan", toggle (),
     attributes (),
     N_("Overscan")
     )
    ;
}

option::map&
compound_scanner::doc_source_options (const quad& q)
{
  using namespace code_token::parameter;

  if (q == FB ) return flatbed_;
  if (q == ADF) return adf_;
  if (q == TPU) return tpu_;

  if (q != quad ())
    log::error ("no matching document source: %1%") % str (q);

  if (caps_.fb ) return flatbed_;
  if (caps_.adf) return adf_;
  if (caps_.tpu) return tpu_;

  BOOST_THROW_EXCEPTION
    (logic_error (_("internal error: no document source")));
}

option::map&
compound_scanner::doc_source_options (const value& v)
{
  using namespace code_token::parameter;

  if (v == value ("Flatbed")) return doc_source_options (FB);
  if (v == value ("ADF"))     return doc_source_options (ADF);
  if (v == value ("TPU"))     return doc_source_options (TPU);

  return doc_source_options (quad ());
}

const option::map&
compound_scanner::doc_source_options (const quad& q) const
{
  return const_cast< compound_scanner& > (*this).doc_source_options (q);
}

const option::map&
compound_scanner::doc_source_options (const value& v) const
{
  return const_cast< compound_scanner& > (*this).doc_source_options (v);
}

context::size_type
compound_scanner::pixel_width () const
{
  if (buffer_.pen) return buffer_.pen->width;
  if (buffer_.pst) return buffer_.pst->width;

  const parameters& p (streaming_flip_side_image_ ? parm_flip_ : parm_);

  return (p.acq
          ? (*p.acq)[2] - (*p.acq)[0]
          : context::unknown_size);
}

context::size_type
compound_scanner::pixel_height () const
{
  if (buffer_.pen) return buffer_.pen->height;
  if (buffer_.pst) return buffer_.pst->height;

  const parameters& p (streaming_flip_side_image_ ? parm_flip_ : parm_);

  return (p.acq
          ? (*p.acq)[3] - (*p.acq)[1]
          : context::unknown_size);
}

context::_pxl_type_
compound_scanner::pixel_type () const
{
  using namespace code_token::parameter;

  const parameters& p (streaming_flip_side_image_ ? parm_flip_ : parm_);

  if (!p.col) return context::unknown_type;

  switch (*p.col)
    {
    case col::M001:
    case col::R001:
    case col::G001:
    case col::B001:
      {
        return context::MONO;
      }
    case col::M008:
    case col::G008:
    case col::R008:
    case col::B008:
      {
        return context::GRAY8;
      }
    case col::M016:
    case col::R016:
    case col::G016:
    case col::B016:
      {
        return context::GRAY16;
      }
    case col::C024:
      {
        return context::RGB8;
      }
    case col::C048:
      {
        return context::RGB16;
      }
    case col::C003:
    default:
      {
        log::fatal ("unsupported color mode (%#08x)") % *p.col;
      }
    }
  return context::unknown_type;
}

// Helper functions for create_message()
static std::string create_adf_message (const quad& what);
static std::string create_fb_message (const quad& what);
static std::string create_tpu_message (const quad& what);

// A message for when all else fails
static std::string
fallback_message (const quad& part, const quad& what)
{
  return (format (_("Unknown device error: %1%/%2%"))
          % str (part) % str (what)).str ();
}

//! Turns a device error into a human readable message
static std::string
create_message (const quad& part, const quad& what)
{
  using namespace code_token::status;
  using namespace code_token::reply;

  BOOST_STATIC_ASSERT ((err::ADF  == info::err::ADF ));
  BOOST_STATIC_ASSERT ((err::TPU  == info::err::TPU ));
  BOOST_STATIC_ASSERT ((err::FB   == info::err::FB  ));

  BOOST_STATIC_ASSERT ((err::OPN  == info::err::OPN ));
  BOOST_STATIC_ASSERT ((err::PJ   == info::err::PJ  ));
  BOOST_STATIC_ASSERT ((err::PE   == info::err::PE  ));
  BOOST_STATIC_ASSERT ((err::ERR  == info::err::ERR ));
  BOOST_STATIC_ASSERT ((err::LTF  == info::err::LTF ));
  BOOST_STATIC_ASSERT ((err::LOCK == info::err::LOCK));
  BOOST_STATIC_ASSERT ((err::DFED == info::err::DFED));

  if (err::ADF == part) return create_adf_message (what);
  if (err::FB  == part) return create_fb_message (what);
  if (err::TPU == part) return create_tpu_message (what);

  return fallback_message (part, what);
}

static std::string
create_adf_message (const quad& what)
{
  using namespace code_token::status;

  if (err::OPN  == what)
    return _("Please close the ADF cover and try again.");
  if (err::PJ   == what)
    return _("Clear the ADF document jam and try again.");
  if (err::PE   == what)
    return _("Please put your document in the ADF before scanning.");
  if (err::DFED == what)
    return _("A multi page feed occurred in the ADF.\n"
             "Clear the document feeder and try again.");
  if (err::ERR  == what)
    return _("A fatal ADF error has occurred.\n"
             "Resolve the error condition and try again.  You may have "
             "to restart the scan dialog or application in order to be "
             "able to scan.");

  return fallback_message (err::ADF, what);
}

static std::string
create_fb_message (const quad& what)
{
  using namespace code_token::status;

  if (err::ERR  == what)
    return _("A fatal error has occurred");

  return fallback_message (err::FB, what);
}

static std::string
create_tpu_message (const quad& what)
{
  using namespace code_token::status;

  return fallback_message (err::TPU, what);
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
