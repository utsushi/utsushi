//  object.hpp -- PDF objects
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

#ifndef filters_pdf_object_hpp_
#define filters_pdf_object_hpp_

#include <cstdio>
#include <cstdlib>
#include <ostream>

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

using namespace std;

/*! A base class for all pdf objects [p 51].
 *
 * A _pdf_::object is also used to pass around object numbers in a transparent
 * fashion so that object references can be output correctly as elements of
 * arrays and dictionaries.
 */
class object
{
private:

  size_t _obj_num;
  static size_t next_obj_num; // the next free object number

public:

  object ();

  /*! Creates a new _pdf_::object with its object number set to \a num.
   *
   *  Constructs an indirect object.
   */
  object (size_t num);

  virtual ~object (void);

  /*! Obtain the _pdf_::object's object number.
   *
   * If the object has not been allocated an object number yet, a new one is
   * allocated and returned.
   */
  size_t obj_num ();

  /*! Determine whether the object is direct or indirect [p 63].
   *
   *  Probably don't need this method.
   *
   *  @return True if the object is direct, False if it is indirect
   */
  bool is_direct () const;

  /*! Output the object contents.
   *
   *  In the case of _pdf_::object, this only outputs an indirect reference to
   *  itself [p 64].
   *
   *  Each subclass re-implements this in order to print its own content.
   *  It should only ever output the object contents, ommitting the object
   *  definition header and footer [p 64]
   */
  virtual void operator>> (std::ostream& os) const;

  /*! Compare the contents of two _pdf_::objects.
   *
   * Only the object contents are compared, the object number of the two
   * objects can be different.
   *
   * In the case of _pdf_::object, the object numbers are compared.
   */
  virtual bool operator== (object& that) const;

  /*! Reset the current object number to recycle them for new documents
   */
  static void reset_object_numbers ();

};

std::ostream&
operator<< (std::ostream& os, const object& o);

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pdf_object_hpp_ */
