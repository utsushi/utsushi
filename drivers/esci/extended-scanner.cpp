//  extended-scanner.cpp -- devices that handle extended commands
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
#include "get-scan-parameters.hpp"
#include "set-color-matrix.hpp"
#include "set-gamma-table.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

using std::cerr;
using std::clog;
using std::endl;

inline static
quantity::integer_type
int_cast (const uint32_t& i)
{
  return boost::numeric_cast< quantity::integer_type > (i);
}

extended_scanner::extended_scanner (const connexion::ptr& cnx)
  : scanner (cnx)
  , capability_(true)
  , dev_status_(true)
  , acquire_(true)
  , src_(NO_SOURCE)
  , area_(point<uint32_t> ())
{
  get_scan_parameters FS_S (true);

  *cnx_ << capability_ << dev_status_ << FS_S;

  // log::trace ("%1%") % capability_;
  // log::trace ("%1%") % dev_status_;
  // log::trace ("%1%") % FS_S;
}

void
extended_scanner::configure ()
{
  {
    add_options ()
      ("resolution", (from< range > ()
                      -> lower (int_cast (capability_.min_resolution ()))
                      -> upper (int_cast (capability_.max_resolution ()))
                      -> default_value
                      (quantity (int_cast (capability_.min_resolution ())))
                      ),
       attributes (tag::general),
       N_("Resolution")
       );
  }
  {

    // bbox = (capability_.scan_area () / capability_.base_resolution ());
    point<quantity> tl (int_cast (capability_.scan_area ().top_left (). x ()),
                        int_cast (capability_.scan_area ().top_left (). y ()));
    point<quantity> br (int_cast (capability_.scan_area ().bottom_right (). x ()),
                        int_cast (capability_.scan_area ().bottom_right (). y ()));
    bounding_box<quantity> bbox (tl / int_cast (capability_.base_resolution ()),
                                 br / int_cast (capability_.base_resolution ()));

    add_options ()
      ("tl-x", (from< range > ()
                -> offset (bbox.offset ().x ())
                -> extent (bbox.width ())
                -> default_value (bbox.top_left ().x ())
                ),
       attributes (tag::geometry),
       N_("Top Left X")
       )
      ("br-x", (from< range > ()
                -> offset (bbox.offset ().x ())
                -> extent (bbox.width ())
                -> default_value (bbox.bottom_right ().x ())
                ),
       attributes (tag::geometry),
       N_("Bottom Right X")
       )
      ("tl-y", (from< range > ()
                -> offset (bbox.offset ().y ())
                -> extent (bbox.height ())
                -> default_value (bbox.top_left ().y ())
                ),
       attributes (tag::geometry),
       N_("Top Left Y")
       )
      ("br-y", (from< range > ()
                -> offset (bbox.offset ().y ())
                -> extent (bbox.height ())
                -> default_value (bbox.bottom_right ().y ())
                ),
       attributes (tag::geometry),
       N_("Bottom Right Y")
       );
  }
  {
    add_options ()
      ("mode", (from< store > ()
                -> alternative (N_("Line Art"))
                -> alternative (N_("B/W"))
                -> default_value (N_("Color"))
                ),
       attributes (tag::general),
       N_("Color Mode")
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

  int count (0);                // document sources
  if (capability_.is_flatbed_type ())         ++count;
  if (dev_status_.adf_detected ())            ++count;
  if (capability_.adf_is_duplex_type ())      ++count;
  if (dev_status_.tpu_detected (TPU1)) ++count;
  if (dev_status_.tpu_detected (TPU2)) ++count;

  if (1 < count)                // multiple sources
    {
      store s;

      if (capability_.is_flatbed_type ())
        s.alternative (N_("Flatbed"));
      if (dev_status_.adf_detected ())
        {
          if (capability_.adf_is_duplex_type ())
            {
              s.alternative (N_("ADF Simplex"));
              s.alternative (N_("ADF Duplex"));
            }
          else
            {
              s.alternative (N_("ADF"));
            }
        }
      if (dev_status_.tpu_detected (TPU1)
          || dev_status_.tpu_detected (TPU2))
        {
          if (dev_status_.tpu_detected (TPU1)
              && dev_status_.tpu_detected (TPU2))
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
         attributes (tag::general),
         N_("Document Source")
         );
    }
  else
    {
      if (!src_ && capability_.is_flatbed_type ())        src_ = MAIN;
      if (!src_ && dev_status_.adf_detected ())           src_ = ADF;
      if (!src_ && dev_status_.tpu_detected (TPU1)) src_ = TPU1;
      if (!src_ && dev_status_.tpu_detected (TPU2)) src_ = TPU2;
    }
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
extended_scanner::set_up_sequence (void)
{
  source_value src = src_;
  bool do_duplex = false;
  if (!src)                     // multiple document sources
    {
      const string& s = *values_["doc-source"];

      if (!src && s == "Flatbed") src = MAIN;
      if (!src && s == "ADF"    ) src = ADF;
      if (!src && s == "ADF Simplex") src = ADF;
      if (!src && s == "ADF Duplex" )
        {
          src = ADF;
          do_duplex = true;
        }
      if (!src && s == "TPU"
          && (dev_status_.tpu_detected (TPU1))) src = TPU1;
      if (!src && s == "TPU"
          && (dev_status_.tpu_detected (TPU2))) src = TPU2;
      if (!src && s == "Primary TPU"  ) src = TPU1;
      if (!src && s == "Secondary TPU") src = TPU2;
    }
  option_value opt;

  /**/ if (MAIN == src)
    {
      opt = MAIN_BODY;
    }
  else if (ADF == src)
    {
      opt = (do_duplex
             ? ADF_DUPLEX
             : ADF_SIMPLEX);
    }
  else if (TPU1 == src)
    {
      opt = TPU_AREA_1;
    }
  else if (TPU2 == src)
    {
      opt = TPU_AREA_2;
    }
  else
    {
      BOOST_THROW_EXCEPTION (logic_error ("unsupported scan source"));
    }

  const string& mode = *values_["mode"];
  color_mode_ = (mode == "Color"
                 ? PIXEL_RGB
                 : MONOCHROME);
  bit_depth_  = (mode == "Line Art"
                 ? 1 : 8);
  quantity lc = *values_["line-count"];
  line_count_ = lc.amount< uint8_t > ();

  quantity res = *values_["resolution"]; // pixels/inch

  quantity tl_x = *values_["tl-x"];      // inches
  quantity tl_y = *values_["tl-y"];
  quantity br_x = *values_["br-x"];
  quantity br_y = *values_["br-y"];

  tl_x *= res;                  // pixels
  tl_y *= res;
  br_x *= res;
  br_y *= res;

  point<uint32_t> tl (tl_x.amount< uint32_t > (), tl_y.amount< uint32_t > ());
  point<uint32_t> br (br_x.amount< uint32_t > (), br_y.amount< uint32_t > ());

  if (capability_.product_name () == "ES-H300")
    {
      uint32_t boundary = (1 == bit_depth_
                           ? 32
                           : 4);
      br.x () += boundary - (br.x () - tl.x ()) % boundary;
    }

  area_ = bounding_box<uint32_t> (tl, br);

  toggle speed = *values_["speed"];
  scan_mode_value scan_mode = (speed
                                     ? HI_SPEED
                                     : NORMAL_SPEED);

  parameters_
    .gamma_correction (CUSTOM_GAMMA_B)
    .color_correction (USER_DEFINED)
    .resolution (res.amount< uint32_t > ())
    .scan_mode (scan_mode)
    .scan_area (area_)
    .color_mode (color_mode_)
    .bit_depth (bit_depth_)
    .option_unit (opt);

  set_gamma_table lut;
  set_color_matrix cm;
  // set_dither_pattern

  *cnx_ << lut () << cm () << parameters_ << dev_status_;

  {                             // divine a more optimal line count
    uint32_t bytes_per_line = area_.width ();

    bytes_per_line *= (mode == "Color" ? 3 : 1);

    if (8 < bit_depth_)
      {
        bytes_per_line *= 2;
      }
    else
      {
        bytes_per_line /= 8 / bit_depth_;
      }

    uint8_t min = std::numeric_limits< uint8_t >::min ();
    uint8_t max = (0 == line_count_
                   ? std::numeric_limits< uint8_t >::max ()
                   : line_count_);

    if (buffer_size_ / bytes_per_line < max)
      {
        max = buffer_size_ / bytes_per_line;
      }

    while (min != max)
      {
        uint8_t cur = (1 + min + max) / 2;

        try
          {
            *cnx_ << parameters_.line_count (cur);
          }
        catch (const invalid_parameter&)
          {
            max /= 2;
            cur = min;
          }
        min = cur;
      }

    if (line_count_ && line_count_ != parameters_.line_count ())
      log::error ("line-count: using %2% instead of %1%")
        % line_count_
        % parameters_.line_count ()
        ;
  }

  return true;
}

bool
extended_scanner::is_consecutive (void) const
{
  return dev_status_.adf_enabled ();
}

bool
extended_scanner::obtain_media (void)
{
  bool rv = true;

  if (is_consecutive ()
      && capability_.adf_is_page_type ())
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

  *cnx_ << dev_status_;
  rv = !(dev_status_.adf_media_out () || dev_status_.main_media_out ());

  return rv;
}

bool
extended_scanner::set_up_image (void)
{
  chunk_  = chunk ();
  offset_ = 0;

  // need to recompute the scan area when FS_F_.media_value() returns
  // non-zero values and the user has activated auto-scan-area

  *cnx_ << parameters_;

  ctx_ = context (area_.width (), area_.height (),
                  (PIXEL_RGB == color_mode_ ? 3 : 1),
                  bit_depth_);

  do
    {
      *cnx_ << dev_status_;
    }
  while (dev_status_.is_warming_up ());

  *cnx_ << acquire_;
  return !acquire_.detected_fatal_error ();
}

void
extended_scanner::finish_image (void)
{
  if (is_consecutive ()
      && !capability_.adf_is_page_type ())
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
  if (offset_ == chunk_.size ())
    {
      chunk_  = ++acquire_;
      offset_ = 0;
    }

  streamsize rv = std::min (chunk_.size () - offset_, n);

  traits::copy (data, reinterpret_cast<const octet *>
                (chunk_.get () + offset_), rv);
  offset_ += rv;

  return rv;
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
