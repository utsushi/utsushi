//  set-scan-parameters.cpp -- for the next scan
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008, 2013  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
//  Author : Olaf Meeuwissen
//  Origin : FreeRISCI
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

#include <boost/static_assert.hpp>

#include "set-scan-parameters.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    set_scan_parameters::set_scan_parameters ()
      : setter<FS,UPPER_W,64> ()
      , scan_parameters (dat_)
    {}

    set_scan_parameters::set_scan_parameters (const set_scan_parameters& s)
      : setter<FS,UPPER_W,64> (s)
      , scan_parameters (dat_)
    {}

    set_scan_parameters&
    set_scan_parameters::operator= (const set_scan_parameters& s)
    {
      if (this != &s)
        {
          rep_ = s.rep_;
          traits::copy (dat_, s.dat_, sizeof (dat_) / sizeof (*dat_));
        }
      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::operator= (const get_scan_parameters& s)
    {
      BOOST_STATIC_ASSERT ((   sizeof (dat_)   / sizeof (*dat_)
                            == sizeof (s.blk_) / sizeof (*s.blk_)));

      rep_ = 0;
      traits::copy (dat_, s.blk_, sizeof (dat_) / sizeof (*dat_));
      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::resolution (uint32_t r_x, uint32_t r_y)
    {
      rep_ = 0;

      from_uint32_t (dat_ + 0, r_x);
      from_uint32_t (dat_ + 4, r_y);

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::scan_area (bounding_box<uint32_t> a)
    {
      rep_ = 0;

      from_uint32_t (dat_ +  8, a.offset (). x());
      from_uint32_t (dat_ + 12, a.offset (). y());
      from_uint32_t (dat_ + 16, a.width ());
      from_uint32_t (dat_ + 20, a.height ());

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::color_mode (byte mode)
    {
      rep_ = 0;
      dat_[24] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::line_count (uint8_t value)
    {
      rep_ = 0;
      dat_[28] = value;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::bit_depth (uint8_t value)
    {
      rep_ = 0;
      dat_[25] = value;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::scan_mode (byte mode)
    {
      rep_ = 0;
      dat_[27] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::option_unit (byte mode)
    {
      rep_ = 0;
      dat_[26] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::film_type (byte type)
    {
      rep_ = 0;
      dat_[37] = type;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::mirroring (bool active)
    {
      rep_ = 0;
      dat_[36] = active;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::auto_area_segmentation (bool active)
    {
      rep_ = 0;
      dat_[34] = active;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::threshold (uint8_t value)
    {
      rep_ = 0;
      dat_[33] = value;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::halftone_processing (byte mode)
    {
      rep_ = 0;
      dat_[32] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::sharpness (int8_t value)
    {
      rep_ = 0;
      dat_[35] = value;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::brightness (int8_t value)
    {
      rep_ = 0;
      dat_[30] = value;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::gamma_correction (byte mode)
    {
      rep_ = 0;
      dat_[29] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::color_correction (byte mode)
    {
      rep_ = 0;
      dat_[31] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::main_lamp_lighting_mode (byte mode)
    {
      rep_ = 0;
      dat_[38] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::double_feed_sensitivity (byte mode)
    {
      rep_ = 0;
      dat_[39] = mode;

      return *this;
    }

    set_scan_parameters&
    set_scan_parameters::quiet_mode (byte mode)
    {
      rep_ = 0;
      dat_[41] = mode;

      return *this;
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
