//  array.hpp -- PDF array objects
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

#ifndef filters_pdf_array_hpp_
#define filters_pdf_array_hpp_

#include <ostream>
#include <vector>

#include "object.hpp"
#include "primitive.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

/*! Defines a PDF array object [p 58].
 */
class array : public object
{
private:
  typedef std::vector<object *>      store_type;
  typedef store_type::iterator       store_iter;
  typedef store_type::const_iterator store_citer;

  store_type _store;
  store_type _mine;

public:
  virtual ~array ();

  /*! Insert an object at the end the array
   */
  void insert (object* obj);

  void insert (primitive obj);
  void insert (object obj);

  /*! Count the number of objects in the array
   */
  size_t size() const;

  /*! Obtain a reference to an object at a given index
   */
  const object* operator[] (size_t index) const;

  void operator>> (std::ostream& os) const;
};

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pdf_array_hpp_ */
