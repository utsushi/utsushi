//  key.hpp -- for use with settings and groups
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

#ifndef utsushi_key_hpp_
#define utsushi_key_hpp_

#include <string>

#include <boost/operators.hpp>

namespace utsushi {

class key
  : boost::totally_ordered< key
  , boost::dividable      < key
  > >
{
public:
  key (const std::string& s);
  key (const char *str);
  explicit key ();

  bool operator== (const key& k) const;
  bool operator<  (const key& k) const;

  key& operator/= (const key& k);

  operator bool () const;
  operator std::string () const;

private:
  std::string key_;

  static const std::string separator_;
};

}       // namespace utsushi

#endif  /* utsushi_key_hpp_ */
