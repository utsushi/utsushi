//  padding.hpp -- octet and scan line removal
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

#ifndef filters_padding_hpp_
#define filters_padding_hpp_

#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {

/*! \file
 *  \todo Move duplicated code into a single location
 *  \todo Add removal of padding scan lines
 *  \todo Add eoi() processing to ipadding filter
 *  \todo Add ipadding documentation
 */

class ipadding
  : public ifilter
{
public:
  //! Produces as much unpadded image data as possible
  streamsize read (octet *data, streamsize n);

  streamsize marker ();

protected:
  void handle_marker (traits::int_type c);

  context::size_type w_padding_;
  context::size_type h_padding_;
  context::size_type scan_line_count_;
  context::size_type skip_;
};

class padding
  : public ofilter
{
public:
  //! Consumes as much unpadded image data as possible
  /*! The implementation always consumes all \a data and the function
   *  therefore returns \a n.
   */
  streamsize write (const octet *data, streamsize n);

protected:
  //! Reinitialises members based on a context \a ctx
  /*! After requirement checking, the context \a ctx is copied, its
   *  padding information backed up and the padding information of the
   *  copied context is set to zero.  This is done here so that other
   *  producers later in the ostream get advance notice.
   */
  void boi (const context& ctx);

  //! Finalises the object's context based on \a ctx
  /*! The implementation accounts for the possibility of changed image
   *  dimensions.  If the final width or height of the input image is
   *  smaller than our initial "target" width or height, we label the
   *  offending octets in our output as padding and admit "defeat".
   *  If the input image turned out to be larger in either dimension,
   *  we confess our overzealous removal of image data.
   */
  void eoi (const context& ctx);

  //! Number of octets to chop off each scan line
  context::size_type w_padding_;

  //! Number of scan lines to chop off each image
  context::size_type h_padding_;

  //! Tracks the number of scan lines produced for a single image
  /*! This value is used to enable the removal of height padding.
   */
  context::size_type scan_line_count_;

  //! Remembers how much of what needs to be ignored.
  /*! Because write() consumes all data, we may end up with partially
   *  written scan lines or partially consumed padding.  A positive
   *  value indicates how many octets of a partial scan line have
   *  already been written.  A negative value how many padding octets
   *  still need to be ignored.
   */
  context::size_type skip_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_padding_hpp_ */
