//  compound-scanner.cpp -- devices that handle compound commands
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/constraint.hpp>
#include <utsushi/exception.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/media.hpp>
#include <utsushi/range.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/store.hpp>

#include "compound-scanner.hpp"
#include "scanner-inquiry.hpp"

#define for_each BOOST_FOREACH

namespace utsushi {
namespace _drv_ {
namespace esci {

using std::deque;
using std::domain_error;
using std::logic_error;
using std::runtime_error;
using std::ios_base;

namespace fs = boost::filesystem;

static system_error::error_code token_to_error_code (const quad& what);
static std::string create_message (const quad& part, const quad& what);
static system_error::error_code
token_to_error_code (const std::vector< status::error >& error);
static std::string
create_message (const std::vector< status::error >& error);

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

//! Make sure protocol and JPEG image sizes are consistent
/*! Assuming that the queue's first data_buffer has a pst member, the
 *  queue is processed until the size info has been patched, a buffer
 *  with a pen member is encountered or the queue is exhausted.  The
 *  return value indicates whether patching was successfull.  If not,
 *  the size information embedded in the JPEG data will be incorrect.
 *
 *  The image size information is at byte offsets 3 to 6 in the JPEG
 *  header's baseline DCT frame.  A baseline DCT frame starts with a
 *  \c 0xff \c 0xc0 marker.
 *
 *  The implementation neither assumes that the baseline DCT is in the
 *  first buffer, nor that it is wholly contained in a single buffer.
 *  As a matter of fact, even the height and width may be split across
 *  adjacent buffers (at the byte level).
 */
static bool
patch_jpeg_image_size_(deque< data_buffer >& q)
{
  BOOST_ASSERT (!q.empty ());
  BOOST_ASSERT ( q.front ().pst);

  byte patch[7];

  // We don't care about the first three bytes of the DCT frame.
  // These bytes indicate the length of the frame and the image's
  // bit depth.  Bytes 4 and 5 indicate the image height in big
  // endian fashion.  Bytes 6 and 7 are the image width.

  patch[3] = 0xff & (q.front ().pst->height / 256);
  patch[4] = 0xff & (q.front ().pst->height % 256);
  patch[5] = 0xff & (q.front ().pst->width  / 256);
  patch[6] = 0xff & (q.front ().pst->width  % 256);

  int patch_size = sizeof (patch) / sizeof (*patch);
  int offset     = -1;          // into baseline DCT frame payload
  byte state     = 0x00;        // anything but 0xff

  deque< data_buffer >::iterator it (q.begin ());

  do
    {
      byte *head = it->data ();
      byte *tail = head + it->size ();

      while (offset < patch_size
             && head != tail)
        {
          if (0 <= offset)      // looking at baseline DCT payload
            {
              if (2 < offset)   // looking at image size
                {
                  traits::assign (*head, patch[offset]);
                }
              ++offset;
            }

          if (traits::eq (0xff, state))
            {
              if (traits::eq (0xc0, *head))
                {
                  offset = 0;   // found baseline DCT
                }
            }

          traits::assign (state, *head);
          ++head;
        }
      if (it->pen) break;
    }
  while (offset < patch_size
         && ++it != q.end ());

  return (offset == patch_size);
}

//! Replace image size estimate with actual size
/*! The implementation works with a queue that has a data_buffer::pst
 *  on its first buffer.  It searches for the first buffer with a pen
 *  member and, if one is found, copies the pen member's size to the
 *  pst member of the buffer at the queue's front.
 *
 *  If no buffer with pen member is found, the queue is not modified.
 *
 *  The \a format argument is used to invoke add-on functions that
 *  know how to modify image size information embedded in the image
 *  data itself if necessary.
 *
 *  \sa patch_jpeg_image_size_
 */
static bool
patch_image_size_(deque< data_buffer >& q,
                  const boost::optional< quad >& format)
{
  BOOST_ASSERT (!q.empty ());
  BOOST_ASSERT ( q.front ().pst);

  deque< data_buffer >::const_iterator it (q.begin ());

  while (it != q.end () && !it->pen) ++it;

  if (it == q.end ())
    {
      log::error ("no image end info found");
      return false;
    }

  q.front ().pst->width  = it->pen->width;
  q.front ().pst->height = it->pen->height;

  using namespace code_token::parameter;

  if (fmt::JPG == format)
    return patch_jpeg_image_size_(q);

  return true;
}

static quantity
nearest_(const quantity& q, const constraint::ptr cp)
{
  /**/ if (dynamic_cast< range * > (cp.get ()))
    {
      range *rp = dynamic_cast< range * > (cp.get ());

      /**/ if (q < rp->lower ()) return rp->lower ();
      else if (q > rp->upper ()) return rp->upper ();
      else return q;
    }
  else if (dynamic_cast< store * > (cp.get ()))
    {
      store *sp = dynamic_cast< store * > (cp.get ());

      store::const_iterator it = sp->begin ();
      store::const_iterator rv = sp->begin ();
      quantity diff;

      while (sp->end () != it)
        {
          if (sp->begin () == it)
            {
              diff = abs (q - *it);
              rv   = it;
            }
          else
            {
              quantity d = abs (q - *it);
              if (d < diff)
                {
                  diff = d;
                  rv = it;
                }
            }
          ++it;
        }
      if (sp->end () != sp->begin ()) return *rv;
    }

  log::error ("no nearest value found, returning as is");

  return q;
}

result_code
do_mechanics (connexion::ptr cnx, scanner_control& ctrl,
              const quad& part, const quad& action)
{
  bool do_finish = !ctrl.is_in_session ();
  result_code rc;

  using namespace code_token::mechanic;

  if (ADF == part)
    {
      hardware_status stat;

      *cnx << ctrl.get (stat);

      switch (action)
        {
        case adf::LOAD:
        case adf::EJCT:
          {
            *cnx << ctrl.mechanics (part, action);
            if (!ctrl.fatal_error ())
              {
                rc = result_code (system_error::no_error,
                                  adf::LOAD == action
                                  ? _("Loading completed")
                                  : _("Ejecting completed"));
              }
            break;
          }
        case adf::CLEN:
        case adf::CALB:
          {
            using namespace code_token::status;

            if (!stat.is_battery_low (err::ADF))
              {
                *cnx << ctrl.mechanics (part, action);
                if (!ctrl.fatal_error())
                  {
                    do
                      {
                        *cnx << ctrl.get (stat);
                      }
                    while (ctrl.is_busy () && ctrl.delay_elapsed ());

                    if (!ctrl.fatal_error())
                      {
                        rc = result_code (system_error::no_error,
                                          adf::CLEN == action
                                          ? _("Cleaning is complete.")
                                          : _("Calibration is complete."));
                      }
                  }
              }
            else
              {
                rc = result_code (system_error::battery_low,
                                  adf::CLEN == action
                                  ? _("Cleaning is failed.")
                                  : _("Calibration is failed."));
              }
            break;
          }
        default:
          {
            log::alert ("unknown %1% request parameters: %2%%3%")
              % str (ADF) % str (part) % str (action)
              ;
          }
        }
    }
  else                          // just do it
    {
      *cnx << ctrl.mechanics (part, action);
    }

  if (!rc && (ctrl.fatal_error () || ctrl.is_warming_up ()))
    {
      /**/ if (adf::LOAD == action)
        rc = result_code (system_error::unknown_error, _("Loading failed"));
      else if (adf::EJCT == action)
        rc = result_code (system_error::unknown_error, _("Ejecting failed"));
      else if (adf::CLEN == action)
        rc = result_code (system_error::unknown_error, _("Cleaning is failed."));
      else if (adf::CALB == action)
        rc = result_code (system_error::unknown_error, _("Calibration is failed."));
      else
        rc = result_code (system_error::unknown_error, _("Maintenance failed"));
    }

  if (do_finish) *cnx << ctrl.finish ();

  return rc;
}

static void
erase (std::vector< quad >& v, const quad& token)
{
  v.erase (remove (v.begin (), v.end (), token), v.end ());
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
  {
    log::trace ("getting basic device information");

    scanner_inquiry cmd;

    *cnx_ << cmd.get (const_cast< information&  > (info_));
    *cnx_ << cmd.get (const_cast< capabilities& > (caps_));
    *cnx_ << cmd.get (const_cast< capabilities& > (caps_flip_), true);
  }

  if (!get_file_defs_(info_.product_name ()))
    {
      log::error ("falling back to device defaults");

      scanner_control cmd;      // resets device state

      *cnx_ << cmd.get (const_cast< information&  > (info_));
      *cnx_ << cmd.get (const_cast< capabilities& > (caps_));
      *cnx_ << cmd.get (const_cast< capabilities& > (caps_flip_), true);
      *cnx_ << cmd.get (const_cast< parameters&   > (defs_));
      *cnx_ << cmd.get (const_cast< parameters&   > (defs_flip_), true);
    }

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

  // Disable flip-side scan parameter support because driver support
  // for it is not ready yet.  The protocol specification is missing
  // information needed for implementation.

  if (caps_flip_)
    {
      log::error ("disabling flip-side scan parameter support");

      capabilities& caps_flip (const_cast< capabilities& > (caps_flip_));
      parameters&   defs_flip (const_cast< parameters& > (defs_flip_));

      caps_flip = capabilities ();
      defs_flip = parameters ();
    }

  // Disable load and eject functionality until it can be verified to
  // work as intended.

  if (caps_.adf)
    {
      capabilities& caps (const_cast< capabilities& > (caps_));

      using namespace code_token::capability;
      if (caps.adf && caps.adf->flags) erase (*caps.adf->flags, adf::LOAD);
      if (caps.adf && caps.adf->flags) erase (*caps.adf->flags, adf::EJCT);
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
    constraint::ptr cp (caps_.formats (defs_.fmt));

    if (cp)
      {
        add_options ()
          ("transfer-format", cp,
           attributes (level::standard),
           N_("Transfer Format"),
           N_("Selecting a compressed format such as JPEG normally"
              " results in faster device side processing.")
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
  // \todo The driver should not provide this option as it does not do
  //       anything with it.  Any actions that need to be taken should
  //       be taken by the application.  However, the application does
  //       need a way to figure out whether it makes sense to present
  //       the functionality to the user.  That should really be based
  //       on some kind of driver capability query.
  if (info_.truncates_at_media_end
      || caps_.has_media_end_detection ())
    {
      add_options ()
        ("force-extent", toggle (true),
         attributes (tag::enhancement),
         N_("Force Extent"),
         N_("Force the image size to equal the user selected size."
            "  Scanners may trim the image data to the detected size of"
            " the document.  This may result in images that are not all"
            " exactly the same size.  This option makes sure all image"
            " sizes match the selected area.\n"
            "Note that this option may slow down application/driver side"
            " processing.")
         )
        ;
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

  {                             // add actions
    namespace mech = code_token::mechanic;
    using boost::bind;

    // FIXME flaming hack alert, abusing text() for progress message
    if (caps_.can_calibrate ())
      {
        action_->add_actions ()
          ("calibrate", bind (do_mechanics, cnx_, ref (acquire_),
                              mech::ADF, mech::adf::CALB),
           _("Calibration"),
           _("Calibrating..."));
      }
    if (caps_.can_clean ())
      {
        action_->add_actions ()
          ("clean", bind (do_mechanics, cnx_, ref (acquire_),
                          mech::ADF, mech::adf::CLEN),
           _("Cleaning"),
           _("Cleaning..."));
      }
    if (caps_.can_eject ())
      {
        action_->add_actions ()
          ("eject", bind (do_mechanics, cnx_, ref (acquire_),
                          mech::ADF, mech::adf::EJCT),
           _("Eject"),
           _("Ejecting ..."));
      }
    if (caps_.can_load ())
      {
        action_->add_actions ()
          ("load", bind (do_mechanics, cnx_, ref (acquire_),
                         mech::ADF, mech::adf::LOAD),
           _("Load"),
           _("Loading..."));
      }
  }
}

bool
compound_scanner::is_single_image () const
{
  return (value ("ADF") != *values_["doc-source"]
          || value (1) == *values_["image-count"]);
}

bool
compound_scanner::is_consecutive () const
{
  bool rv (parm_.adf || parm_flip_.adf);
  if (!rv) *cnx_ << acquire_.finish ();
  return rv;
}

static
bool
at_image_start (const std::deque< data_buffer >& q)
{
  return (!q.empty () && q.front ().pst);
}

bool
compound_scanner::obtain_media ()
{
  buffer_.clear ();
  offset_ = 0;

  if (acquire_.is_duplexing ())
    streaming_flip_side_image_ = (1 == image_count_ % 2);

  deque< data_buffer >& q (streaming_flip_side_image_ ? rear_ : face_);

  while (!cancelled_ && !media_out () && !at_image_start (q))
    {
      queue_image_data_();
    }

  bool rv (!cancelled_ && !media_out () && at_image_start (q));
  if (!rv) *cnx_ << acquire_.finish ();
  return rv;
}

bool
compound_scanner::set_up_image ()
{
  fill_data_queue_();           // until width and height are known

  if (cancelled_)
    {
      *cnx_ << acquire_.finish ();
      return false;
    }

  ctx_ = context (pixel_width (), pixel_height (), pixel_type ());
  ctx_.resolution (*parm_.rsm, *parm_.rss);

  ctx_.content_type (transfer_content_type_(parm_));

  if (buffer_.pst
      && 0 != buffer_.pst->padding
      && compressed_transfer_(parm_))
    {
      log::alert ("ignoring %1% byte padding")
        % buffer_.pst->padding;
      buffer_.pst->padding = 0;
    }

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
      fill_data_queue_();
      if (cancelled_)
        {
          *cnx_ << acquire_.finish ();
          return traits::eof ();
        }
    }

  streamsize rv = std::min (buffer_.size () - offset_, sz);

  traits::copy (data, reinterpret_cast< const octet * >
                (buffer_.data () + offset_), rv);
  offset_ += rv;

  return rv;
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
  media_out_ = false;

  if (val_.count ("scan-area")
      && value ("Automatic") == val_["scan-area"])
    {
      media size = probe_media_size_(val_["doc-source"]);
      update_scan_area_(size, val_);
      option::map::finalize (val_);
    }
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

  {
    using namespace code_token::status;

    string doc_src = val_["doc-source"];
    quad   src     = quad ();

    if (doc_src == "Document Table")  src = psz::FB;
    else if (doc_src == "ADF") src = psz::ADF;

    quad error = stat_.error (src);

    if (error)
      {
        *cnx_ << acquire_.finish ();

        BOOST_THROW_EXCEPTION
          (system_error
           (token_to_error_code (error), create_message (src, error)));
      }
  }

  *cnx_ << acquire_.start ();

  if (acquire_.fatal_error ())
    {
      std::vector < status::error > error (*acquire_.fatal_error ());

      *cnx_ << acquire_.finish ();

      BOOST_THROW_EXCEPTION
        (system_error
         (token_to_error_code (error), create_message (error)));
    }

  if (parm_.bsz)
    buffer_size_ = *parm_.bsz;

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
      else if (v == value ("Thin"))
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

      if (src == "Document Table")  parm_.fb  = src_opts;
      else if (src == "ADF") parm_.adf = src_opts;
      else if (src == "Transparency Unit") parm_.tpu = src_opts;
    }
  else                          // only one document source
    {
      if (caps_.fb ) parm_.fb  = src_opts;
      if (caps_.adf) parm_.adf = src_opts;
      if (caps_.tpu) parm_.tpu = src_opts;
    }

  if (parm_.adf && caps_.has_media_end_detection ())
    {
      parm_.adf->push_back (adf::PEDT);
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

  if (!parm_.is_color ())
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

  if (val_.count ("speed"))
    {
      toggle t = val_["speed"];
      if (t)
        {
          /**/ if (col::M001 == parm_.col) parm_.col = col::M008;
          else if (col::R001 == parm_.col) parm_.col = col::R008;
          else if (col::G001 == parm_.col) parm_.col = col::G008;
          else if (col::B001 == parm_.col) parm_.col = col::B008;
        }
    }

  string fmt = val_["transfer-format"];

  /**/ if (fmt == "RAW")  parm_.fmt = fmt::RAW;
  else if (fmt == "JPEG") parm_.fmt = fmt::JPG;
  else
    log::error ("unknown transfer format value: %1%, using default")
      % fmt
      ;

  // Because val_ contains the actual value, we have to make sure we
  // send a token that the firmware understands.  The firmware takes
  // some liberty with the interpretation of said token and may very
  // well return data in a different format.

  if (caps_.fmt
      && !caps_.fmt->empty ())
    {
      if (!std::count (caps_.fmt->begin (), caps_.fmt->end (),
                       parm_.fmt))
        {
          parm_.fmt = caps_.fmt->front ();
        }
    }

  if (fmt::JPG == transfer_format_(parm_))
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

          std::vector< integer > border (4);

          if (value ("None") != val_["border-fill"])
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

boost::optional< quad >
compound_scanner::transfer_format_(const parameters& p) const
{
  using namespace code_token::parameter;

  return (!p.is_bilevel ()
          ? p.fmt
          : fmt::RAW);
}

bool
compound_scanner::compressed_transfer_(const parameters& p) const
{
  using namespace code_token::parameter;

  return fmt::JPG == transfer_format_(p);
}

std::string
compound_scanner::transfer_content_type_(const parameters& p) const
{
  using namespace code_token::parameter;

  std::string rv = context().content_type();

  if (fmt::JPG == transfer_format_(p))
    {
      rv = "image/jpeg";
    }

  return rv;
}

void
compound_scanner::queue_image_data_()
{
  bool do_cancel = cancel_requested ();

  if (do_cancel) acquire_.cancel ();

  data_buffer buf = ++acquire_;

  cancelled_ = (buf.empty()
                && (do_cancel || buf.is_cancel_requested ()));
  if (cancelled_)
    {
      cancel ();            // notify idevice::read()
      *cnx_ << acquire_.finish ();
    }

  if (buf.is_flip_side ())
    rear_.push_back (buf);
  else
    face_.push_back (buf);

  if (acquire_.fatal_error ())
    {
      std::vector < status::error > error (*acquire_.fatal_error ());

      *cnx_ << acquire_.finish ();

      BOOST_THROW_EXCEPTION
        (system_error
         (token_to_error_code (error), create_message (error)));
    }
}

void
compound_scanner::fill_data_queue_()
{
  const parameters&     p (streaming_flip_side_image_ ? parm_flip_ : parm_);
  deque< data_buffer >& q (streaming_flip_side_image_ ? rear_ : face_);

  while (!cancelled_ && !enough_image_data_(p, q))
    {
      queue_image_data_();
    }

  if (q.front ().pst && use_final_image_size_(p))
    {
      patch_image_size_(q, transfer_format_(p));
    }

  buffer_ = q.front ();
  q.pop_front ();

  offset_    = 0;
  media_out_ = buffer_.media_out ();
}

bool
compound_scanner::media_out () const
{
  return (media_out_ || acquire_.media_out ());
}

bool
compound_scanner::use_final_image_size_(const parameters& parm) const
{
  bool rv = info_.truncates_at_media_end;

  if (!rv && parm.adf)
    {
      namespace adf = code_token::parameter::adf;

      rv = (parm.adf->end ()
            != std::find (parm.adf->begin (), parm.adf->end (),
                          adf::PEDT));
    }

  return rv;
}

bool
compound_scanner::enough_image_data_(const parameters& parm,
                                     const deque< data_buffer >& q) const
{
  if (q.empty ()) return false;

  // Handle status feedback with a priority higher than PEN.  A PST
  // status should fall through.  A queue with only PST data may or
  // may not be sufficient.

  if (!q.back ().err.empty ()) return true;
  if ( q.back ().nrd)
    {
      log::trace ("unexpected not-ready status while acquiring");
      return true;
    }

  return (use_final_image_size_(parm)
          ?  q.back ().pen
          : !q.empty ());
}

/*! \todo Make repeat_count configurable
 *  \todo Make delay time interval configurable
 */
media
compound_scanner::probe_media_size_(const string& doc_source)
{
  using namespace code_token::status;

  quad src = quad();
  media size = media (length (), length ());

  if (doc_source == "Document Table")  src = psz::FB;
  else if (doc_source == "ADF") src = psz::ADF;

  if (src)
    {
      int repeat_count = 5;
      do
        {
          *cnx_ << acquire_.get (stat_);
        }
      while (!stat_.size_detected (src)
             && acquire_.delay_elapsed ()
             && --repeat_count);

      *cnx_ << acquire_.finish ();

      if (stat_.size_detected (src))
        {
          size = stat_.size (src);
        }
      else
        {
          log::error
            ("unable to determine media size in allotted time");
        }
    }
  else
    {
      log::error
        ("document size detection not enabled for current document"
         " source");
    }

  return size;
}

void
compound_scanner::update_scan_area_(const media& size, value::map& vm) const
{
  if (size.width () > 0 && size.height () > 0)
    {
      quantity tl_x (0.0);
      quantity tl_y (0.0);
      quantity br_x = size.width ();
      quantity br_y = size.height ();

      align_document (vm["doc-source"],
                      tl_x, tl_y, br_x, br_y);

      vm["tl-x"] = tl_x;
      vm["tl-y"] = tl_y;
      vm["br-x"] = br_x;
      vm["br-y"] = br_y;
    }
  else
    {
      log::brief ("using default scan-area");
      // This relies on default values being set to lower() values for
      // tl-x and tl-y and upper() values for br-x and br-y.
      // Note that alignment is irrelevant for the maximum size.
      vm["tl-x"] = constraints_["tl-x"]->default_value ();
      vm["tl-y"] = constraints_["tl-y"]->default_value ();
      vm["br-x"] = constraints_["br-x"]->default_value ();
      vm["br-y"] = constraints_["br-y"]->default_value ();
    }
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

  {
    // Users should be shown the actual transfer-format value, *not*
    // whatever token is sent to the firmware.  That means that the
    // values_ and constraints_ as well as the internal copy of the
    // values_ in val_ need to support that.

    string type = final_vm["image-type"];
    toggle t = false;

    if (final_vm.count ("speed"))
      t = final_vm["speed"];

    if (!t &&
        (   type == "Color (1 bit)"
         || type == "Gray (1 bit)"
         || type == "Red (1 bit)"
         || type == "Blue (1 bit)"
         || type == "Green (1 bit)"))
      {
        if ((*constraints_["transfer-format"]) (string ("RAW"))
            != value (string ("RAW")))
          {
            constraints_["transfer-format"]
              = shared_ptr <constraint >
              (from< store > () -> alternative (N_("RAW")));
          }
        final_vm["transfer-format"] = "RAW";
      }
    else
      {
        constraints_["transfer-format"] = caps_.formats (defs_.fmt);
        if (final_vm["transfer-format"]
            != (*constraints_["transfer-format"]) (final_vm["transfer-format"]))
          {
            final_vm["transfer-format"]
              = constraints_["transfer-format"]->default_value ();
          }
      }
  }

  string scan_area = final_vm["scan-area"];
  if (scan_area != "Manual")
    {
      media size = media (length (), length ());

      /**/ if (scan_area == "Maximum")
        {
          size = media (length (), length ());
        }
      else if (scan_area == "Automatic")
        {
          size = probe_media_size_(final_vm["doc-source"]);
        }
      else                      // well-known media size
        {
          size = media::lookup (scan_area);
        }
      update_scan_area_(size, final_vm);
    }

  // Link force-extent default value to scan-area changes.

  if (final_vm.count ("force-extent")
      && final_vm["scan-area"] != *values_["scan-area"])
    {
      final_vm["force-extent"] = toggle (scan_area != "Automatic");
    }

  {                             // minimal scan area check
    quantity tl_x = final_vm["tl-x"];
    quantity tl_y = final_vm["tl-y"];
    quantity br_x = final_vm["br-x"];
    quantity br_y = final_vm["br-y"];

    if (br_x < tl_x) swap (tl_x, br_x);
    if (br_y < tl_y) swap (tl_y, br_y);

    if (br_x - tl_x < min_width_ || br_y - tl_y < min_height_)
      BOOST_THROW_EXCEPTION
        (constraint::violation
         ((format (_("Scan area too small.\n"
                     "The area needs to be larger than %1% by %2%."))
           % min_width_ % min_height_).str ()));
  }

  {                             // finalize resolution options
    boost::optional< toggle > resample;
    boost::optional< toggle > sw_bound;
    boost::optional< toggle > bound;
    {
      if (final_vm.count ("enable-resampling"))
        {
          toggle t = final_vm["enable-resampling"];
          resample = t;
        }
      if (final_vm.count ("sw-resolution-bind"))
        {
          toggle t = final_vm["sw-resolution-bind"];
          sw_bound = t;
        }
      if (final_vm.count ("resolution-bind"))
        {
          toggle t = final_vm["resolution-bind"];
          bound = t;
        }
      if (resample && sw_bound && bound)
        {
          if (*resample)
            {
              *bound = *sw_bound;
              final_vm["resolution-bind"] = *bound;
            }
          else
            {
              *sw_bound = *bound;
              final_vm["sw-resolution-bind"] = *sw_bound;
            }
        }
    }

    BOOST_ASSERT (!bound || !sw_bound || (*bound == *sw_bound));

    if (!bound)
      {
        // Both may be absent but if one exists, the other does not.
        if (final_vm.count ("resolution-x"))
          {
            descriptors_["resolution-x"]->read_only (false);
            descriptors_["resolution-y"]->read_only (false);
            descriptors_["resolution-x"]->active (true);
            descriptors_["resolution-y"]->active (true);
          }
        if (final_vm.count ("resolution"))
          {
            descriptors_["resolution"]->read_only (false);
            descriptors_["resolution"]->active (true);
          }
      }
    else
      {
        descriptors_["resolution-x"]->read_only (*bound);
        descriptors_["resolution-y"]->read_only (*bound);

        descriptors_["resolution-bind"]->active (true);
        descriptors_["resolution-x"]   ->active (true);
        descriptors_["resolution-y"]   ->active (true);
        descriptors_["resolution"]     ->active (*bound);

        // We bind the resolutions after we handle the resampling
        // implications.  If resampling, the selected resolutions
        // will have an impact on any resolutions we need to send
        // to the device.
      }

    if (resample)
      {
        if (!sw_bound)
          {
            // Both may be absent but if one exists, the other does not.
            if (final_vm.count ("sw-resolution-x"))
              {
                descriptors_["sw-resolution-x"]->read_only (false);
                descriptors_["sw-resolution-y"]->read_only (false);
                descriptors_["sw-resolution-x"]->active (*resample);
                descriptors_["sw-resolution-y"]->active (*resample);
              }
            if (final_vm.count ("sw-resolution"))
              {
                descriptors_["sw-resolution"]->read_only (false);
                descriptors_["sw-resolution"]->active (*resample);
              }
          }
        else
          {
            descriptors_["sw-resolution-x"]->read_only (*sw_bound);
            descriptors_["sw-resolution-y"]->read_only (*sw_bound);

            descriptors_["sw-resolution-bind"]->active (*resample);
            descriptors_["sw-resolution-x"]   ->active (*resample);
            descriptors_["sw-resolution-y"]   ->active (*resample);
            descriptors_["sw-resolution"]     ->active (*resample
                                                        && *sw_bound);

            if (*sw_bound)
              {
                final_vm["sw-resolution-x"] = final_vm["sw-resolution"];
                final_vm["sw-resolution-y"] = final_vm["sw-resolution"];
              }
            else
              {
                final_vm["sw-resolution"]
                  = nearest_(final_vm["sw-resolution-x"],
                             constraints_["sw-resolution"]);
              }
          }

        if (!bound)
          {
            if (final_vm.count ("resolution-x"))
              {
                descriptors_["resolution-x"]->active (!*resample);
                descriptors_["resolution-y"]->active (!*resample);
              }
            if (final_vm.count ("resolution"))
              {
                descriptors_["resolution"]->active (!*resample);
              }
          }
        else
          {
            descriptors_["resolution-bind"]->active (!*resample);
            descriptors_["resolution-x"]   ->active (!*resample);
            descriptors_["resolution-y"]   ->active (!*resample);
            descriptors_["resolution"]     ->active (!*resample && *bound);
          }

        if (*resample)          // update device resolutions
          {
            if (final_vm.count ("resolution-x"))
              {
                quantity q;

                if (final_vm.count ("sw-resolution"))   // fallback
                  {
                    q = final_vm["sw-resolution"];
                  }

                if (final_vm.count ("sw-resolution-x"))
                  {
                    q = final_vm["sw-resolution-x"];
                  }

                BOOST_ASSERT (quantity () != q);

                final_vm["resolution-x"]
                  = nearest_(q, constraints_["resolution-x"]);

                if (final_vm.count ("sw-resolution-y"))
                  {
                    q = final_vm["sw-resolution-y"];
                  }

                final_vm["resolution-y"]
                  = nearest_(q, constraints_["resolution-y"]);
              }

            if (final_vm.count ("resolution"))
              {
                quantity q;

                if (final_vm.count ("sw-resolution-x")) // fallback
                  {
                    q = std::max< quantity > (final_vm["sw-resolution-x"],
                                              final_vm["sw-resolution-y"]);
                  }

                if (final_vm.count ("sw-resolution"))
                  {
                    q = final_vm["sw-resolution"];
                  }

                BOOST_ASSERT (quantity () != q);

                final_vm["resolution"]
                  = nearest_(q, constraints_["resolution"]);
              }
          }
      }

    if (bound)
      {
        if (*bound)
          {
            final_vm["resolution-x"] = final_vm["resolution"];
            final_vm["resolution-y"] = final_vm["resolution"];
          }
        else
          {
            final_vm["resolution"] = nearest_(final_vm["resolution-x"],
                                              constraints_["resolution"]);
          }
      }

    if (resample)
      {
        if (!*resample)         // follow device resolutions
          {
            if (final_vm.count ("sw-resolution-x"))
              {
                if (final_vm.count ("resolution-x"))
                  {
                    final_vm["sw-resolution-x"]
                      = nearest_(final_vm["resolution-x"],
                                 constraints_["sw-resolution-x"]);
                    final_vm["sw-resolution-y"]
                      = nearest_(final_vm["resolution-y"],
                                 constraints_["sw-resolution-y"]);
                  }
                else
                  {
                    final_vm["sw-resolution-x"]
                      = nearest_(final_vm["resolution"],
                                 constraints_["sw-resolution-x"]);
                    final_vm["sw-resolution-y"]
                      = nearest_(final_vm["resolution"],
                                 constraints_["sw-resolution-y"]);
                  }
              }

            if (final_vm.count ("sw-resolution"))
              {
                if (final_vm.count ("resolution"))
                  {
                    final_vm["sw-resolution"]
                      = nearest_(final_vm["resolution"],
                                 constraints_["sw-resolution"]);
                  }
                else
                  {
                    final_vm["sw-resolution"]
                      = nearest_(final_vm["resolution-x"],
                                 constraints_["sw-resolution"]);
                  }
              }
          }
      }
  }

  option::map::finalize (final_vm);
  relink ();

  // Update best effort estimate for the context at time of scan.
  // While not a *hard* requirement, this does make for a better
  // sane_get_parameters() experience.

  val_ = final_vm;
  set_up_image_mode ();
  set_up_resolution ();
  set_up_scan_area ();

  using namespace code_token::parameter;

  ctx_ = context (pixel_width (), pixel_height (), pixel_type ());
  ctx_.content_type (transfer_content_type_(parm_));
}

//! \todo clarify intent of AMIN and AMAX info_ fields
void
compound_scanner::configure_adf_options ()
{
  if (!info_.adf) return;

  add_doc_source_options (adf_, *info_.adf, caps_.adf->flags, caps_);

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

  add_doc_source_options (flatbed_, *info_.flatbed, caps_.fb->flags, caps_);
}

//! \todo add alternative area option (needs own scan-area constraint)
void
compound_scanner::configure_tpu_options ()
{
  if (!info_.tpu) return;

  source_capabilities src_caps;
  add_doc_source_options (tpu_, *info_.tpu,
                          caps_.tpu ? caps_.tpu->flags : src_caps,
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

constraint::ptr
intersection_of_(const constraint::ptr& cp_x, const constraint::ptr& cp_y)
{
  constraint::ptr rv;

  /**/ if (dynamic_pointer_cast< store > (cp_x) && cp_y)
    {
      store::ptr sp_x = dynamic_pointer_cast< store > (cp_x);
      store::ptr sp = make_shared< store > ();
      store::const_iterator it = sp_x->begin ();

      while (sp_x->end () != it)
        {
          if (*it == (*cp_y) (*it))
            sp->alternative (*it);
          ++it;
        }
      if (0 < sp->size ()) rv = sp;
    }
  else if (dynamic_pointer_cast< store > (cp_y))
    {
      return intersection_of_(cp_y, cp_x);
    }
  else if (   dynamic_pointer_cast< range > (cp_x)
           && dynamic_pointer_cast< range > (cp_y))
    {
      range::ptr rp_x = dynamic_pointer_cast< range > (cp_x);
      range::ptr rp_y = dynamic_pointer_cast< range > (cp_y);
      range::ptr rp = make_shared< range > ();

      quantity lo = std::max (rp_x->lower (), rp_y->lower ());
      quantity hi = std::min (rp_x->upper (), rp_y->upper ());

      if (lo <= hi)
        {
          rp->lower (lo)->upper (hi);
          rv = rp;
        }
    }

  if (rv)                       // try to preserve one of the defaults
    {
      /**/ if (cp_x->default_value () == (*rv) (cp_x->default_value ()))
        rv->default_value (cp_x->default_value ());
      else if (cp_y->default_value () == (*rv) (cp_y->default_value ()))
        rv->default_value (cp_y->default_value ());
    }
  return rv;
}

void
compound_scanner::add_resolution_options (option::map& opts,
                                          const information::source& src) const
{
  using namespace code_token::capability;

  integer max = src.resolution;

  if (!max)
    max = std::numeric_limits< integer >::max ();

  constraint::ptr cp_x (caps_.resolutions (RSM, defs_.rsm, max));
  constraint::ptr cp_y (caps_.resolutions (RSS, defs_.rss));
  constraint::ptr cp (intersection_of_(cp_x, cp_y));

  if (cp)                       // both cp_x and cp_y exist too
    {
      opts.add_options ()
        ("resolution-bind", toggle (true),
         attributes (tag::general),
         N_("Bind X and Y resolutions")
         )
        ("resolution", cp,
         attributes (tag::general)(level::standard),
         N_("Resolution")
         )
        ("resolution-x", cp_x,
         attributes (tag::general),
         N_("X Resolution")
         )
        ("resolution-y", cp_y,
         attributes (tag::general),
         N_("Y Resolution")
         )
        ;
    }
  else
    {
      if (cp_x && cp_y)
        {
          opts.add_options ()
            ("resolution-x", cp_x,
             attributes (tag::general)(level::standard),
             N_("X Resolution")
             )
            ("resolution-y", cp_y,
             attributes (tag::general)(level::standard),
             N_("Y Resolution")
             )
            ;
        }
      else
        {
          cp = (cp_x ? cp_x : cp_y);
          if (cp)
            {
              opts.add_options ()
                ("resolution", cp,
                 attributes (tag::general)(level::standard),
                 N_("Resolution")
                 )
                ;
            }
          else
            {
              log::brief ("no hardware resolution options");
            }
        }
    }

  if (!res_x_ && !res_y_) return;

  // repeat the above for software-emulated resolution options

  opts.add_options ()
    ("enable-resampling", toggle (true),
     attributes (tag::general),
     N_("Enable Resampling"),
     N_("This option provides the user with a wider range of supported"
        " resolutions.  Resolutions not supported by the hardware will"
        " be achieved through image processing methods.")
     );

  cp = intersection_of_(res_x_, res_y_);

  if (cp)
    {
      opts.add_options ()
        ("sw-resolution-bind", toggle (true),
         attributes (tag::general),
         N_("Bind X and Y resolutions")
         )
        ("sw-resolution", cp,
         attributes (tag::general)(level::standard).emulate (true),
         N_("Resolution")
         )
        ("sw-resolution-x", res_x_,
         attributes (tag::general).emulate (true),
         N_("X Resolution")
         )
        ("sw-resolution-y", res_y_,
         attributes (tag::general).emulate (true),
         N_("Y Resolution")
         )
        ;
    }
  else
    {
      if (res_x_ && res_y_)
        {
          opts.add_options ()
            ("sw-resolution-x", res_x_,
             attributes (tag::general)(level::standard).emulate (true),
             N_("X Resolution")
             )
            ("sw-resolution-y", res_y_,
             attributes (tag::general)(level::standard).emulate (true),
             N_("Y Resolution")
             )
            ;
        }
      else
        {
          cp = (res_x_ ? res_x_ : res_y_);
          if (cp)
            {
              opts.add_options ()
                ("sw-resolution", cp,
                 attributes (tag::general)(level::standard).emulate (true),
                 N_("Resolution")
                 )
                ;
            }
          else
            {
              log::brief ("no software resolution options");
            }
        }
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

  std::list< std::string > areas = media::within (double (area[0]) / 100,
                                                  double (area[1]) / 100);
  areas.push_back (N_("Manual"));
  areas.push_back (N_("Maximum"));
  if (src.supports_size_detection ())
    areas.push_back (("Automatic"));

  opts.add_options ()
    ("scan-area", (from< utsushi::store > ()
                   -> alternatives (areas.begin (), areas.end ())
                   -> default_value ("Manual")),
     attributes (tag::general)(level::standard),
     N_("Scan Area")
     )
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

  if (v == value ("Document Table")) return doc_source_options (FB);
  if (v == value ("ADF"))     return doc_source_options (ADF);
  if (v == value ("Transparency Unit"))     return doc_source_options (TPU);

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

void
compound_scanner::align_document (const string& doc_source,
                                  quantity& tl_x, quantity& tl_y,
                                  quantity& br_x, quantity& br_y) const
{
  using namespace code_token::information;

  BOOST_STATIC_ASSERT ((adf::LEFT == fb::LEFT));
  BOOST_STATIC_ASSERT ((adf::CNTR == fb::CNTR));
  BOOST_STATIC_ASSERT ((adf::RIGT == fb::RIGT));

  quad align (adf::CNTR);       // default as per spec
  double max_width  = 0;
  double max_height = 0;

  if (doc_source == "ADF")
    {
      align = info_.adf->alignment;
      max_width  = info_.adf->area[0];
      max_height = info_.adf->area[1];
    }
  if (doc_source == "Document Table")
    {
      align = info_.flatbed->alignment;
      max_width  = info_.flatbed->area[0];
      max_height = info_.flatbed->area[1];
    }
  if (doc_source == "Transparency Unit")
    {
      // TPU has no alignment "attribute"
      max_width  = info_.tpu->area[0];
      max_height = info_.tpu->area[1];
    }

  if (0 == max_width)           // nothing we can do
    return;
  if (0 == max_height)          // nothing we can do
    return;

  max_width  /= 100.0;          // conversion to inches
  max_height /= 100.0;

  quantity width (br_x - tl_x);
  quantity x_shift;
  quantity y_shift;             // no specification, assume 0

  if (adf::LEFT == align) x_shift =  0.0;
  if (adf::CNTR == align) x_shift = (max_width - width) / 2;
  if (adf::RIGT == align) x_shift =  max_width - width;

  tl_x += x_shift;
  tl_y += y_shift;
  br_x += x_shift;
  br_y += y_shift;
}

//! Return refspec file name for \a product
/*! This contains the logic to map a \a product name to a suitable
 *  file in the filesystem that holds device specific information.
 *
 *  \todo Add a product to file name dictionary to accommodate any
 *        product names that are troublesome as file system names.
 *        This dictionary should be maintained in a file itself.
 */
static fs::path
map_(std::string product)
{
  run_time rt;

  if ("PID 08BC" == product) product = "PX-M7050";
  if ("PID 08CC" == product) product = "PX-M7050FX";
  if ("PID 08CE" == product) product = "PX-M860F";

  if (rt.running_in_place ())
    product.insert (0, "data/");

  product.insert (0, "drivers/esci/");
  return rt.data_file (run_time::pkg, product + ".dat");
}

bool
compound_scanner::get_file_defs_(const std::string& product)
{
  log::trace ("trying to get device information from file");

  if (product.empty ())
    {
      log::error ("product name unknown");
      return false;
    }

  fs::path refspec (map_(product));

  if (!fs::exists (refspec))
    {
      log::trace ("file not found: %1%") % refspec;
      return false;
    }

  fs::basic_ifstream< byte > fs (refspec, ios_base::binary | ios_base::in);

  if (!fs.is_open ())
    {
      log::error ("cannot read %1%") % refspec;
      return false;
    }

  // Based on code from verify.cpp

  information  info;
  capabilities caps;
  capabilities caps_flip;
  parameters   parm;
  parameters   parm_flip;

  byte_buffer blk;
  header      hdr;

  try
    {
      decoding::grammar           gram;
      decoding::grammar::iterator head;
      decoding::grammar::iterator tail;

      while (fs::basic_ifstream< byte >::traits_type::eof ()
             != fs.peek ())
        {
          const streamsize hdr_len = 12;
          blk.resize (hdr_len);
          fs.read (blk.data (), hdr_len);

          head = blk.begin ();
          tail = head + hdr_len;

          if (!gram.header_(head, tail, hdr))
            {
              log::error ("%1%") % gram.trace ();
              fs.close ();
              return false;
            }

          log::trace ("%1%: %2% byte payload")
            % str (hdr.code)
            % hdr.size
            ;

          if (0 == hdr.size) continue;

          blk.resize (hdr.size);
          fs.read (blk.data (), hdr.size);

          head = blk.begin ();
          tail = head + hdr.size;

          bool result = false;
          namespace reply = code_token::reply;
          /**/ if (reply::INFO == hdr.code)
            {
              result = gram.information_(head, tail, info);
            }
          else if (reply::CAPA == hdr.code)
            {
              result = gram.capabilities_(head, tail, caps);
            }
          else if (reply::CAPB == hdr.code)
            {
              result = gram.capabilities_(head, tail, caps_flip);
            }
          else if (reply::RESA == hdr.code)
            {
              result = gram.scan_parameters_(head, tail, parm);
            }
          else if (reply::RESB == hdr.code)
            {
              result = gram.scan_parameters_(head, tail, parm_flip);
            }

          if (!result)
            {
              log::error ("%1%") % gram.trace ();
              fs.close ();
              return false;
            }
        }
    }
  catch (decoding::grammar::expectation_failure& e)
    {
      std::stringstream ss;
      ss << "\n  " << e.what () << " @ "
         << std::hex << e.first - blk.begin ()
         << "\n  Looking at " << str (hdr.code)
         << "\n  Processing " << std::string (blk.begin (), blk.end ())
         << "\n  Expecting " << e.what_
         << "\n  Got: " << std::string (e.first, e.last);
      log::error (ss.str ());
      return false;
    }

  // Data files may modify product and version information for other
  // purposes.  Make sure to keep values obtained from the device.
  info.product = info_.product;
  info.version = info_.version;
  if (info_ != info)
    {
      log::brief ("using device information from file");
      const_cast< information& > (info_) = info;
    }
  if (caps_ != caps)
    {
      log::brief ("using device capabilities from file");
      const_cast< capabilities& > (caps_) = caps;
    }
  if (caps_flip_ != caps_flip)
    {
      log::brief ("using device flip-side capabilities from file");
      const_cast< capabilities& > (caps_flip_) = caps_flip;
    }

  const_cast< parameters&   > (defs_     ) = parm;
  const_cast< parameters&   > (defs_flip_) = parm_flip;

  return true;
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
        toggle speed = false;
        if (val_.count ("speed"))
          speed = val_.at ("speed");

        return (speed ? context::GRAY8 : context::MONO);
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

static system_error::error_code
token_to_error_code (const quad& what)
{
  using namespace code_token::reply::info;

  /**/ if (err::OPN  == what) return system_error::cover_open;
  else if (err::PE   == what) return system_error::media_out;
  else if (err::PJ   == what) return system_error::media_jam;
  else if (err::AUTH == what) return system_error::permission_denied;
  else if (err::PERM == what) return system_error::permission_denied;

  return system_error::unknown_error;
}

static system_error::error_code
token_to_error_code (const std::vector< status::error >& error)
{
  using namespace code_token::reply::info;

  std::vector< status::error >::const_iterator it;

  for (it = error.begin (); error.end () != it; ++it)
    {
      if (err::AUTH == it->what) return token_to_error_code (it->what);
    }
  for (it = error.begin (); error.end () != it; ++it)
    {
      if (err::PERM == it->what) return token_to_error_code (it->what);
    }
  for (it = error.begin (); error.end () != it; ++it)
    {
      if (err::PE   != it->what) return token_to_error_code (it->what);
    }

  return (!error.empty ()
          ? token_to_error_code (error.begin ()->what)
          : token_to_error_code (quad ()));
}

// Helper functions for create_message()
static std::string create_adf_message (const quad& what);
static std::string create_fb_message (const quad& what);
static std::string create_tpu_message (const quad& what);

// A message for when all else fails or when differentiating by part
// doesn't seem to make sense
static std::string
fallback_message (const quad& part, const quad& what)
{
  using namespace code_token::reply::info;

  if (err::AUTH == what || err::PERM == what)
    return _("Authentication is required.\n"
             "Unfortunately, this version of the driver does not support "
             "authentication yet.");

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
  BOOST_STATIC_ASSERT ((err::DTCL == info::err::DTCL));
  // status::err does not have err::AUTH
  // status::err does not have err::PERM
  BOOST_STATIC_ASSERT ((err::BTLO == info::err::BTLO));

  if (err::ADF == part) return create_adf_message (what);
  if (err::FB  == part) return create_fb_message (what);
  if (err::TPU == part) return create_tpu_message (what);

  return fallback_message (part, what);
}

static std::string
create_message (const std::vector< status::error >& error)
{
  using namespace code_token::reply::info;

  std::string rv;
  std::vector< status::error >::const_iterator it;

  for (it = error.begin (); error.end () != it; ++it)
    {
      if (err::AUTH == it->what)
        {
          if (!rv.empty ()) rv += "\n";
          rv += create_message (it->part, it->what);
        }
    }
  for (it = error.begin (); error.end () != it; ++it)
    {
      if (err::PERM == it->what)
        {
          if (!rv.empty ()) rv += "\n";
          rv += create_message (it->part, it->what);
        }
    }
  for (it = error.begin (); error.end () != it; ++it)
    {
      if (   err::AUTH != it->what
          && err::PERM != it->what
          && err::PE   != it->what)
        {
          if (!rv.empty ()) rv += "\n";
          rv += create_message (it->part, it->what);
        }
    }
  for (it = error.begin (); error.end () != it; ++it)
    {
      if (err::PE == it->what)
        {
          if (!rv.empty ()) rv += "\n";
          rv += create_message (it->part, it->what);
        }
    }

  return rv;
}

/*! \warn  The message strings are used by the SANE backend to map some
 *         exceptions to SANE_Status values as a fallback for the cases
 *         where it doesn't recognize our system_error exceptions.
 */
static std::string
create_adf_message (const quad& what)
{
  using namespace code_token::reply::info;

  if (err::OPN  == what)
    return _("The Automatic Document Feeder is open.\n"
             "Please close it.");
  if (err::PJ   == what)
    return _("A paper jam occurred.\n"
             "Open the Automatic Document Feeder and remove any paper.\n"
             "If there are any documents loaded in the ADF, remove them"
             " and load them again.");
  if (err::PE   == what)
    return _("Please load the document(s) into the Automatic Document Feeder.");
  if (err::DFED == what)
    return _("A multi page feed occurred in the auto document feeder. "
             "Open the cover, remove the documents, and then try again."
             " If documents remain on the tray, remove them and then"
             " reload them.");
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
  using namespace code_token::reply::info;

  if (err::ERR  == what)
    return _("A fatal error has occurred");

  return fallback_message (err::FB, what);
}

static std::string
create_tpu_message (const quad& what)
{
  using namespace code_token::reply::info;

  return fallback_message (err::TPU, what);
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
