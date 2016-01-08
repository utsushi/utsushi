//  primitive.hpp -- PDF primitives
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

#ifndef filters_pdf_primitive_hpp_
#define filters_pdf_primitive_hpp_

#include <ostream>
#include <sstream>
#include <string>

#include "object.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

using std::string;
using std::stringstream;

/*! Defines a primitive pdf object: one of string, name, integer, or real.
 */
class primitive : public object
{
private:
  string _str;

public:
  primitive ();

  primitive (const char *str)
    : _str (str)
  {}

  primitive (const string& s)
    : _str (s)
  {}

  template< typename T >
  primitive (const T& t)
  {
    stringstream ss;
    ss << t;
    ss >> _str;
  }

  virtual ~primitive (void);

  virtual void operator>> (std::ostream& os) const;

  /*! Compare the contents of two _pdf_::primitives.
   *
   * Only the object contents are compared, the object number of the
   * two objects may differ.
   */
  virtual bool operator== (const primitive& other) const;

  void operator= (const primitive& that);
};

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pdf_primitive_hpp_ */
