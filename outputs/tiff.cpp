//  tiff.cpp -- TIFF image file format support
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#include <ios>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>

#include "utsushi/i18n.hpp"
#include "tiff.hpp"

namespace utsushi {
namespace _out_ {

using std::bad_alloc;
using std::ios_base;
using std::logic_error;
using std::runtime_error;

// TODO Review the overly trigger-happy throwing of exceptions.
//      We may want to pass error and warning handlers to the libtiff
//      code as well.

static TIFF *
open (const fs::path& name)
{
  // TIFFOpen() interprets 'b' as big-endian, not binary as fopen()
  TIFF *rv (TIFFOpen (name.string ().c_str (), "w"));
  if (!rv) BOOST_THROW_EXCEPTION (bad_alloc ());
  return rv;
}

static void set_tags (TIFF *_tiff, const context& ctx);

  tiff_odevice::tiff_odevice (const fs::path& name)
    : name_ (name)
  {}

  tiff_odevice::tiff_odevice (const path_generator& generator)
    : generator_ (generator)
  {}

  streamsize
  tiff_odevice::write (const octet *data, streamsize n)
  {
    BOOST_ASSERT ((data && 0 < n) || 0 == n);

    ios_base::failure e (_("failure writing TIFF scanline"));

    streamsize octets = std::min (ctx_.octets_per_line () - partial_size_, n);

    {                           // continue with stashed octets
      traits::copy (partial_line_.get () + partial_size_,
                    data, octets);
      partial_size_ += octets;
      if (partial_size_ == ctx_.octets_per_line ())
        {
          if (1 != TIFFWriteScanline (_tiff, partial_line_.get (), _row, 1))
            {
              BOOST_THROW_EXCEPTION (e);
            }
          ctx_.octets_seen () += ctx_.octets_per_line ();
          ++_row;
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
        if (1 != TIFFWriteScanline (_tiff, buffer, _row, 1))
          {
            BOOST_THROW_EXCEPTION (e);
          }
        octets              += ctx_.octets_per_line ();
        ctx_.octets_seen () += ctx_.octets_per_line ();
        ++_row;
      }

    partial_size_ = n - octets;
    if (0 < partial_size_)    // stash left-over octets for next write
      {
        traits::copy (partial_line_.get (), data + octets,
                      partial_size_);
      }

    return n;
  }

  void
  tiff_odevice::bos (const context& ctx)
  {
    _page = 0;
    if (!generator_) _tiff = open (name_);
  }

  void
  tiff_odevice::boi (const context& ctx)
  {
    if (!(1 == ctx.comps () || 3 == ctx.comps ()))
      {
        BOOST_THROW_EXCEPTION
          (logic_error (_("unsupported colour space")));
      }
    if (!(1 == ctx.depth () || 8 == ctx.depth ()))
      {
        BOOST_THROW_EXCEPTION
          (logic_error (_("unsupported bit depth")));
      }

    if (generator_) {
      name_ = generator_();
      _tiff = open (name_);
    }

    ctx_ = ctx;
    ctx_.content_type ("image/tiff");

    partial_line_.reset (new octet[ctx_.octets_per_line ()]);
    partial_size_ = 0;
    ctx_.octets_seen () = 0;

    ++_page;
    _row = 0;

    set_tags (_tiff, ctx_);
  }

  void
  tiff_odevice::eoi (const context& ctx)
  {
    BOOST_ASSERT (partial_size_ == 0);
    BOOST_ASSERT (ctx_.octets_seen () == ctx.octets_per_image ());

    if (1 != TIFFWriteDirectory (_tiff))
      {
        BOOST_THROW_EXCEPTION
          (runtime_error (_("failure writing TIFF directory")));
      }

    if (generator_) TIFFClose (_tiff);
  }

  void
  tiff_odevice::eos (const context& ctx)
  {
    if (!generator_) TIFFClose (_tiff);
  }

  static void
  set_tags (TIFF *_tiff, const context& ctx)
  {
    TIFFSetField (_tiff, TIFFTAG_SAMPLESPERPIXEL, ctx.comps ());

    uint16 pm = 0;              // uint16 is courtesy of tiffio.h
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
    TIFFSetField (_tiff, TIFFTAG_PHOTOMETRIC, pm);

    if (3 == ctx.comps())
      TIFFSetField (_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    TIFFSetField (_tiff, TIFFTAG_BITSPERSAMPLE, ctx.depth());

    TIFFSetField (_tiff, TIFFTAG_IMAGEWIDTH , ctx.width ());
    TIFFSetField (_tiff, TIFFTAG_IMAGELENGTH, ctx.height ());
    TIFFSetField (_tiff, TIFFTAG_ROWSPERSTRIP, 1);

    if (0 != ctx.x_resolution () && 0 != ctx.y_resolution ())
      {
        TIFFSetField (_tiff, TIFFTAG_XRESOLUTION, float (ctx.x_resolution ()));
        TIFFSetField (_tiff, TIFFTAG_YRESOLUTION, float (ctx.y_resolution ()));
        TIFFSetField (_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
      }

    TIFFSetField (_tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  }

} // namespace _out_
} // namespace utsushi
