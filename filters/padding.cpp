//  padding.cpp -- octet and scan line removal
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

#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>

#include "padding.hpp"

namespace utsushi {
namespace _flt_ {

using std::logic_error;

streamsize
padding::write (const octet *data, streamsize n)
{
  BOOST_ASSERT ((data && 0 < n) || 0 == n);

  if (scan_line_count_ >= ctx_.scan_height ())
    return n;

  streamsize octets = 0;

  if (0 > skip_)                // skip remaining padding octets
    {
      octets = std::min (-skip_, n);
      skip_ += octets;
      if (octets == n)
        return n;
    }

  if (0 < skip_)                // continue with partial scanline
    {
      octets = std::min (ctx_.scan_width () - skip_, n);

      output_->write (data, octets);
      skip_ += octets;
      if (ctx_.scan_width () == skip_)
        {
          ++scan_line_count_;
          octets += w_padding_;
        }
      else
        {
          return octets;
        }
    }

  while (octets + ctx_.scan_width () <= n
         && scan_line_count_ < ctx_.scan_height ())
    {
      output_->write (data + octets, ctx_.scan_width ());
      ++scan_line_count_;
      octets += ctx_.scan_width ();
      octets += w_padding_;
    }

  if (scan_line_count_ < ctx_.scan_height ())
    {
      skip_ = n - octets;

      if (0 < skip_)            // write partial scanline
        output_->write (data + octets, skip_);
    }
  else                          // skip anything beyond last scanline
    {
      skip_ = 0;
    }

  return n;
}

void
padding::boi (const context& ctx)
{
  logic_error e
    ("padding only works with raster images of known size");

  if (!ctx.is_raster_image ())
    BOOST_THROW_EXCEPTION (e);
  if (0 != ctx.padding_octets ()
      && context::unknown_size == ctx.width ())
    BOOST_THROW_EXCEPTION (e);
  if (0 != ctx.padding_lines ()
      && context::unknown_size == ctx.height ())
    BOOST_THROW_EXCEPTION (e);

  w_padding_ = ctx.padding_octets ();
  h_padding_ = ctx.padding_lines ();
  scan_line_count_ = 0;
  skip_ = 0;

  ctx_ = ctx;
  ctx_.width (ctx.width (), 0);          // zap our padding settings
  ctx_.height (ctx.height (), 0);
}

void
padding::eoi (const context& ctx)
{
  context::size_type padding;

  if (ctx_.width () >= ctx.width ())
    {
      padding = ctx_.scan_width () - ctx.scan_width ();

      if (padding)
        log::alert ("%1% padding octets remain") % padding;

      ctx_.width (ctx.width (), padding);
    }
  else
    {
      log::alert
        ("%1% pixels inadvertently cropped when removing padding octets")
        % (ctx.width () - ctx_.width ());
    }

  if (ctx_.height () >= ctx.height ())
    {
      padding = ctx_.scan_height () - ctx.scan_height ();

      if (padding)
        log::alert ("%1% padding scan lines remain") % padding;

      ctx_.height (ctx.height (), padding);
    }
  else
    {
      log::alert
        ("%1% pixels inadvertently cropped when removing padding lines")
        % (ctx.height () - ctx_.height ());
    }
}

bottom_padder::bottom_padder (const quantity& width, const quantity& height)
  : width_(width)
  , height_(height)
{}

streamsize
bottom_padder::write (const octet *data, streamsize n)
{
  if (!octets_left_) return n;

  streamsize cnt = std::min (octets_left_, n);
  octets_left_  -= cnt;

  output_->write (data, cnt);

  return n;
}

void
bottom_padder::boi (const context& ctx)
{
  logic_error e ("bottom_padder only works with raster images");

  if (!ctx.is_raster_image ())
    BOOST_THROW_EXCEPTION (e);

  streamsize pixels = width_.amount< double > () * ctx.x_resolution ();
  if (pixels != ctx_.width ())
    log::error ("width padding not supported yet");

  streamsize lines = height_.amount< double > () * ctx.y_resolution ();

  ctx_ = ctx;
  ctx_.height (lines);

  octets_left_ = lines * ctx_.octets_per_line ();
}

void
bottom_padder::eoi (const context& ctx)
{
  streamsize size = default_buffer_size;

  boost::scoped_array< octet > pad (new octet [size]);

  traits::assign (pad.get (), size, 0xff);

  while (octets_left_)
    {
      streamsize cnt = std::min (octets_left_, size);
      octets_left_  -= cnt;

      output_->write (pad.get (), cnt);
    }
}

}       // namespace _flt_
}       // namespace utsushi
