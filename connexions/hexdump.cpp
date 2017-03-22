//  hexdump.cpp -- connexion transmissions onto another stream
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2009  Olaf Meeuwissen
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

#include <iomanip>
#include <sstream>
#include <string>

#include "hexdump.hpp"

namespace utsushi {
namespace _cnx_ {

extern "C" {

  void
  libcnx_hexdump_LTX_factory (connexion::ptr& cnx)
  {
    cnx = make_shared< hexdump > (cnx);
  }
}

using namespace std;

void
hexdump::send (const octet *message, streamsize size)
{
  hexdump_(message, size, ">>");
  instance_->send (message, size);
}

void
hexdump::recv (octet *message, streamsize size)
{
  instance_->recv (message, size);
  hexdump_(message, size, "<<");
}

void
hexdump::hexdump_(const octet *buf, streamsize sz,
                  const std::string& io)
{
  const streamsize quad_length = 4;
  const streamsize quad_count  = 4;
  const streamsize line_length = quad_length * quad_count;

  streamsize i = 0;
  while (i < sz)
    {
      basic_stringstream< traits::char_type > asc_dump;
      basic_stringstream< traits::char_type > hex_dump;

      asc_dump.imbue (locale::classic ());
      hex_dump.imbue (locale::classic ());
      hex_dump.fill ('0');

      streamsize j = 0;
      while (i < sz && j < line_length)
        {
          traits::char_type c = buf[i];
          asc_dump << (isprint (c, locale::classic ()) ? c : '.');
          hex_dump << " " << setw (2) << hex << traits::to_int_type (buf[i]);
          ++i, ++j;
          if (0 == j % quad_length && j != line_length)
            hex_dump << " ";
        }
      while (0 != j % line_length)
        {
          asc_dump << " ";
          hex_dump << "   ";
          ++i, ++j;
          if (0 == j % quad_length && j != line_length)
            hex_dump << " ";
        }
      os_ << setw (8) << setfill ('0') << hex << i - line_length
          << io << " " << hex_dump.str ()
          << "  |" << asc_dump.str () << "|\n";
    }
}

}       // namespace _cnx_
}       // namespace utsushi
