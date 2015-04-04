//  magick.cpp -- touches applied to your image data
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

//  Guard against preprocessor symbol confusion.  We only care about the
//  command line utilities here, not about any C++ API.
#ifdef HAVE_GRAPHICS_MAGICK_PP
#undef HAVE_GRAPHICS_MAGICK_PP
#endif
#ifdef HAVE_IMAGE_MAGICK_PP
#undef HAVE_IMAGE_MAGICK_PP
#endif

//  Guard against possible mixups by preferring GraphicsMagick in case
//  both are available.
#if HAVE_GRAPHICS_MAGICK
#if HAVE_IMAGE_MAGICK
#undef  HAVE_IMAGE_MAGICK
#define HAVE_IMAGE_MAGICK 0
#endif
#endif

#include "magick.hpp"

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>
#include <utsushi/store.hpp>
#include <utsushi/log.hpp>

#include <boost/lexical_cast.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <stdio.h>

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

bool
image_magick_version_before_(const char *cutoff)
{
  FILE *fp = popen (MAGICK_CONVERT " -version"
                    "| awk '/^Version:/{print $3}'", "r");
  int errc = errno;

  if (fp)
    {
      char  buf[80];
      char *version = fgets (buf, sizeof (buf), fp);
      errc = errno;

      fclose (fp);

      if (version)
        {
          log::debug (format ("using ImageMagick-%1%") % version);
          return (0 > strverscmp (version, cutoff));
        }
    }

  log::alert (format ("failed to get ImageMagick version: %1%")
              % strerror (errc));
  return false;
}

}       // namespace

namespace utsushi {
namespace _flt_ {

magick::magick ()
  : shell_pipe (MAGICK_CONVERT)
  , bilevel_(false)
  , force_extent_(false)
{
  option_->add_options ()
    ("bilevel", toggle (false))
    ("threshold", (from< range > ()     // percentage
                   -> lower (  0.0)
                   -> upper (100.0)
                   -> default_value (50.0)))
    ("force-extent", toggle (false))
    ("resolution-x", quantity ())
    ("resolution-y", quantity ())
    ("width", quantity ())
    ("height", quantity ())
    ("image-format", (from< store > ()
                      -> alternative ("PNG")
                      -> alternative ("PNM")
                      -> alternative ("JPEG")
                      -> alternative ("TIFF")
                      -> alternative ("PDF")
                      -> default_value (string ())))
    ("color-correction", toggle (false))
    ;

  for (size_t i = 0; i < sizeof (cct_) / sizeof (*cct_); ++i)
    {
      key k ("cct-" + lexical_cast< std::string > (i+1));
      option_->add_options ()
        (k, quantity ());
    }
}

void
magick::freeze_options ()
{
  {
    toggle t = value ((*option_)["bilevel"]);
    bilevel_ = t;

    quantity thr = value ((*option_)["threshold"]);
    threshold_ = thr.amount< double > ();

    toggle c = value ((*option_)["color-correction"]);
    color_correction_ = c;

    for (size_t i = 0; i < sizeof (cct_) / sizeof (*cct_); ++i)
      {
        key k ("cct-" + boost::lexical_cast< std::string > (i+1));
        quantity q = value ((*option_)[k]);
        cct_[i] = q.amount< double > ();
      }
  }

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

  image_format_ = value ((*option_)["image-format"]);
}

context
magick::estimate (const context& ctx)
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

  if (image_format_)
    {
      /**/ if (image_format_ == "GIF" ) rv.content_type ("image/gif");
      else if (image_format_ == "JPEG") rv.content_type ("image/jpeg");
      else if (image_format_ == "PDF")
        {
          rv.content_type (bilevel_
                           ? "image/x-portable-bitmap"
                           : "image/jpeg");
        }
      else if (image_format_ == "PNG" ) rv.content_type ("image/png");
      else if (image_format_ == "PNM" )
        {
          rv.content_type ("image/x-portable-anymap");
        }
      else if (image_format_ == "TIFF") rv.content_type ("image/x-raster");
      else
        {
          // internal error
        }
    }
  else
    {
      rv.content_type ("image/x-raster");
    }

  if (bilevel_)
    {
      rv.depth (1);
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

  // Pass output resolutions so they can be embedded where supported
  // by the data format.

  argv += " -density " + geom_(ctx_.x_resolution (), ctx_.y_resolution ());

  // Specify the "resampling" algorithm and parameters, if necessary.

  if (   x_resolution_ != ctx.x_resolution ()
      || y_resolution_ != ctx.y_resolution ())
    {
      double x_sample_factor = x_resolution_ / ctx.x_resolution ();
      double y_sample_factor = y_resolution_ / ctx.y_resolution ();

      argv += " -scale " + geom_(ctx.width ()  * x_sample_factor,
                                 ctx.height () * y_sample_factor) + "!";
    }

  if (force_extent_)
    argv += " -extent " + geom_(width_  * x_resolution_,
                                height_ * y_resolution_);

  if (color_correction_)
    {
      if (HAVE_IMAGE_MAGICK
          && !image_magick_version_before_("6.6.1-0"))
        argv += " -color-matrix";
      else
        argv += " -recolor";

      argv += " \"";
      for (size_t i = 0; i < sizeof (cct_) / sizeof (*cct_); ++i)
        argv += lexical_cast< string > (cct_[i]) + " ";
      argv += "\"";
    }

  if (bilevel_)
    {
      // Thresholding an already thresholded image should be safe
      argv += " -threshold " + lexical_cast< string > (threshold_) + "%";
      if (image_format_ == "PNG")
        {
          argv += " -monochrome";
        }
      else
        {
          argv += " -type bilevel";
        }
    }

  // Prevent GraphicsMagick from converting gray JPEG images to RGB
  if (HAVE_GRAPHICS_MAGICK && !ctx.is_rgb ())
    argv += " -colorspace gray";

  if (image_format_)
    {
      /**/ if (image_format_ == "GIF" ) argv += " gif:-";
      else if (image_format_ == "JPEG") argv += " jpeg:-";
      else if (image_format_ == "PDF" )
        {
          if (!bilevel_) argv += " jpeg:-";
          else           argv += " pbm:-";
        }
      else if (image_format_ == "PNG" ) argv += " png:-";
      else if (image_format_ == "PNM" ) argv += " pnm:-";
      else if (image_format_ == "TIFF")
        {
          argv += " -depth " + lexical_cast< string > (ctx_.depth ());
          /**/ if (ctx_.is_rgb ())
            argv += " rgb:-";
          else
            {
              if (HAVE_GRAPHICS_MAGICK
                  && bilevel_)
                argv += " mono:-";
              else
                argv += " gray:-";
            }
        }
      else
        {
          argv += " -";
        }
    }
  else
    {
      argv += " -depth " + lexical_cast< string > (ctx_.depth ());
      /**/ if (ctx_.is_rgb ())
        argv += " rgb:-";
      else
        {
          if (HAVE_GRAPHICS_MAGICK
              && bilevel_)
            argv += " mono:-";
          else
            argv += " gray:-";
        }
    }

  return argv;
}

}       // namespace _flt_
}       // namespace utsushi
