//  filter.hpp -- interface declarations
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_filter_hpp_
#define utsushi_filter_hpp_

#include "device.hpp"
#include "iobase.hpp"
#include "memory.hpp"

#include "pattern/decorator.hpp"

namespace utsushi {

//!  Modify an image data sequence
//!  Interface for image data consuming filters
class filter
  : public device< output >
  , public output
{
public:
  typedef shared_ptr< filter > ptr;

  void mark (traits::int_type c, const context& ctx);

  //!  Sets a filter's underlying output object
  virtual void open (output::ptr output);

  using output::buffer_size;
  virtual void buffer_size (streamsize size);

protected:

  output::ptr output_;
};

//!  Add responsibilities to a \c filter
/*!  Meant as a convenient starting point for any filter decorator,
 *   this class implements the full \em public filter API by simply
 *   forwarding the API call to the decorated object.  This way, any
 *   subclass only needs to override those parts that require added
 *   responsibilities.
 */
template<>
class decorator< filter >
  : public filter
{
public:
  typedef shared_ptr< filter > ptr;

  decorator (ptr instance);

  streamsize write (const octet *data, streamsize n);
  void mark (traits::int_type c, const context& ctx);

  void open (output::ptr output);

  streamsize buffer_size () const;
  void buffer_size (streamsize size);
  context get_context () const;

protected:
  ptr instance_;
};

}       // namespace utsushi

#endif  /* utsushi_filter_hpp_ */
