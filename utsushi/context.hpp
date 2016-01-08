//  context.hpp -- in which to interpret octets in streams
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_context_hpp_
#define utsushi_context_hpp_

#include <cstdlib>

#include <ios>
#include <string>

#include "memory.hpp"

namespace utsushi {

class context
{
  typedef ssize_t size_type_;

public:
  typedef size_type_ size_type;

  enum {
    unknown_size = -1,
  };

  enum orientation_type {
    undefined,
    bottom_left,
    bottom_right,
    left_bottom,
    left_top,
    right_bottom,
    right_top,
    top_left,
    top_right,
  };

  enum direction_type {
    unknown,
    top_to_bottom,
    bottom_to_top,
  };

  //! \e Temporary pixel type classification scheme
  enum _pxl_type_ {
    unknown_type = -1,
    MONO  = 0,                  // 8 pixels to the octet
    GRAY8 = 1, GRAY16 = 2,
    RGB8  = 3, RGB16  = 6,
  };

  context (const size_type& width  = unknown_size,
           const size_type& height = unknown_size,
           const _pxl_type_& pixel_type = RGB8);
  context (const size_type& width, const size_type& height,
           const std::string content_type,
           const _pxl_type_& pixel_type = RGB8);

  //! A content type identifier as specified in RFC 2046
  /*!
   *  Additional information can be found at:
   *   - http://tools.ietf.org/html/rfc2046
   *   - http://en.wikipedia.org/wiki/Internet_media_type
   */
  std::string content_type () const;
  void content_type (const std::string& type);

  bool is_image () const;
  bool is_raster_image () const;
  bool is_rgb () const;

  //! Image size in pixels
  size_type size () const;
  //! Image height in pixels
  size_type height () const;
  //! Image width in pixels
  size_type width () const;
  //! Image depth in bits
  size_type depth () const;

  //! Image size in octets
  size_type scan_size () const;
  //! Image height in scan lines
  size_type scan_height () const;
  //! Image width in octets
  size_type scan_width () const;

  size_type x_resolution () const;
  size_type y_resolution () const;

  //! Number of octets in an image, includes padding octets
  size_type octets_per_image () const;
  //! Number of scan lines in an image, includes padding_lines()
  size_type lines_per_image () const;
  //! Number of octets per scan line, includes padding_octets()
  size_type octets_per_line () const;
  //! Number of extraneous scan lines in an image
  size_type padding_lines () const;
  //! Number of octets used to pad scan lines
  size_type padding_octets () const;

  size_type  octets_seen () const;
  size_type& octets_seen ();

  void height (const size_type& pixels, const size_type& padding = 0);
  void width (const size_type& pixels, const size_type& padding = 0);
  void depth (const size_type& bits);
  void resolution (const size_type& res);
  void resolution (const size_type& x_res, const size_type& y_res);

  orientation_type orientation () const;
  void orientation (const orientation_type& orientation);

  direction_type direction () const;
  void direction (const direction_type& direction);

protected:
  std::string content_type_;
  _pxl_type_  pixel_type_;

  size_type height_;
  size_type width_;
  size_type h_padding_;
  size_type w_padding_;
  size_type x_resolution_;
  size_type y_resolution_;

  std::streamsize octets_seen_;

  orientation_type orientation_;
  direction_type direction_;

  size_type octets_per_pixel_() const;

  void check_pixel_type_() const;

// highly experimental API from here on ...
public:
  typedef shared_ptr< context > ptr;

  short     comps  () const;
};

}       // namespace utsushi

#endif  /* utsushi_context_hpp_ */
