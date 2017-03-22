//  hexdump.hpp -- connexion transmissions onto another stream
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

#ifndef connexions_hexdump_hpp_
#define connexions_hexdump_hpp_

#include <iostream>

#include <utsushi/connexion.hpp>

namespace utsushi {

extern "C" {
  void libcnx_hexdump_LTX_factory (connexion::ptr& cnx);
}

namespace _cnx_ {

class hexdump
  : public decorator< connexion >
{
public:
  hexdump (connexion::ptr instance, std::ostream& os = std::cerr)
    : base_(instance), os_(os)
  {}

  virtual void send (const octet *message, streamsize size);
  virtual void recv (      octet *message, streamsize size);

protected:
  void hexdump_(const octet *message, streamsize size,
                const std::string& io);

  std::ostream&  os_;
};

} // namespace _cnx_
} // namespace utsushi

#endif  /* connexions_hexdump_hpp_ */
