//  magick.cpp -- touches applied to your image data
//  Copyright (C) 2014-2016  SEIKO EPSON CORPORATION
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

#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <stdio.h>
#include <string.h>

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

inline void
chomp (char *str)
{
  if (!str) return;
  char *nl = strrchr (str, '\n');
  if (nl) *nl = '\0';
}

bool
magick_version_before_(const char *magick, const char *cutoff)
{
  FILE *fp = NULL;
  int errc = 0;

  if (0 == strcmp ("GraphicsMagick", magick))
    fp = popen ("gm convert -version"
                "| awk '/^GraphicsMagick/{print $2}'", "r");
  if (fp) errc = errno;

  if (0 == strcmp ("ImageMagick", magick))
    fp  = popen ("convert -version"
                 "| awk '/^Version:/{print $3}'", "r");
  if (fp) errc = errno;

  if (fp)
    {
      char  buf[80];
      char *version = fgets (buf, sizeof (buf), fp);

      pclose (fp);
      chomp (version);

      if (version)
        {
          log::debug ("found %1%-%2%") % magick % version;
          return (0 > strverscmp (version, cutoff));
        }
    }

  if (errc)
    log::alert ("failure checking %1% version: %2%")
      % magick
      % strerror (errc);

  return false;
}

bool
graphics_magick_version_before_(const char *cutoff)
{
  return magick_version_before_("GraphicsMagick", cutoff);
}

bool
image_magick_version_before_(const char *cutoff)
{
  return magick_version_before_("ImageMagick", cutoff);
}

bool
auto_orient_is_usable ()
{
  static int usable = -1;

  if (-1 == usable)
    {
      if (HAVE_GRAPHICS_MAGICK)         // version -auto-orient was added
        usable = !graphics_magick_version_before_("1.3.18");
      if (HAVE_IMAGE_MAGICK)            // version known to work
        usable = !image_magick_version_before_("6.7.8-9");
      if (-1 == usable)
        usable = false;
      usable = (usable ? 1 : 0);
    }

  return usable;
}

const std::map < context::orientation_type, std::string >
orientation = boost::assign::map_list_of
  (context::bottom_left , "bottom-left")
  (context::bottom_right, "bottom-right")
  (context::left_bottom , "left-bottom")
  (context::left_top    , "left-top")
  (context::right_bottom, "right-bottom")
  (context::right_top   , "right-top")
  (context::top_left    , "top-left")
  (context::top_right   , "top-right")
  ;

}       // namespace

namespace utsushi {
namespace _flt_ {

magick::magick ()
  : shell_pipe (MAGICK_CONVERT)
  , bilevel_(false)
  , force_extent_(false)
  , auto_orient_(false)
{
  option_->add_options ()
    ("bilevel", toggle (false))
    ("threshold", (from< range > ()
                   -> lower (  0)
                   -> upper (255)
                   -> default_value (128)),
     attributes (tag::enhancement)(level::standard),
     SEC_N_("Threshold")
     )
    ("brightness", (from< range > ()
                    -> lower (-100)
                    -> upper ( 100)
                    -> default_value (0)),
     attributes (tag::enhancement)(level::standard),
     SEC_("Brightness"),
     CCB_("Change brightness of the acquired image.")
     )
    ("contrast", (from< range > ()
                  -> lower (-100)
                  -> upper ( 100)
                  -> default_value (0)),
     attributes (tag::enhancement)(level::standard),
     SEC_("Contrast"),
     CCB_("Change contrast of the acquired image.")
     )
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
    ("auto-orient", toggle (false))
    ;

  for (size_t i = 0; i < sizeof (cct_) / sizeof (*cct_); ++i)
    {
      key k ("cct-" + lexical_cast< std::string > (i+1));
      option_->add_options ()
        (k, quantity ());
    }
  freeze_options ();   // initializes option tracking member variables
}

void
magick::freeze_options ()
{
  {
    toggle t = value ((*option_)["bilevel"]);
    bilevel_ = t;

    quantity thr = value ((*option_)["threshold"]);
    thr *= 100.0;
    thr /= (dynamic_pointer_cast< range >
            ((*option_)["threshold"].constraint ()))->upper ();
    threshold_ = thr.amount< double > ();

    quantity brightness = value ((*option_)["brightness"]);
    brightness_ = brightness.amount< double > () / 100;

    quantity contrast = value ((*option_)["contrast"]);
    contrast_ = contrast.amount< double > () / 100;

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

  {
    toggle t = value ((*option_)["auto-orient"]);
    auto_orient_ = t;
  }
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
  if (auto_orient_)
    {
      rv.orientation (context::top_left);

      // Swap x/y attributes for 90/270 degree rotations
      if (   context::left_bottom  == ctx.orientation ()
          || context::right_top    == ctx.orientation ()
          || context::left_top     == ctx.orientation ()
          || context::right_bottom == ctx.orientation ())
        {
          context::size_type tmp = rv.width ();
          rv.width (rv.height ());
          rv.height (tmp);
          rv.resolution (rv.x_resolution (), rv.y_resolution ());
        }
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

  if (   0 != brightness_
      || 0 != contrast_)
    {
      if ( 1 - contrast_ <= 0.0)
      {
        contrast_ = 0.999;
      }
      
      double a = 1 / (1 - contrast_);
      double b = (brightness_ - contrast_) * a / 2;
      size_t mat_size = ((HAVE_IMAGE_MAGICK) ? 6 : 5);

      if (HAVE_IMAGE_MAGICK
          && !image_magick_version_before_("6.6.1-0"))
        argv += " -color-matrix";
      else
        argv += " -recolor";

      argv += " \"";
      for (size_t row = 0; row < mat_size; ++row)
        for (size_t col = 0; col < mat_size; ++col)
          {
            double coef = 0;

            if ((mat_size - 1)  == col) coef = ((3 > row) ? b : 0);
            if (row == col)             coef = ((3 > col) ? a : 1);
            argv += lexical_cast< string > (coef) + " ";
          }
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

  if (auto_orient_)
    {
      if (auto_orient_is_usable ())
        {
          try
            {
              std::string orient
                = " -orient " + orientation.at (ctx.orientation ());

              argv += orient + " -auto-orient";
              ctx_.orientation (context::top_left);
            }
          catch (const std::out_of_range&)
            {
              // FIXME log something?
            }
        }
      else                      // auto-orient emulation fallback
        {
          /**/ if (context::bottom_left  == ctx.orientation ())
            argv += " -flip";
          else if (context::bottom_right == ctx.orientation ())
            argv += " -flip -flop";
          else if (context::left_bottom  == ctx.orientation ())
            argv += " -rotate -90";
          else if (context::left_top     == ctx.orientation ())
            argv += " -rotate 90 -flop";
          else if (context::right_bottom == ctx.orientation ())
            argv += " -rotate -90 -flop";
          else if (context::right_top    == ctx.orientation ())
            argv += " -rotate 90";
          else if (context::top_left     == ctx.orientation ())
            argv += " -noop";
          else if (context::top_right    == ctx.orientation ())
            argv += " -flop";
          else
            {
              // FIXME log something?
            }
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
