//  extended-scanner.cpp -- devices that handle extended commands
//  Copyright (C) 2013  Olaf Meeuwissen
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

#include <boost/numeric/conversion/cast.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>
#include <utsushi/store.hpp>

#include "action.hpp"
#include "code-point.hpp"
#include "exception.hpp"
#include "extended-scanner.hpp"
#include "get-identity.hpp"
#include "set-color-matrix.hpp"
#include "set-gamma-table.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

inline static
quantity::integer_type
int_cast (const uint32_t& i)
{
  return boost::numeric_cast< quantity::integer_type > (i);
}

extended_scanner::extended_scanner (const connexion::ptr& cnx)
  : scanner (cnx)
  , caps_(true)
  , defs_(true)
  , acquire_(true)
  , stat_(true)
  , cancelled_(false)
{
  *cnx_ << const_cast< get_extended_identity& > (caps_)
        << const_cast< get_scan_parameters&   > (defs_)
        << stat_;
}

void
extended_scanner::configure ()
{
  {
    add_options ()
      ("resolution", (from< range > ()
                      -> lower (int_cast (caps_.min_resolution ()))
                      -> upper (int_cast (caps_.max_resolution ()))
                      -> default_value
                      (quantity (int_cast (caps_.min_resolution ())))
                      ),
       attributes (tag::general)(level::standard),
       N_("Resolution")
       );
  }
  {
    // bbox = (caps_.scan_area () / caps_.base_resolution ());
    point<quantity> tl (1. * caps_.scan_area ().top_left (). x (),
                        1. * caps_.scan_area ().top_left (). y ());
    point<quantity> br (1. * caps_.scan_area ().bottom_right (). x (),
                        1. * caps_.scan_area ().bottom_right (). y ());
    bounding_box<quantity> bbox (tl / (1. * caps_.base_resolution ()),
                                 br / (1. * caps_.base_resolution ()));

    add_options ()
      ("tl-x", (from< range > ()
                -> offset (bbox.offset ().x ())
                -> extent (bbox.width ())
                -> default_value (bbox.top_left ().x ())
                ),
       attributes (tag::geometry)(level::standard),
       N_("Top Left X")
       )
      ("br-x", (from< range > ()
                -> offset (bbox.offset ().x ())
                -> extent (bbox.width ())
                -> default_value (bbox.bottom_right ().x ())
                ),
       attributes (tag::geometry)(level::standard),
       N_("Bottom Right X")
       )
      ("tl-y", (from< range > ()
                -> offset (bbox.offset ().y ())
                -> extent (bbox.height ())
                -> default_value (bbox.top_left ().y ())
                ),
       attributes (tag::geometry)(level::standard),
       N_("Top Left Y")
       )
      ("br-y", (from< range > ()
                -> offset (bbox.offset ().y ())
                -> extent (bbox.height ())
                -> default_value (bbox.bottom_right ().y ())
                ),
       attributes (tag::geometry)(level::standard),
       N_("Bottom Right Y")
       );
  }
  {
    add_options ()
      ("image-type", (from< store > ()
                      -> alternative (N_("Gray (1 bit)"))
                      -> alternative (N_("Gray (8 bit)"))
                      -> default_value (N_("Color (8 bit)"))
                ),
       attributes (tag::general)(level::standard),
       N_("Image Type")
       )
      ("speed", toggle (false),
       attributes (),
       N_("Speed")
       )
      ("line-count", (from< range > ()
                      -> lower (0)
                      -> upper (255)
                      -> default_value (1)),
       attributes (),
       N_("Line Count"),
       N_("Specify how many scan lines to move from the device to the "
          "software in one transfer.  Note that 0 will use the maximum "
          "usable value.  Values larger than the maximum usable value "
          "are clamped to the maximum.")
       );
  }

  {
    store s;

    if (caps_.is_flatbed_type ())
      s.alternative (N_("Flatbed"));
    if (stat_.adf_detected ())
      {
        if (caps_.adf_is_duplex_type ())
          {
            s.alternative (N_("ADF Simplex"));
            s.alternative (N_("ADF Duplex"));
          }
        else
          {
            s.alternative (N_("ADF"));
          }
      }
    if (stat_.tpu_detected (TPU1)
        || stat_.tpu_detected (TPU2))
      {
        if (stat_.tpu_detected (TPU1)
            && stat_.tpu_detected (TPU2))
          {
            s.alternative (N_("Primary TPU"));
            s.alternative (N_("Secondary TPU"));
          }
        else
          {
            s.alternative (N_("TPU"));
          }
      }
    add_options ()
      ("doc-source", (from< store > (s)
                      -> default_value (s.front ())
                      ),
       attributes (tag::general)(level::standard),
       N_("Document Source")
       );
  }

  //! \todo Remove transfer-format work-around for scan-cli utility
  add_options ()
    ("transfer-format", (from< store > ()
                         ->default_value ("RAW")
                         ),
     attributes (level::standard),
     N_("Transfer Format")
     )
    ;
}

bool
extended_scanner::is_single_image () const
{
  bool result = true;
  try
    {
      const string& s = *values_["doc-source"];

      result = !(s == "ADF" || s == "ADF Simplex" || s == "ADF Duplex");
    }
  catch (const std::out_of_range&)
    {}
  return result;
}

bool
extended_scanner::is_consecutive () const
{
  return stat_.adf_enabled ();
}

bool
extended_scanner::obtain_media ()
{
  bool rv = true;

  if (is_consecutive ()
      && caps_.adf_is_page_type ())
    {
      try
        {
          load_media load;
          *cnx_ << load;
        }
      catch (const invalid_command& e)
        {
          log::alert (e.what ());
        }
      catch (const unknown_reply& e)
        {
          log::alert (e.what ());
          rv = false;
        }
    }

  *cnx_ << stat_;
  rv = !(stat_.adf_media_out () || stat_.main_media_out ());

  return rv;
}

bool
extended_scanner::set_up_image ()
{
  chunk_  = chunk ();
  offset_ = 0;

  if (cancelled_) return false;

  // need to recompute the scan area when FS_F_.media_value() returns
  // non-zero values and the user has activated auto-scan-area

  if (!set_up_hardware ())
    {
      return false;
    }

  ctx_ = context (parm_.scan_area ().width (), parm_.scan_area ().height (),
                  (PIXEL_RGB == parm_.color_mode () ? 3 : 1),
                  parm_.bit_depth ());

  do
    {
      *cnx_ << stat_;
    }
  while (stat_.is_warming_up ());

  *cnx_ << acquire_;
  return !acquire_.detected_fatal_error ();
}

void
extended_scanner::finish_image ()
{
  if (is_consecutive ()
      && !caps_.adf_is_page_type ())
    {
      try
        {
          eject_media eject;
          *cnx_ << eject;
        }
      catch (const invalid_command& e)
        {
          log::alert (e.what ());
        }
      catch (const unknown_reply& e)
        {
          log::alert (e.what ());
        }
    }
}

streamsize
extended_scanner::sgetn (octet *data, streamsize n)
{
  bool do_cancel = cancel_requested ();

  if (offset_ == chunk_.size ())
    {
      if (do_cancel) acquire_.cancel ();

      chunk_  = ++acquire_;
      offset_ = 0;

      cancelled_ = (!chunk_
                    && (do_cancel || acquire_.is_cancel_requested ()));
      if (cancelled_)
        {
          cancel ();            // notify idevice::read()
          return traits::eof ();
        }
    }

  streamsize rv = std::min (chunk_.size () - offset_, n);

  traits::copy (data, reinterpret_cast<const octet *>
                (chunk_.get () + offset_), rv);
  offset_ += rv;

  return rv;
}

void
extended_scanner::set_up_initialize ()
{
  parm_ = defs_;

  cancelled_ = false;
}

bool
extended_scanner::set_up_hardware ()
{
  try
    {
      *cnx_ << parm_;
      if (read_back_)
        {
          get_scan_parameters parm;
          *cnx_ << parm;
          if (parm != parm_)
            log::alert ("scan parameters may not be set as requested");
        }
    }
  catch (const invalid_parameter& e)
    {
      log::alert (e.what ());
      return false;
    }
  return true;
}

void
extended_scanner::set_up_color_matrices ()
{
  parm_.color_correction (USER_DEFINED);

  set_color_matrix cm;
  *cnx_ << cm ();
}

void
extended_scanner::set_up_dithering ()
{
}

void
extended_scanner::set_up_doc_source ()
{
  if (!val_.count ("doc-source")) return;

  bool do_duplex = false;
  source_value src = NO_SOURCE;

  const string& s = val_["doc-source"];

  if (!src && s == "Flatbed") src = MAIN;
  if (!src && s == "ADF"    ) src = ADF;
  if (!src && s == "ADF Simplex") src = ADF;
  if (!src && s == "ADF Duplex" )
    {
      src = ADF;
      do_duplex = true;
    }
  if (!src && s == "TPU"
      && (stat_.tpu_detected (TPU1))) src = TPU1;
  if (!src && s == "TPU"
      && (stat_.tpu_detected (TPU2))) src = TPU2;
  if (!src && s == "Primary TPU"  ) src = TPU1;
  if (!src && s == "Secondary TPU") src = TPU2;

  /**/ if (MAIN == src)
    {
      parm_.option_unit (MAIN_BODY);
    }
  else if (ADF == src)
    {
      parm_.option_unit (do_duplex
                         ? ADF_DUPLEX
                         : ADF_SIMPLEX);
    }
  else if (TPU1 == src)
    {
      parm_.option_unit (TPU_AREA_1);
    }
  else if (TPU2 == src)
    {
      parm_.option_unit (TPU_AREA_2);
    }
  else
    {
      BOOST_THROW_EXCEPTION (logic_error ("unsupported scan source"));
    }
}

void
extended_scanner::set_up_gamma_tables ()
{
  parm_.gamma_correction (CUSTOM_GAMMA_B);

  set_gamma_table lut;
  *cnx_ << lut ();
}

void
extended_scanner::set_up_image_mode ()
{
  if (!val_.count ("image-type")) return;

  const string& mode = val_["image-type"];
  parm_.color_mode (mode == "Color (8 bit)"
                    ? PIXEL_RGB
                    : MONOCHROME);
  parm_.bit_depth (mode == "Gray (1 bit)"
                   ? 1 : 8);
}

void
extended_scanner::set_up_mirroring ()
{
}

void
extended_scanner::set_up_resolution ()
{
  quantity res = val_["resolution"]; // pixels/inch
  parm_.resolution (res.amount< uint32_t > ());
}

void
extended_scanner::set_up_scan_area ()
{
  quantity tl_x = val_["tl-x"];      // inches
  quantity tl_y = val_["tl-y"];
  quantity br_x = val_["br-x"];
  quantity br_y = val_["br-y"];

  if (br_x < tl_x) swap (tl_x, br_x);
  if (br_y < tl_y) swap (tl_y, br_y);

  tl_x *= 1. * parm_.resolution ().x ();      // pixels
  tl_y *= 1. * parm_.resolution ().y ();
  br_x *= 1. * parm_.resolution ().x ();
  br_y *= 1. * parm_.resolution ().y ();

  point<uint32_t> tl (tl_x.amount< uint32_t > (), tl_y.amount< uint32_t > ());
  point<uint32_t> br (br_x.amount< uint32_t > (), br_y.amount< uint32_t > ());

  if (caps_.product_name () == "ES-H300")
    {
      uint32_t boundary = (1 == parm_.bit_depth ()
                           ? 32
                           : 4);
      br.x () += boundary - (br.x () - tl.x ()) % boundary;
    }

  parm_.scan_area (tl, br);
}

void
extended_scanner::set_up_scan_count ()
{
}

void
extended_scanner::set_up_scan_speed ()
{
  if (!val_.count ("speed")) return;

  toggle speed = val_["speed"];
  parm_.scan_mode (speed ? HI_SPEED : NORMAL_SPEED);
}

void
extended_scanner::set_up_sharpness ()
{
}

void
extended_scanner::set_up_threshold ()
{
}

void
extended_scanner::set_up_transfer_size ()
{
  if (!val_.count ("line-count")) return;

  quantity lc = val_["line-count"];
  parm_.line_count (lc.amount< uint8_t > ());

  {                             // divine a more optimal line count
    uint32_t bytes_per_line = parm_.scan_area ().width ();

    bytes_per_line *= (parm_.color_mode () == PIXEL_RGB ? 3 : 1);

    if (8 < parm_.bit_depth ())
      {
        bytes_per_line *= 2;
      }
    else
      {
        bytes_per_line /= 8 / parm_.bit_depth ();
      }

    uint8_t lc = parm_.line_count ();
    uint8_t min = std::numeric_limits< uint8_t >::min ();
    uint8_t max = (0 == lc
                   ? std::numeric_limits< uint8_t >::max ()
                   : lc);

    if (buffer_size_ / bytes_per_line < max)
      {
        max = buffer_size_ / bytes_per_line;
      }

    while (min != max)
      {
        uint8_t cur = (1 + min + max) / 2;

        try
          {
            *cnx_ << parm_.line_count (cur);
          }
        catch (const invalid_parameter&)
          {
            max /= 2;
            cur = min;
          }
        min = cur;
      }

    if (lc && lc != parm_.line_count ())
      log::error ("line-count: using %2% instead of %1%")
        % lc
        % parm_.line_count ()
        ;
  }
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
