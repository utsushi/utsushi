//  tiff.cpp -- TIFF image file format support
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

#include "tiff.hpp"

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>

#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>
#include <boost/throw_exception.hpp>

#include <cerrno>
#include <cstdarg>
#include <ios>
#include <new>
#include <stdexcept>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace utsushi {
namespace _out_ {

using std::bad_alloc;
using std::ios_base;
using std::logic_error;
using std::runtime_error;

std::string tiff_odevice::err_msg = std::string ();

namespace {

// The TIFF library only allows for library-wide handlers.  If there
// are more components that use the TIFF library in the same run-time,
// then they may be clobbering each other's handlers.  Until such time
// this possibility will be ignored.
//
// The error and warning handlers below simply forward the formatted
// message to utsushi::log at a suitable level of verbosity.  In case
// of an error, the class-wide tiff_odevice::err_msg variable is set
// so that member functions can create a more intelligible exception.
// They need to clear this variable before calling TIFF library API.

using boost::scoped_array;

void
handle_error (const char *module, const char *fmt, va_list ap)
{
  int sz = vsnprintf (NULL, 0, fmt, ap);
  scoped_array< char > buf (new char[sz + 1]);

  vsnprintf (buf.get (), sz + 1, fmt, ap);
  log::fatal ("%1%: %2%") % module % buf.get ();

  tiff_odevice::err_msg = buf.get ();
}

void
handle_warning (const char *module, const char *fmt, va_list ap)
{
  int sz = vsnprintf (NULL, 0, fmt, ap);
  scoped_array< char > buf (new char[sz + 1]);

  vsnprintf (buf.get (), sz + 1, fmt, ap);
  log::alert ("%1%: %2%") % module % buf.get ();
}

}       // namespace

tiff_odevice::tiff_odevice (const std::string& filename)
  : file_odevice (filename)
  , tiff_(NULL)
{
  if (filename_ == "/dev/stdout")
    {
      if (-1 == lseek (STDOUT_FILENO, 0L, SEEK_SET))
        {
          if (ESPIPE == errno)
            BOOST_THROW_EXCEPTION
              (logic_error ("cannot write TIFF to tty or pipe"));
          else
            BOOST_THROW_EXCEPTION
              (runtime_error (strerror (errno)));
        }
    }

  TIFFSetErrorHandler (handle_error);
  TIFFSetWarningHandler (handle_warning);
}

tiff_odevice::tiff_odevice (const path_generator& generator)
  : file_odevice (generator)
  , tiff_(NULL)
{
  TIFFSetErrorHandler (handle_error);
  TIFFSetWarningHandler (handle_warning);
}

tiff_odevice::~tiff_odevice ()
{
  close ();
}

streamsize
tiff_odevice::write (const octet *data, streamsize n)
{
  BOOST_ASSERT ((data && 0 < n) || 0 == n);

  boost::scoped_array< octet > tmp;
  if (HAVE_GRAPHICS_MAGICK
      && (1 == ctx_.depth() && 1 == ctx_.comps()))
    {
      tmp.reset (new octet[n]);
      for (streamsize i = 0; i < n; ++i)
        {
          octet v = data[i];

          v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
          v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
          v = ((v >> 4) & 0x0F) | ((v & 0x0F) << 4);

          tmp[i] = v;
        }
      data = tmp.get ();
    }

  streamsize octets = std::min (ctx_.octets_per_line () - partial_size_, n);

  {                             // continue with stashed octets
    traits::copy (partial_line_.get () + partial_size_,
                  data, octets);
    partial_size_ += octets;
    if (partial_size_ == ctx_.octets_per_line ())
      {
        err_msg.clear ();
        if (1 != TIFFWriteScanline (tiff_, partial_line_.get (), row_, 1))
          {
            BOOST_THROW_EXCEPTION (ios_base::failure (err_msg));
          }
        ctx_.octets_seen () += ctx_.octets_per_line ();
        ++row_;
      }
    else
      {
        return n;
      }
  }

  while (octets + ctx_.octets_per_line () <= n)
    {
      // TIFFWriteScanline() is not const-correct :-(
      tdata_t buffer (const_cast< octet * > (data + octets));
      err_msg.clear ();
      if (1 != TIFFWriteScanline (tiff_, buffer, row_, 1))
        {
          BOOST_THROW_EXCEPTION (ios_base::failure (err_msg));
        }
      octets              += ctx_.octets_per_line ();
      ctx_.octets_seen () += ctx_.octets_per_line ();
      ++row_;
    }

  partial_size_ = n - octets;
  if (0 < partial_size_)      // stash left-over octets for next write
    {
      traits::copy (partial_line_.get (), data + octets,
                    partial_size_);
    }

  return n;
}

void
tiff_odevice::open ()
{
  file_odevice::open ();
  err_msg.clear ();
  tiff_ = TIFFFdOpen (fd_, filename_.c_str (), "w");

  if (!tiff_)
    {
      eof (ctx_);               // reverse effects of base class' open()
      BOOST_THROW_EXCEPTION (ios_base::failure (err_msg));
    }
}

void
tiff_odevice::close ()
{
  if (!tiff_) return;

  TIFFClose (tiff_);
  tiff_ = NULL;

  // The call to TIFFClose() causes fd_ to become invalid.  That will
  // trigger a spurious warning in the base class' close().  Setting
  // fd_ to -1 here will prevent the warning but also short-circuits
  // any other logic that that function is taking care of.
  // Try reopening the file to get a good file descriptor again.  In
  // any case, make sure that the base class' close() can do its job.

  int fd = ::open (filename_.c_str (), O_RDONLY);
  if (-1 == fd)
    {
      log::alert (strerror (errno));
    }
  else
    {
      fd_ = fd;
    }

  file_odevice::close ();
}

void
tiff_odevice::bos (const context& ctx)
{
  page_ = 0;
  file_odevice::bos (ctx_);
}

void
tiff_odevice::boi (const context& ctx)
{
  if (!(1 == ctx.comps () || 3 == ctx.comps ()))
    {
      BOOST_THROW_EXCEPTION
        (logic_error ("unsupported colour space"));
    }
  if (!(1 == ctx.depth () || 8 == ctx.depth ()))
    {
      BOOST_THROW_EXCEPTION
        (logic_error ("unsupported bit depth"));
    }

  ctx_ = ctx;
  ctx_.content_type ("image/tiff");

  partial_line_.reset (new octet[ctx_.octets_per_line ()]);
  partial_size_ = 0;
  ctx_.octets_seen () = 0;

  ++page_;
  row_ = 0;

  file_odevice::boi (ctx_);

  // set up TIFF tags for the upcoming image

  TIFFSetField (tiff_, TIFFTAG_SAMPLESPERPIXEL, ctx.comps ());

  uint16 pm = 0;                // uint16 is courtesy of tiffio.h
  if (8 == ctx.depth())
    {
      if (3 == ctx.comps())
        {
          pm = PHOTOMETRIC_RGB;
        }
      else if (1 == ctx.comps())
        {
          pm = PHOTOMETRIC_MINISBLACK;
        }
    }
  else if (1 == ctx.depth() && 1 == ctx.comps())
    {
      pm = PHOTOMETRIC_MINISBLACK;
    }
  TIFFSetField (tiff_, TIFFTAG_PHOTOMETRIC, pm);

  if (3 == ctx.comps())
    TIFFSetField (tiff_, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (tiff_, TIFFTAG_BITSPERSAMPLE, ctx.depth());

  TIFFSetField (tiff_, TIFFTAG_IMAGEWIDTH , ctx.width ());
  TIFFSetField (tiff_, TIFFTAG_IMAGELENGTH, ctx.height ());
  TIFFSetField (tiff_, TIFFTAG_ROWSPERSTRIP, 1);

  if (0 != ctx.x_resolution () && 0 != ctx.y_resolution ())
    {
      TIFFSetField (tiff_, TIFFTAG_XRESOLUTION, float (ctx.x_resolution ()));
      TIFFSetField (tiff_, TIFFTAG_YRESOLUTION, float (ctx.y_resolution ()));
      TIFFSetField (tiff_, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    }

  TIFFSetField (tiff_, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
}

void
tiff_odevice::eoi (const context& ctx)
{
  BOOST_ASSERT (partial_size_ == 0);
  BOOST_ASSERT (ctx_.octets_seen () == ctx.octets_per_image ());

  err_msg.clear ();
  if (1 != TIFFWriteDirectory (tiff_))
    {
      BOOST_THROW_EXCEPTION (ios_base::failure (err_msg));
    }

  file_odevice::eoi (ctx_);
}

}       // namespace _out_
}       // namespace utsushi
