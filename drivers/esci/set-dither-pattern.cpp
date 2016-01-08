//  set-dither-pattern.cpp -- diffuse banding in B/W images
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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

#include <boost/throw_exception.hpp>

#include "exception.hpp"
#include "set-dither-pattern.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    const byte set_dither_pattern::cmd_[2] = { ESC, LOWER_B };

    set_dither_pattern::set_dither_pattern (void)
      : rep_(0)
      , dat_(NULL)
      , dat_size_(0)
    {}

    set_dither_pattern::set_dither_pattern (const set_dither_pattern& s)
      : rep_(s.rep_)
      , dat_(NULL)
      , dat_size_(0)
    {
      if (s.dat_)
        {
          dat_ = new byte [s.dat_size ()];
          dat_size_ = s.dat_size ();
          traits::copy (dat_, s.dat_, s.dat_size ());
        }
    }

    set_dither_pattern::~set_dither_pattern (void)
    {
      delete [] dat_;
    }

    set_dither_pattern&
    set_dither_pattern::operator= (const set_dither_pattern& s)
    {
      if (this != &s)
        {
          rep_ = s.rep_;

          if (s.dat_)
            {
              if (dat_size_ < s.dat_size ())
                {
                  delete [] dat_;
                  dat_ = new byte [s.dat_size ()];
                  dat_size_ = s.dat_size ();
                }
              traits::copy (dat_, s.dat_, s.dat_size ());
            }
        }
      return *this;
    }

    void
    set_dither_pattern::operator>> (connexion& cnx)
    {
      cnx.send (cmd_, sizeof (cmd_) / sizeof (*cmd_));
      cnx.recv (&rep_, 1);

      this->validate_cmd_reply ();

      cnx.send (dat_, 2);
      cnx.send (dat_ + 2, dat_size () - 2);
      cnx.recv (&rep_, 1);

      this->validate_dat_reply ();
    }

    set_dither_pattern&
    set_dither_pattern::operator() (byte pattern)
    {
      matrix<uint8_t,4> m;

      /**/ if (CUSTOM_A == pattern) // 4x4 Bayer
        {
          m[0][0] = 248; m[0][1] = 120; m[0][2] = 216; m[0][3] =  88;
          m[1][0] =  56; m[1][1] = 184; m[1][2] =  24; m[1][3] = 152;
          m[2][0] = 200; m[2][1] =  72; m[2][2] = 232; m[2][3] = 104;
          m[3][0] =   8; m[3][1] = 136; m[3][2] =  40; m[3][3] = 168;
        }
      else if (CUSTOM_B == pattern) // 4x4 spiral
        {
          m[0][0] =  40; m[0][1] = 152; m[0][2] = 136; m[0][3] =  24;
          m[1][0] = 168; m[1][1] = 248; m[1][2] = 232; m[1][3] = 120;
          m[2][0] = 184; m[2][1] = 200; m[2][2] = 216; m[2][3] = 104;
          m[3][0] =  56; m[3][1] =  72; m[3][2] =  88; m[3][3] =   8;
        }
      else
        BOOST_THROW_EXCEPTION (range_error ("unknown default dither pattern"));

      return this->operator() (pattern, m);
    }

    streamsize
    set_dither_pattern::dat_size (void) const
    {
      return (dat_
              ? 2 + dat_[1] * dat_[1]
              : 0);
    }

    void
    set_dither_pattern::validate_cmd_reply (void) const
    {
      if (ACK == rep_)
        return;

      if (NAK == rep_)
        BOOST_THROW_EXCEPTION (invalid_command ());

      BOOST_THROW_EXCEPTION (unknown_reply ());
    }

    void
    set_dither_pattern::validate_dat_reply (void) const
    {
      if (ACK == rep_)
        return;

      if (NAK == rep_)
        BOOST_THROW_EXCEPTION (invalid_parameter ());

      BOOST_THROW_EXCEPTION (unknown_reply ());
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
