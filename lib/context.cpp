//  context.cpp -- in which to interpret octets in streams
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include "utsushi/context.hpp"

namespace utsushi {

#define DEFAULT_CONTENT_TYPE "image/x-raster"
#define BAD_PIXEL_TYPE std::logic_error ("unsupported pixel type")

context::context (const size_type& width, const size_type& height,
                  const _pxl_type_& pixel_type)
  : content_type_(DEFAULT_CONTENT_TYPE)
  , pixel_type_(pixel_type)
  , height_(height)
  , width_(width)
  , h_padding_(0)
  , w_padding_(0)
  , x_resolution_(0)
  , y_resolution_(0)
  , octets_seen_(0)
  , orientation_(undefined)
  , direction_(unknown)
{
  check_pixel_type_();
}

context::context (const size_type& width, const size_type& height,
                  const std::string content_type,
                  const context::_pxl_type_& pixel_type)
  : content_type_(content_type)
  , pixel_type_(pixel_type)
  , height_(height)
  , width_(width)
  , h_padding_(0)
  , w_padding_(0)
  , x_resolution_(0)
  , y_resolution_(0)
  , octets_seen_(0)
  , orientation_(undefined)
  , direction_(unknown)
{
  check_pixel_type_();
}

std::string
context::content_type () const
{
  return content_type_;
}

void
context::content_type (const std::string& type)
{
  content_type_ = type;
}

bool
context::is_image () const
{
  return (0 == content_type ().find ("image/"));
}

bool
context::is_raster_image () const
{
  return (DEFAULT_CONTENT_TYPE == content_type_);
}

bool
context::is_rgb () const
{
  return RGB8 == pixel_type_ || RGB16 == pixel_type_;
}

context::size_type
context::size () const
{
  if (unknown_size == height () || unknown_size == width ())
    return unknown_size;

  return height () * width ();
}

context::size_type
context::height () const
{
  return height_;
}

context::size_type
context::width () const
{
  return width_;
}

context::size_type
context::depth () const
{
  switch (pixel_type_)
    {
    case MONO:
      return 1;
    case GRAY8:
    case RGB8:
      return 8;
    case GRAY16:
    case RGB16:
      return 16;
    default:
      BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);
    }
}

context::size_type
context::scan_size () const
{
  if (unknown_size == scan_height () || unknown_size == scan_width ())
    return unknown_size;

  return scan_height () * scan_width ();
}

context::size_type
context::scan_height () const
{
  if (unknown_size == height_)
    return unknown_size;

  return height_;
}

context::size_type
context::x_resolution () const
{
  return x_resolution_;
}

context::size_type
context::y_resolution () const
{
  return y_resolution_;
}

context::size_type
context::scan_width () const
{
  if (unknown_size == width_)
    return unknown_size;

  if (MONO == pixel_type_)
    return (width_ + 7) / 8;

  return width_ * octets_per_pixel_();
}

context::size_type
context::octets_per_image () const
{
  if (   unknown_size == lines_per_image ()
      || unknown_size == octets_per_line ())
    return unknown_size;

  return lines_per_image () * octets_per_line ();
}

context::size_type
context::lines_per_image () const
{
  if (unknown_size == scan_height ())
    return unknown_size;

  return scan_height () + padding_lines ();
}

context::size_type
context::octets_per_line () const
{
  if (unknown_size == scan_width ())
    return unknown_size;

  return scan_width () + padding_octets ();
}

context::size_type
context::padding_lines () const
{
  return h_padding_;
}

context::size_type
context::padding_octets () const
{
  return w_padding_;
}

context::size_type
context::octets_seen () const
{
  return octets_seen_;
}

context::size_type&
context::octets_seen ()
{
  return octets_seen_;
}

void
context::height (const size_type& pixels, const size_type& padding)
{
  height_ = pixels;
  h_padding_ = padding;
}

void
context::width (const size_type& pixels, const size_type& padding)
{
  width_ = pixels;
  w_padding_ = padding;
}

void
context::depth (const size_type& bits)
{
  /**/ if (1 == comps ())
    {
      /**/ if ( 1 == bits) pixel_type_ = MONO;
      else if ( 8 == bits) pixel_type_ = GRAY8;
      else if (16 == bits) pixel_type_ = GRAY16;
      else BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);
    }
  else if (3 == comps ())
    {
      /**/ if ( 1 == bits) pixel_type_ = MONO;
      else if ( 8 == bits) pixel_type_ = RGB8;
      else if (16 == bits) pixel_type_ = RGB16;
      else BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);
    }
  else
    BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);
}

void
context::resolution (const size_type& res)
{
  resolution (res, res);
}

void
context::resolution (const size_type& x_res, const size_type& y_res)
{
  x_resolution_ = x_res;
  y_resolution_ = y_res;
}

context::orientation_type
context::orientation () const
{
  return orientation_;
}

void
context::orientation (const orientation_type& orientation)
{
  orientation_ = orientation;
}

context::direction_type
context::direction () const
{
  return direction_;
}

void
context::direction (const direction_type& direction)
{
  direction_ = direction;
}

context::size_type
context::octets_per_pixel_() const
{
  check_pixel_type_();
  if (MONO == pixel_type_)
    BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);

  return pixel_type_;
}

void
context::check_pixel_type_() const
{
  switch (pixel_type_)
    {
    case MONO:
    case GRAY8:
    case GRAY16:
    case RGB8:
    case RGB16:
      break;
    default:
      BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);
    }
}

short
context::comps () const
{
  switch (pixel_type_)
    {
    case MONO:
    case GRAY8:
    case GRAY16:
      return 1;
    case RGB8:
    case RGB16:
      return 3;
    default:
      BOOST_THROW_EXCEPTION (BAD_PIXEL_TYPE);
    }
}

}       // namespace utsushi
