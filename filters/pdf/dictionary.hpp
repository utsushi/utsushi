//  dictionary.hpp -- PDF dictionaries
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

#ifndef filters_pdf_dictionary_hpp_
#define filters_pdf_dictionary_hpp_

#include <map>
#include <ostream>

#include "object.hpp"
#include "primitive.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

/*! Defines a pdf dictionary object [p 59]
 */
class dictionary : public object
{
private:
  typedef std::map<const char *, object *> store_type;
  typedef store_type::iterator             store_iter;
  typedef store_type::const_iterator       store_citer;

  store_type _store;
  store_type _mine;

public:
  virtual ~dictionary ();

  /*! Insert a key/value pair into the dictionary
   *
   *  If the key already exists, its value is replaced with the new one.
   *
   *  The key is written to the PDF file as a name object as defined in the
   *  PDF spec [p. 59]
   */
  void insert (const char *key, object *value);

  void insert (const char *key, primitive value);
  void insert (const char *key, object value);

  /*! Count the number of objects in the dictionary
   */
  size_t size () const;

  /*! Obtain a reference to an object with a given key
   */
  const object * operator[] (const char *key) const;

  void operator>> (std::ostream& os) const;
};

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pdf_dictionary_hpp_ */
