//  filter.hpp -- interface declarations
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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
template< typename IO >
class filter
  : public device< IO >
{
protected:
  typedef shared_ptr< IO > io_ptr;

public:
  typedef shared_ptr< filter > ptr;

  //!  Sets a filter's underlying I/O object
  virtual void open (io_ptr io) { io_ = io; }

protected:
  io_ptr io_;
};

//!  Interface for image data producing filters
class ifilter
  : public filter< input >
  , public input
{
public:
  typedef shared_ptr< ifilter > ptr;

  streamsize marker ();

  using input::buffer_size;
  virtual void buffer_size (streamsize size);
};

//!  Interface for image data consuming filters
class ofilter
  : public filter< output >
  , public output
{
public:
  typedef shared_ptr< ofilter > ptr;

  void mark (traits::int_type c, const context& ctx);

  using output::buffer_size;
  virtual void buffer_size (streamsize size);
};

//!  Add responsibilities to an \c ifilter
/*!  Meant as a convenient starting point for any ifilter decorator,
 *   this class implements the full \em public ifilter API by simply
 *   forwarding the API call to the decorated object.  This way, any
 *   subclass only needs to override those parts that require added
 *   responsibilities.
 */
template<>
class decorator< ifilter >
  : public ifilter
{
public:
  typedef shared_ptr< ifilter > ptr;

  decorator (ptr instance);

  streamsize read (octet *data, streamsize n);
  streamsize marker ();

  void open (io_ptr io);

  streamsize buffer_size () const;
  void buffer_size (streamsize size);
  context get_context () const;

protected:
  ptr instance_;
};

//!  Add responsibilities to an \c ofilter
/*!  Meant as a convenient starting point for any ofilter decorator,
 *   this class implements the full \em public ofilter API by simply
 *   forwarding the API call to the decorated object.  This way, any
 *   subclass only needs to override those parts that require added
 *   responsibilities.
 */
template<>
class decorator< ofilter >
  : public ofilter
{
public:
  typedef shared_ptr< ofilter > ptr;

  decorator (ptr instance);

  streamsize write (const octet *data, streamsize n);
  void mark (traits::int_type c, const context& ctx);

  void open (io_ptr io);

  streamsize buffer_size () const;
  void buffer_size (streamsize size);
  context get_context () const;

protected:
  ptr instance_;
};

}       // namespace utsushi

#endif  /* utsushi_filter_hpp_ */
