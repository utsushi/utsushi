//  doc-locate.cpp -- post-process images based on document location
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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
//  C++ API here, not about any command line utilities.
#ifdef HAVE_GRAPHICS_MAGICK
#undef HAVE_GRAPHICS_MAGICK
#endif
#ifdef HAVE_IMAGE_MAGICK
#undef HAVE_IMAGE_MAGICK
#endif

//  Guard against possible mixups by preferring GraphicsMagick in case
//  both are available.
#if HAVE_GRAPHICS_MAGICK_PP
#if HAVE_IMAGE_MAGICK_PP
#undef  HAVE_IMAGE_MAGICK_PP
#define HAVE_IMAGE_MAGICK_PP 0
#endif
#endif

#include <Magick++.h>

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if HAVE_GRAPHICS_MAGICK_PP
#ifndef QuantumRange
#define QuantumRange ((Quantum) ((2 << QuantumDepth) - 1))
#endif
#endif

namespace {

//! Create a smaller version of an image
/*! Primarily useful to speed up later steps of the image analysis so
 *  they do not have to process as many pixels.
 *
 *  Uninteresting parts of the original image are removed as well.
 *  The returned image's page() attribute holds the original image's
 *  size after removal of any such parts.  This means that a scaling
 *  factor for each dimension can later be retrieved from the page()
 *  attribute together with the columns() and rows() attributes.
 *
 *  Normally, the uninteresting parts are border pixels of \e exactly
 *  the same color.  A \a fuzz factor in [0,1], can be passed to do a
 *  second round of removal of border areas.  In this case, colors
 *  that differ less than the fuzz factor are considered to be the
 *  identical.  How color differences are computed is implementation
 *  dependent.  Typically one should only need a small fuzz factor if
 *  the scan data was subject to lossy compression before transfer.
 *  A value of 0.01 to 0.02 should suffice.
 */
Magick::Image
thumbnail (const Magick::Image& image, double fuzz = -1)
{
  using Magick::Quantum;
  Magick::Image rv (image);

  rv.colorFuzz (0);
  rv.trim ();
  if (0 <= fuzz && fuzz <= 1)
    {
      rv.colorFuzz (fuzz * QuantumRange);
      rv.trim ();
      rv.colorFuzz (0);
    }

  Magick::Geometry trimmed_size (rv.columns (), rv.rows ());
  Magick::Geometry thumbed_size ("20%");

#if HAVE_GRAPHICS_MAGICK_PP
#if MagickLibVersion >= 0x151200
  rv.thumbnail (thumbed_size);
#else  /* use an inlined backport */
  {
    MagickLib::Image         *newImage;
    MagickLib::ExceptionInfo  exceptionInfo;

    long x = 0;
    long y = 0;
    unsigned long width  = rv.columns();
    unsigned long height = rv.rows();

    MagickLib::GetMagickGeometry
      (static_cast< std::string > (thumbed_size).c_str (),
       &x, &y,
       &width, &height);

    MagickLib::GetExceptionInfo (&exceptionInfo);
    newImage = MagickLib::ThumbnailImage (rv.image(), width, height,
                                         &exceptionInfo );
    rv.replaceImage (newImage);

    Magick::throwException (exceptionInfo);
  }
#endif
#else  /* HAVE_IMAGE_MAGICK_PP */
#if MagickLibVersion >= 0x689
  rv.thumbnail (thumbed_size);
#else  /* use an inlined backport */
  {
    MagickCore::Image         *newImage;
    MagickCore::ExceptionInfo *exceptionInfo;

#if MagickLibVersion >= 0x662
    size_t
#else
    unsigned long
#endif
      height = rv.rows (),
      width  = rv.columns ();

#if MagickLibVersion >= 0x662
    ssize_t
#else
    long
#endif
      x=0,
      y=0;

    MagickCore::ParseMetaGeometry
      (static_cast<std::string> (thumbed_size).c_str (),
       &x, &y, &width, &height);

    exceptionInfo = MagickCore::AcquireExceptionInfo ();
    newImage      = MagickCore::ThumbnailImage (rv.constImage (),
                                                width, height,
                                                exceptionInfo);
    rv.replaceImage (newImage);
    Magick::throwException (*exceptionInfo);
    MagickCore::DestroyExceptionInfo (exceptionInfo);
  }
#endif
#endif
  rv.page (trimmed_size);

  return rv;
}

//! Create a mask of an \a image's background
/*! The \a image is processed with two threshold values in [0,1] so
 *  that pixels with intensities between the \a lo_threshold and \a
 *  hi_threshold are black in the image returned.
 *
 *  Implementation dependent noise reduction is done as well in an
 *  attempt to make the background area a solid swat of black.  Its
 *  main purpose is to make sure that white pixels near the image's
 *  edges are \e not part of the background.
 *
 *  Note that the details of this noise reduction may depend on how
 *  much the \a image was downsized before being passed.
 */
Magick::Image
threshold (const Magick::Image& image,
           double lo_threshold, double hi_threshold)
{
  using Magick::Quantum;

  lo_threshold *= QuantumRange;
  hi_threshold *= QuantumRange;

  Magick::Image lo (image);
  lo.threshold (lo_threshold);

  Magick::Image hi (image);
  hi.threshold (hi_threshold);

  Magick::Image& mask (lo);
  mask.composite (hi, 0, 0, Magick::DifferenceCompositeOp);

  const double ign = 0;
  const double kernel[] = { ign, ign,  1 , ign, ign,
                            ign,  1 , ign,  1 , ign,
                             1 , ign, ign, ign,  1 ,
                            ign,  1 , ign,  1 , ign,
                            ign, ign,  1 , ign, ign };
  mask.convolve (5, kernel);
  mask.threshold (0.50 * QuantumRange);
  mask.negate ();
  mask.backgroundColor ("black");

  return mask;
}

class locator
{
  Magick::Image image_;
  Magick::Image mask_;

  double x_scale_;
  double y_scale_;
  double skew_angle_;

public:
  locator (const Magick::Image& image,
           double lo_threshold, double hi_threshold,
           double fuzz = -1)
    : image_(thumbnail (image, fuzz))
    , mask_(threshold (image_, lo_threshold, hi_threshold))
  {
    x_scale_  = image_.page ().width ();
    x_scale_ /= image_.columns ();
    y_scale_  = image_.page ().height ();
    y_scale_ /= image_.rows ();

    skew_angle_ = get_skew_angle (image_);
   }

  //! Returns the angle in degrees to rotate through in order to deskew
  double deskew_angle () const
  {
    return -skew_angle_ * 180 / M_PI;
  }

  //! Returns the minimally interesting part of the original image
  Magick::Geometry cropbox () const
  {
    return scale (bbox (mask_));
  }

  //! Returns the minimally interesting part of the deskewed image
  Magick::Geometry cropdoc () const
  {
    Magick::Image clone (mask_);

    clone.crop (bbox (mask_));
    clone.rotate (deskew_angle ());

    Magick::Geometry rv = bbox (clone);
    if (HAVE_IMAGE_MAGICK_PP)
    {
      if (clone.page ().xNegative ())
        rv.xOff (rv.xOff () - clone.page ().xOff());
      else
        rv.xOff (rv.xOff () + clone.page ().xOff());
      if (clone.page ().yNegative ())
        rv.yOff (rv.yOff () - clone.page ().yOff());
      else
        rv.yOff (rv.yOff () + clone.page ().yOff());
    }
    return scale (rv);
  }

private:

  Magick::Geometry scale (const Magick::Geometry& geometry) const
  {
    return Magick::Geometry (x_scale_ * geometry.width (),
                             y_scale_ * geometry.height (),
                             x_scale_ * geometry.xOff (),
                             y_scale_ * geometry.yOff ());
  }

  //! Returns remaining image size after removing black edges
  Magick::Geometry
  bbox (const Magick::Image& image) const
  {
    const Magick::Color black ("black");

#define is_black(x,y) (black == image.pixelColor (x, y))

    const unsigned int
      cols = image.columns (),
      rows = image.rows ();

    unsigned int x_min, x_max, y_min, y_max;

    bool found;
    unsigned int x, y;

    found = false;
    y = 0;
    do {
      x = 0;
      do {
        found = !is_black (x, y);
      } while (!found && cols != ++x);
    } while (!found && rows != ++y);
    y_min = y;

    found = false;
    y = rows;
    do {
      x = 0;
      do {
        found = !is_black (x, y-1);
      } while (!found && cols != ++x);
    } while (!found && 0 != --y);
    y_max = y;

    found = false;
    x = 0;
    do {
      y = 0;
      do {
        found = !is_black (x, y);
      } while (!found && rows != ++y);
    } while (!found && cols != ++x);
    x_min = x;

    found = false;
    x = cols;
    do {
      y = 0;
      do {
        found = !is_black (x-1, y);
      } while (!found && rows != ++y);
    } while (!found && 0 != --x);
    x_max = x;

    return Magick::Geometry (x_max - x_min, y_max - y_min, x_min, y_min);
  }

  double
  get_skew_angle (const Magick::Image& image) const
  {
    const double t = 0.40;

    int cols = image.columns ();
    int rows = image.rows ();
    double *img = new double[cols*rows*3];

    Magick::Image clone (image);
    clone.write (0, 0, cols, rows, "RGB", Magick::DoublePixel, img);

    int w;
    for (w = 1; w < (cols+7)/8; w <<= 1);
    double *ltr = new double[w*rows];
    double *rtl = new double[w*rows];

    memset (ltr, 0, w * rows * sizeof (double));
    memset (rtl, 0, w * rows * sizeof (double));

    unsigned short bits[256];
    for (unsigned i = 0; i < 256; ++i)
      {
        unsigned b = i, c = 0;

        for (; b; b >>= 1)
          c += b & 0x01;
        bits[i] = c;
      }

    double *ptr = img;
    for (int y = 0; y < rows; ++y)
      {
        unsigned bit = 0, byte = 0;
        unsigned i = 0;
        unsigned j = (cols+7)/8;

        for (int x = 0; x < cols; ++x)
          {
            double r = *ptr++;
            double g = *ptr++;
            double b = *ptr++;

            byte <<= 1;
            if ((r < t) || (g < t) || (b < t))
              byte |= 0x01;
            ++bit;
            if (8 == bit)
              {
                ltr[i++ + y * w] = bits[byte];
                rtl[--j + y * w] = bits[byte];
                bit = 0;
                byte = 0;
              }
          }
        if (bit)
          {
            byte <<= (8 - bit);
            ltr[i++ + y * w] = bits[byte];
            rtl[--j + y * w] = bits[byte];
          }
      }
    delete [] img;

    double *rm = new double[w * rows];
    double *rp = new double[2*w + 1];
    double *q = rm;
    double *p = ltr;

    memset (rm, 0, sizeof (double) * w * rows);
    memset (rp, 0, sizeof (double) * 2 * w + sizeof (double));

    for (int step = 1; step < w; step *= 2)
      {
        for (int x = 0; x < w; x += 2*step)
          {
            for (int i = 0; i < step; ++i)
              {
                int y = 0;
                for (; y < rows - i - 1; ++y)
                  {
                    double e = p[x+i      +  y     *w];
                    double n = p[x+i+step + (y+i  )*w];
                    double m = p[x+i+step + (y+i+1)*w];

                    q[x+2*i   + y*w] = e + n;
                    q[x+2*i+1 + y*w] = e + m;
                  }
                for (; y < rows - i; ++y)
                  {
                    double e = p[x+i      +  y     *w];
                    double n = p[x+i+step + (y+i  )*w];
                    double m = 0;

                    q[x+2*i   + y*w] = e + n;
                    q[x+2*i+1 + y*w] = e + m;
                  }
                for (; y < rows; ++y)
                  {
                    double e = p[x+i      +  y     *w];
                    double n = 0;
                    double m = 0;

                    q[x+2*i   + y*w] = e + n;
                    q[x+2*i+1 + y*w] = e + m;
                  }
              }
          }
        std::swap (p, q);
      }

    for (int x = 0; x < w; ++x)
      {
        double sum = 0;
        for (int y = 0; y < rows - 1; ++y)
          {
            double d = p[x + y * w] - p[x + (y+1)*w];
            sum += d * d;
          }
        rp[w - x - 1] = sum;
      }

    p = rtl;
    q = rm;

    for (int step = 1; step < w; step *= 2)
      {
        for (int x = 0; x < w; x += 2*step)
          {
            for (int i = 0; i < step; ++i)
              {
                int y = 0;
                for (; y < rows - i - 1; ++y)
                  {
                    double e = p[x+i      +  y     *w];
                    double n = p[x+i+step + (y+i  )*w];
                    double m = p[x+i+step + (y+i+1)*w];

                    q[x+2*i   + y*w] = e + n;
                    q[x+2*i+1 + y*w] = e + m;
                  }
                for (; y < rows - i; ++y)
                  {
                    double e = p[x+i      +  y     *w];
                    double n = p[x+i+step + (y+i  )*w];
                    double m = 0;

                    q[x+2*i   + y*w] = e + n;
                    q[x+2*i+1 + y*w] = e + m;
                  }
                for (; y < rows; ++y)
                  {
                    double e = p[x+i      +  y     *w];
                    double n = 0;
                    double m = 0;

                    q[x+2*i   + y*w] = e + n;
                    q[x+2*i+1 + y*w] = e + m;
                  }
              }
          }
        std::swap (p, q);
      }

    for (int x = 0; x < w; ++x)
      {
        double sum = 0;
        for (int y = 0; y < rows - 1; ++y)
          {
            double d = p[x + y * w] - p[x + (y+1)*w];
            sum += d * d;
          }
        rp[w + x - 1] = sum;
      }

    double mp = 0;
    double skew = 0;

    for (int i = 0; i < 2 * w - 1; ++i)
      {
        if (rp[i] > mp)
          {
            skew = i - w + 1;
            mp = rp[i];
          }
      }
    delete [] rp;
    delete [] rm;

    delete [] ltr;
    delete [] rtl;

    return -atan (skew/w/8);
  }
};

void
deskew (Magick::Image& image, const locator& loc)
{
  if (0 == loc.deskew_angle ()) return;

  using Magick::Quantum;

  size_t width  = image.columns ();
  size_t height = image.rows ();

  // TODO store bottom background color

  image.colorFuzz (0);
  image.trim ();

  // TODO store scanner background color

  image.colorFuzz (0.02 * QuantumRange);
  image.trim ();

  size_t cols = image.columns ();
  size_t rows = image.rows ();

  // TODO set background to scanner background color

  image.rotate (loc.deskew_angle ());
#if HAVE_GRAPHICS_MAGICK_PP
  {
    MagickLib::Image         *newImage;
    MagickLib::ExceptionInfo  exceptionInfo;
    MagickLib::RectangleInfo  geometry;

    MagickLib::SetGeometry(image.image (), &geometry);
    geometry.width  = cols;
    geometry.height = rows;
    geometry.x -= (image.image()->columns  - cols) / 2;
    geometry.y -= (image.image()->rows     - rows) / 2;

    image.modifyImage ();
    MagickLib::GetExceptionInfo (&exceptionInfo);
    newImage = MagickLib::ExtentImage (image.image (), &geometry,
                                       &exceptionInfo);
    image.replaceImage (newImage);

    Magick::throwException (exceptionInfo);
  }
#else  /* HAVE_IMAGE_MAGICK_PP */
#if MagickLibVersion >= 0x659
  image.extent (Magick::Geometry (cols, rows),
                Magick::CenterGravity);
#else  /* use an inlined backport */
  {
    MagickCore::Image         *newImage;
    MagickCore::ExceptionInfo *exceptionInfo;
    MagickCore::RectangleInfo  geometry;

    MagickCore::SetGeometry(image.image (), &geometry);
    geometry.width  = cols;
    geometry.height = rows;
    MagickCore::GravityAdjustGeometry (image.image ()->columns,
                                       image.image ()->rows,
                                       Magick::CenterGravity, &geometry);
    image.modifyImage ();
#if MagickLibVersion >= 0x650   /* somewhere between 0x644 and 0x657 */
    geometry.x *= -1;
    geometry.y *= -1;
#endif
    exceptionInfo = MagickCore::AcquireExceptionInfo ();
    newImage      = MagickCore::ExtentImage (image.image (), &geometry,
                                             exceptionInfo);
    image.replaceImage (newImage);

    Magick::throwException (*exceptionInfo);
    MagickCore::DestroyExceptionInfo (exceptionInfo);
  }
#endif
#endif

  // TODO set background to bottom background color

#if HAVE_GRAPHICS_MAGICK_PP
  {
    MagickLib::Image         *newImage;
    MagickLib::ExceptionInfo  exceptionInfo;
    MagickLib::RectangleInfo  geometry;

    MagickLib::SetGeometry(image.image (), &geometry);
    geometry.width  = width;
    geometry.height = height;

    image.modifyImage ();

    MagickLib::GetExceptionInfo (&exceptionInfo);
    newImage = MagickLib::ExtentImage (image.image (), &geometry,
                                       &exceptionInfo);
    image.replaceImage (newImage);

    Magick::throwException (exceptionInfo);
  }
#else  /* we HAVE_IMAGE_MAGICK_PP */
#if MagickLibVersion >= 0x659
  image.extent (Magick::Geometry (width, height),
                Magick::NorthGravity);
#else  /* use an inlined backport */
  {
    MagickCore::Image         *newImage;
    MagickCore::ExceptionInfo *exceptionInfo;
    MagickCore::RectangleInfo  geometry;

    MagickCore::SetGeometry(image.image (), &geometry);
    geometry.width  = width;
    geometry.height = height;
    MagickCore::GravityAdjustGeometry (image.image ()->columns,
                                       image.image ()->rows,
                                       Magick::NorthGravity, &geometry);
    image.modifyImage ();
    exceptionInfo = MagickCore::AcquireExceptionInfo ();
    newImage      = MagickCore::ExtentImage (image.image (), &geometry,
                                             exceptionInfo);
    image.replaceImage (newImage);

    Magick::throwException (*exceptionInfo);
    MagickCore::DestroyExceptionInfo (exceptionInfo);
  }
#endif
#endif
}

void
autocrop (Magick::Image& image, const locator& loc)
{
  image.crop (loc.cropbox ());
  if (0 != loc.deskew_angle ())
    {
      image.rotate (loc.deskew_angle ());
      image.crop (loc.cropdoc ());
    }
}

void
trim (Magick::Image& image, const locator& loc)
{
  image.colorFuzz (0);
  image.trim ();
}
}       // namespace

int
main (int argc, char *argv[])
{
  if (3 > argc)
    {
      std::cerr
        << "Usage: " << argv[0]
        << " lo hi [action [size [source [destination]]]]\n"
        << "\n"
        << "The program expects two threshold values bracketing the image's\n"
        << "background intensity.  Values should be in [0,1].\n"
        << "Supported actions are crop, trim and deskew.\n"
        << "Source and destination image specifications are optional.  They\n"
        << "default to standard input and standard output.\n"
        ;
      exit (1);
    }

  const double fuzz = -1;

  double lo_threshold;
  double hi_threshold;
  size_t data_size = 0;

  {
    std::stringstream ss;

    ss << argv[1];
    ss >> lo_threshold;
    if (!ss.eof ())
      {
        std::cerr << "Invalid argument (" << argv[1] << ")\n";
        exit (1);
      }
  }
  {
    std::stringstream ss;

    ss << argv[2];
    ss >> hi_threshold;
    if (!ss.eof ())
      {
        std::cerr << "Invalid argument (" << argv[2] << ")\n";
        exit (1);
      }
  }

  const std::string action (argc > 3 ? argv[3] : "show");

  if (argc > 4)
    {
      std::stringstream ss;

      ss << argv[4];
      ss >> data_size;
      if (!ss.eof ())
        {
          std::cerr << "Invalid argument (" << argv[4] << ")\n";
          exit (1);
        }
    }

  void (*process)(Magick::Image&, const locator&);

  /**/ if (action == "crop")   process = autocrop;
  else if (action == "trim")   process = trim;
  else if (action == "deskew") process = deskew;
  else
    {
      std::cerr << "Invalid action (" << action << ")\n";
      exit (EXIT_FAILURE);
    }

  const char *input  = (argc > 5 ? argv[5] : "-");
  const char *output = (argc > 6 ? argv[6] : "-");

  Magick::InitializeMagick (*argv);
  Magick::Image original;

  try
    {
      char *data = NULL;
      int fd = -1;

      if (0 < data_size)
        {
          data = (char *) malloc (data_size);
        }
      if (data)
        {
          if (!strcmp (input, "-"))
            fd = STDIN_FILENO;
          else
            fd = open (input, O_RDONLY);
          if (-1 == fd)
            {
              free (data);
              data = NULL;
            }
        }
      if (data && 0 <= fd)
        {
          size_t cnt = 0;
          size_t n;
          do
            {
              n = read (fd, data + cnt, data_size - cnt);
              if (0 < n) cnt += n;
            }
          while (0 < n
                 || EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno);

          if (-1 == close (fd))         // just warn about this
            std::cerr << strerror (errno) << std::endl;

          Magick::Blob blob;

          blob.updateNoCopy (data, cnt, Magick::Blob::MallocAllocator);
          // data is now owned by blob, do *not* free (data)
          original.read (blob);
        }
      else
        {
          original.read (input);
        }

      locator loc (original, lo_threshold, hi_threshold, fuzz);

      process (original, loc);
      original.write (output);
    }
  catch (std::exception &oops)
    {
      std::cerr << oops.what () << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
