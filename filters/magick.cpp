//  magick.cpp -- touches applied to your image data
//  Copyright (C) 2014  SEIKO EPSON CORPORATION
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "magick.hpp"

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>

#include <boost/lexical_cast.hpp>

#include <cstdlib>

namespace {

using namespace utsushi;
using boost::lexical_cast;
using std::string;

inline
string
geom_(context::size_type width, context::size_type height)
{
  return (lexical_cast< string > (width)
          + 'x'
          + lexical_cast< string > (height));
}

}       // namespace

namespace utsushi {
namespace _flt_ {

magick::magick ()
  : shell_pipe (MAGICK_CONVERT)
  , force_extent_(false)
{
  option_->add_options ()
    ("force-extent", toggle (false))
    ("resolution-x", quantity ())
    ("resolution-y", quantity ())
    ("width", quantity ())
    ("height", quantity ())
    ;
}

void
magick::bos (const context& ctx)
{
  quantity x_res = value ((*option_)["resolution-x"]);
  quantity y_res = value ((*option_)["resolution-y"]);

  x_resolution_ = x_res.amount< double > ();
  y_resolution_ = y_res.amount< double > ();

  toggle t = value ((*option_)["force-extent"]);
  force_extent_ = t;

  if (force_extent_)
    {
      quantity w = value ((*option_)["width"]);
      quantity h = value ((*option_)["height"]);

      width_  = w.amount< double > ();
      height_ = h.amount< double > ();
    }
}

context
magick::estimate (const context& ctx) const
{
  double x_sample_factor = x_resolution_ / ctx.x_resolution ();
  double y_sample_factor = y_resolution_ / ctx.y_resolution ();

  context rv (ctx);

  rv.width  (ctx.width  () * x_sample_factor);
  rv.height (ctx.height () * y_sample_factor);
  rv.resolution (x_resolution_, y_resolution_);

  if (force_extent_)
    {
      rv.width  (width_  * x_resolution_);
      rv.height (height_ * y_resolution_);
    }

  return rv;
}

context
magick::finalize (const context& ctx) const
{
  double x_sample_factor = x_resolution_ / ctx.x_resolution ();
  double y_sample_factor = y_resolution_ / ctx.y_resolution ();

  context rv (ctx);

  rv.width  (ctx.width  () * x_sample_factor);
  rv.height (ctx.height () * y_sample_factor);
  rv.resolution (x_resolution_, y_resolution_);

  if (force_extent_)
    {
      rv.width  (width_  * x_resolution_);
      rv.height (height_ * y_resolution_);
    }

  return rv;
}

std::string
magick::arguments (const context& ctx)
{
  using std::string;

  string argv;

  // Set up input data characteristics

  argv += " -size " + geom_(ctx.width (), ctx.height ());
  argv += " -depth " + lexical_cast< string > (ctx.depth ());
  argv += " -density " + geom_(ctx.x_resolution (), ctx.y_resolution ());
  argv += " -units PixelsPerInch";
  if (ctx.is_raster_image ())
    {
      /**/ if (ctx.is_rgb ())     argv += " rgb:-";
      else if (1 != ctx.depth ()) argv += " gray:-";
      else                        argv += " mono:-";
    }
  else
    argv += " -";

  // Specify the "resampling" algorithm and parameters.  Pass output
  // resolutions so they can be embedded where supported by the data
  // format.

  argv += " -density " + geom_(ctx_.x_resolution (), ctx_.y_resolution ());

  double x_sample_factor = x_resolution_ / ctx.x_resolution ();
  double y_sample_factor = y_resolution_ / ctx.y_resolution ();

  argv += " -scale " + geom_(ctx.width ()  * x_sample_factor,
                             ctx.height () * y_sample_factor) + "!";

  if (force_extent_)
    argv += " -extent " + geom_(width_  * x_resolution_,
                                height_ * y_resolution_);

  // Prevent GraphicsMagick from converting gray JPEG images to RGB
  if (ctx.is_raster_image () && !ctx.is_rgb ())
    argv += " -colorspace gray";

  argv += " -";                 // output destination

  return argv;
}

}       // namespace _flt_
}       // namespace utsushi
