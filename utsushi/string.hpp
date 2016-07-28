//  string.hpp -- bounded type for utsushi::value objects
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

#ifndef utsushi_string_hpp_
#define utsushi_string_hpp_

#include <iosfwd>
#include <string>

#include <boost/operators.hpp>

namespace utsushi {

class string
  : private boost::totally_ordered< string >
{
public:
  typedef std::string::size_type size_type;

  string (const std::string& s);
  string (const char *str);
  explicit string ();

  bool operator== (const string& s) const;
  bool operator<  (const string& s) const;

  string& operator= (const string& s);

  operator bool () const;
  operator std::string () const;

  const char * c_str () const;

  size_type copy (char *dest, size_type n, size_type pos = 0) const;

  //! Returns the number of \c char elements in a string
  /*! This is \e different from the std::string::size() specification,
   *  which has it return the number of characters (not \c char's) in
   *  the string.
   *
   *  \return  The equivalent of \c strlen(c_str()) .
   */
  size_type size () const;

  friend
  std::ostream& operator<< (std::ostream& os, const string& s);
  friend
  std::istream& operator>> (std::istream& is, string& s);

private:
  std::string string_;
};

}       // namespace utsushi

#endif  /* utsushi_string_hpp_ */
