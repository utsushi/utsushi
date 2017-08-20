//  jpeg.cpp -- JPEG image format support
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

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <new>

#include <boost/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/scoped_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/range.hpp>

#include "jpeg.hpp"

using std::bad_alloc;
using std::min;
using std::nothrow;
using std::runtime_error;

using boost::scoped_array;

namespace utsushi {
namespace _flt_ {
namespace jpeg {

using detail::common;

//! Callback wrappers for use by the JPEG API.
/*! These C-style wrapper function have been collected in a struct so
 *  we can give them access to protected (de)compressor API.  This is
 *  achieved by declaring the callback structure a \c friend of that
 *  class.  All wrappers perform a sanity check on the sole argument
 *  passed before forwarding the call to the (de)compressor instance.
 */
struct callback
{
  static void
  error_exit_(j_common_ptr cinfo)
  {
    using detail::decompressor;

    common *self;
    if (cinfo->is_decompressor)
      self = static_cast< decompressor * > (cinfo->client_data);
    else
      self = static_cast<   compressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo->err == &self->jerr_);
    self->error_exit (cinfo);
  }

  static void
  output_message_(j_common_ptr cinfo)
  {
    using detail::decompressor;

    common *self;
    if (cinfo->is_decompressor)
      self = static_cast< decompressor * > (cinfo->client_data);
    else
      self = static_cast<   compressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo->err == &self->jerr_);
    self->output_message (cinfo);
  }

  static void
  init_destination_(j_compress_ptr cinfo)
  {
    compressor *self = static_cast< compressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    self->init_destination ();
  }

  static boolean
  empty_output_buffer_(j_compress_ptr cinfo)
  {
    compressor *self = static_cast< compressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    return self->empty_output_buffer ();
  }

  static void
  term_destination_(j_compress_ptr cinfo)
  {
    compressor *self = static_cast< compressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    self->term_destination ();
  }

  static void
  init_source_(j_decompress_ptr cinfo)
  {
    using detail::decompressor;

    decompressor *self = static_cast< decompressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    self->init_source ();
  }

  static boolean
  fill_input_buffer_(j_decompress_ptr cinfo)
  {
    using detail::decompressor;

    decompressor *self = static_cast< decompressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    return self->fill_input_buffer ();
  }

  static void
  skip_input_data_(j_decompress_ptr cinfo, long num_bytes)
  {
    using detail::decompressor;

    decompressor *self = static_cast< decompressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    self->skip_input_data (num_bytes);
  }

  static void
  term_source_(j_decompress_ptr cinfo)
  {
    using detail::decompressor;

    decompressor *self = static_cast< decompressor * > (cinfo->client_data);
    BOOST_ASSERT (cinfo == &self->cinfo_);
    self->term_source ();
  }
};

common::common ()
  : jbuf_(nullptr)
  , jbuf_size_(0)
{
  jpeg_std_error (&jerr_);
  jerr_.error_exit     = &callback::error_exit_;
  jerr_.output_message = &callback::output_message_;

  resize (default_buffer_size);
}

common::~common ()
{
  delete [] jbuf_;
}

void
common::resize (size_t buf_size)
{
  if (jbuf_size_ < buf_size)
    {
      JOCTET *buf = new (nothrow) JOCTET[buf_size];
      if (buf)
        {
          delete [] jbuf_;

          jbuf_      = buf;
          jbuf_size_ = buf_size;
        }
      else
        {
          log::error
            ("could not acquire %1% byte JPEG work buffer")
            % buf_size
            ;
        }
    }
}

void
common::error_exit (j_common_ptr cinfo)
{
  char msg[JMSG_LENGTH_MAX];

  jerr_.format_message (cinfo, msg);
  jpeg_destroy (cinfo);

  log::fatal (msg);

  BOOST_THROW_EXCEPTION (runtime_error (msg));
}

void
common::output_message (j_common_ptr cinfo)
{
  char msg[JMSG_LENGTH_MAX];

  jerr_.format_message (cinfo, msg);

  log::error (msg);
}

void
common::add_buffer_size_(option::map::ptr om_)
{
  om_->add_options ()
    ("buffer-size", (from< range > ()
                     -> lower (default_buffer_size)
                     -> upper (default_buffer_size * 64)
                     -> default_value (default_buffer_size)
                     ),
     attributes (level::complete),
     CCB_N_("Buffer Size")
     )
    ;
}

compressor::compressor ()
  : quality_(75)                // buried in libjpeg.txt somewhere
  , cache_(nullptr)
  , cache_size_(0)
  , cache_fill_(0)
{
  // Set up filter specific options

  common::add_buffer_size_(option_);
  option_->add_options ()
    ("quality", (from< range > ()
                 -> lower (  0)
                 -> upper (100)
                 -> default_value (quality_)
                 ),
     attributes (),
     CCB_N_("Image Quality")
     )
    ;

  // Set up the minimal information that might be useful for our error
  // handler before creating a compressor.  The jpeg_create_compress()
  // call tries to allocate memory and will jump to the error handler
  // when memory is not available.

  cinfo_.client_data = this;
  cinfo_.err         = &jerr_;

  jpeg_create_compress (&cinfo_);

  // Set up the destination manager callbacks

  dmgr_.init_destination    = &callback::init_destination_;
  dmgr_.empty_output_buffer = &callback::empty_output_buffer_;
  dmgr_.term_destination    = &callback::term_destination_;

  cinfo_.dest = &dmgr_;
}

compressor::~compressor ()
{
  if (cache_size_)
    delete [] cache_;
  jpeg_destroy_compress (&cinfo_);
}

streamsize
compressor::write (const octet *data, streamsize n)
{
  BOOST_ASSERT ((data && 0 < n) || 0 == n);

  BOOST_ASSERT (0 <= cache_fill_ && cache_fill_ <= cache_size_);

  streamsize rv = n;            // we consume all data

  if (cache_fill_ && cache_fill_ != cache_size_)
    {
      streamsize count = min (n, cache_size_ - cache_fill_);

      memcpy (cache_ + cache_fill_, data, count);
      data += count;
      n    -= count;
      cache_fill_ += count;

      if (cache_fill_ != cache_size_)
        return rv;
    }

  // Create an array of pointers to scan lines as per JPEG library
  // expectations.  It is unfortunate that the JPEG library is not
  // const correct and settled on a JSAMPLE type that is not quite
  // compatible with our octet type.  As long as their respective
  // sizes are the same, the type issue is of no concern.

  BOOST_STATIC_ASSERT ((sizeof (JSAMPLE) == sizeof (octet)));

  JDIMENSION in_rows  = n / ctx_.octets_per_line ();
  if (cache_fill_ == cache_size_) ++in_rows;

  scoped_array< JSAMPLE * > rows (new JSAMPLE *[in_rows]);
  JDIMENSION i = 0;
  if (cache_fill_ == cache_size_)
    {
      rows[i] = reinterpret_cast< JSAMPLE * > (cache_);
      ++i;
    }
  for (; i < in_rows; ++i)
    {
      rows[i] = const_cast< JSAMPLE * >
        (reinterpret_cast< const JSAMPLE * > (data));
      data += ctx_.octets_per_line ();
      n    -= ctx_.octets_per_line ();
    }

  for (JDIMENSION out_rows = 0; out_rows < in_rows;)
    {
      out_rows += jpeg_write_scanlines (&cinfo_, rows.get () + out_rows,
                                        in_rows - out_rows);
    }

  cache_fill_ = 0;

  if (0 < n)
    {
      memcpy (cache_, data, n);
      cache_fill_ = n;
    }

  return rv;
}

void
compressor::bos (const context& ctx)
{
  // Use the same quality for all images in a sequence

  quantity q = value ((*option_)["quality"]);
  quality_ = q.amount< int > ();

  // Resize the work buffer only if necessary

  quantity sz = value ((*option_)["buffer-size"]);
  resize (sz.amount< int > ());

  if (!jbuf_)
    {
      log::fatal ("could not create JPEG work buffer");
      BOOST_THROW_EXCEPTION (bad_alloc ());
    }

  log::trace
    ("using %1% byte JPEG work buffer")
    % jbuf_size_
    ;

  dmgr_.next_output_byte = jbuf_;
  dmgr_.free_in_buffer   = jbuf_size_;
}

void
compressor::boi (const context& ctx)
{
  // Validate image size assumptions
  // Note that the JPEG format can in principle handle images with
  // unknown up-front height (via its DNL marker) but this is *not*
  // support by the JPEG library.

  BOOST_ASSERT (0 < ctx.width ());
  BOOST_ASSERT (0 < ctx.height ());
  BOOST_ASSERT (0 < ctx.octets_per_line ());

  // Validate pixel type assumptions

  BOOST_ASSERT (8 == ctx.depth ());
  BOOST_ASSERT (3 == ctx.comps () || 1 == ctx.comps ());

  ctx_ = ctx;
  ctx_.content_type ("image/jpeg");

  /**/ if (3 == ctx_.comps ())
    {
      cinfo_.in_color_space   = JCS_RGB;
      cinfo_.input_components = 3;
    }
  else if (1 == ctx_.comps ())
    {
      cinfo_.in_color_space   = JCS_GRAYSCALE;
      cinfo_.input_components = 1;
    }
  else
    {
      bool supported_jpeg_color_space (false);
      BOOST_ASSERT (supported_jpeg_color_space);
    }

  cinfo_.image_width  = ctx_.width ();
  cinfo_.image_height = ctx_.height ();

  jpeg_set_defaults (&cinfo_);
  jpeg_set_quality  (&cinfo_, quality_, true);

  cinfo_.density_unit = 1;      // in dpi
  cinfo_.X_density = ctx_.x_resolution ();
  cinfo_.Y_density = ctx_.y_resolution ();

  jpeg_start_compress (&cinfo_, true);

  cache_ = new octet[ctx_.octets_per_line ()];
  cache_size_ = ctx_.octets_per_line ();
  cache_fill_ = 0;
}

void
compressor::eoi (const context& ctx)
{
  BOOST_ASSERT (!cache_fill_);

  jpeg_finish_compress (&cinfo_);

  delete [] cache_;
  cache_size_ = 0;
}

void
compressor::init_destination ()
{
  dmgr_.next_output_byte = jbuf_;
  dmgr_.free_in_buffer   = jbuf_size_;
}

/*! \warning  The JPEG library documentation explicitly states that
 *            the implementation should ignore the current values of
 *            \c dmgr_.next_output_byte and \c dmgr_.free_in_buffer.
 */
boolean
compressor::empty_output_buffer ()
{
  BOOST_STATIC_ASSERT ((sizeof (JOCTET) == sizeof (octet)));

  octet *data = reinterpret_cast< octet * > (jbuf_);

  streamsize n = output_->write (data, jbuf_size_);

  if (0 == n)
    log::alert ("unable to empty JPEG buffer");

  traits::move (data, data + n, jbuf_size_ - n);

  dmgr_.next_output_byte = jbuf_ + (jbuf_size_ - n);
  dmgr_.free_in_buffer   = n;

  return true;
}

void
compressor::term_destination ()
{
  BOOST_STATIC_ASSERT ((sizeof (JOCTET) == sizeof (octet)));

  octet *data = reinterpret_cast< octet * > (jbuf_);

  size_t count = jbuf_size_ - dmgr_.free_in_buffer;
  size_t n = output_->write (data, count);

  while (0 != n && count != n)
    {
      data  += n;
      count -= n;
      n = output_->write (data, count);
    }

  if (0 == n)
    log::alert ("unable to flush JPEG output, %1% octets left") % count;
}

namespace detail {

decompressor::decompressor ()
  : header_done_(false)
  , decompressing_(false)
  , flushing_(false)
  , bytes_to_skip_(0)
  , sample_rows_(nullptr)
{
  // Set up minimally useful information for our error handler before
  // creating a decompressor.

  cinfo_.client_data = this;
  cinfo_.err         = &jerr_;

  jpeg_create_decompress (&cinfo_);

  // Set up the source manager callbacks
  // Note that we default the resync_to_restart() callback

  smgr_.init_source       = &callback::init_source_;
  smgr_.fill_input_buffer = &callback::fill_input_buffer_;
  smgr_.skip_input_data   = &callback::skip_input_data_;
  smgr_.term_source       = &callback::term_source_;

  cinfo_.src = &smgr_;

  smgr_.next_input_byte = jbuf_;
  smgr_.bytes_in_buffer = 0;
}

decompressor::~decompressor ()
{
  jpeg_destroy_decompress (&cinfo_);
}

void
decompressor::init_source ()
{
  reclaim_space ();
}

boolean
decompressor::fill_input_buffer ()
{
  reclaim_space ();

  //jpegデータの一部が分割されて送られてきても正常に動作するように変更

  return false;
}

void
decompressor::skip_input_data (long num_bytes)
{
  if (0 >= num_bytes) return;   // as per libjpeg documentation

  BOOST_STATIC_ASSERT
    ((LONG_MAX <= boost::integer_traits< size_t >::const_max));

  if (size_t (num_bytes) > smgr_.bytes_in_buffer)
    {
      bytes_to_skip_ = num_bytes - smgr_.bytes_in_buffer;
      smgr_.next_input_byte = jbuf_;
      smgr_.bytes_in_buffer = 0;
    }
  else
    {
      bytes_to_skip_ = 0;
      smgr_.next_input_byte += num_bytes;
      smgr_.bytes_in_buffer -= num_bytes;
      reclaim_space ();
    }
}

void
decompressor::term_source ()
{
}

bool
decompressor::reclaim_space ()
{
  memmove (jbuf_, smgr_.next_input_byte, smgr_.bytes_in_buffer);
  smgr_.next_input_byte = jbuf_;

  return (jbuf_size_ != smgr_.bytes_in_buffer);
}

bool
decompressor::read_header ()
{
  if (!header_done_)
    {
      if (JPEG_SUSPENDED == jpeg_read_header (&cinfo_, true))
        {
          log::trace ("jpeg_read_header suspended");
          if (!reclaim_space ())
            {
              string msg ("not enough space to read JPEG header");
              log::error (msg);
              BOOST_THROW_EXCEPTION (runtime_error (msg));
            }
          return header_done_;
        }

      log::trace ("read JPEG header");
      header_done_ = true;

      //! \todo Apply option values to cinfo_
    }
  return header_done_;
}

bool
decompressor::start_decompressing (const context& ctx)
{
  if (!decompressing_)
    {
      if (!jpeg_start_decompress (&cinfo_))
        {
          log::trace ("jpeg_start_decompress suspended");
          if (!reclaim_space ())
            {
              string msg ("not enough space to start JPEG decompression");
              log::error (msg);
              BOOST_THROW_EXCEPTION (runtime_error (msg));
            }
          return decompressing_;
        }

      log::trace ("started JPEG decompression");
      decompressing_ = true;

      /*! \todo Cross-check ctx_ settings.  Note that these can
       *        only be set at boi() or eoi() so it is too late
       *        to update them here.
       */

      sample_rows_ = new JSAMPROW[cinfo_.rec_outbuf_height];
      for (int i = 0; i < cinfo_.rec_outbuf_height; ++i)
        {
          sample_rows_[i] = new JSAMPLE[ctx.scan_width ()];
        }
    }
  return decompressing_;
}

void
decompressor::handle_bos (const option::map& om)
{
  // Resize the work buffer only if necessary

  quantity sz = value (const_cast< option::map& > (om)["buffer-size"]);
  resize (sz.amount< int > ());

  if (!jbuf_)
    {
      log::fatal ("could not create JPEG work buffer");
      BOOST_THROW_EXCEPTION (bad_alloc ());
    }

  log::trace
    ("using %1% byte JPEG work buffer")
    % jbuf_size_
    ;

  smgr_.next_input_byte = jbuf_;
  smgr_.bytes_in_buffer = 0;
}

context
decompressor::handle_boi (const context& ctx)
{
  BOOST_ASSERT ("image/jpeg" == ctx.content_type ());

  context ctx_(ctx);
  ctx_.content_type ("image/x-raster");

  header_done_   = false;
  decompressing_ = false;
  flushing_      = false;

  return ctx_;
}

void
decompressor::handle_eoi ()
{
  for (int i = 0; i < cinfo_.rec_outbuf_height; ++i)
    delete [] sample_rows_[i];
  delete [] sample_rows_;
  sample_rows_ = nullptr;

  if (cinfo_.output_scanline < cinfo_.output_height)
    {
      log::error ("JPEG decompressor did not receive all scanlines");
      jpeg_abort_decompress (&cinfo_);
    }
  else
    {
      if (!jpeg_finish_decompress (&cinfo_))
        {
          log::error ("JPEG decompressor failed to finish cleanly");

          // If we get here the decompressor was suspended and needs
          // more image data.  There is no more image data and all the
          // scan lines have been handled as well (as per previous if
          // statement), so there is something very wrong internally.
        }
    }

  // Ensure that the decompressor starts off on the right footing for
  // the next image.  We do not create a new decompressor with every
  // image and the jpeg_start_decompress()/jpeg_finish_decompress() is
  // either dropping the ball here or assuming that the image data is
  // perfectly specification compliant.

  if (smgr_.bytes_in_buffer)
    {
      log::error
        ("Corrupt JPEG data: %1% extraneous bytes after marker 0xd9")
        % smgr_.bytes_in_buffer;

      smgr_.next_input_byte = jbuf_;
      smgr_.bytes_in_buffer = 0;
    }

  decompressing_ = false;
  header_done_   = false;
}

}       // namespace detail

decompressor::decompressor ()
{
  // Set up filter specific options

  common::add_buffer_size_(option_);
}

streamsize
decompressor::write (const octet *data, streamsize n)
{
  size_t left = n;

  if (n > bytes_to_skip_)
    {
      data += bytes_to_skip_;
      left -= bytes_to_skip_;
    }
  else
    {
      bytes_to_skip_ -= n;
      left -= n;
    }

  while (0 < left
         && (!decompressing_
             || cinfo_.output_scanline < cinfo_.output_height))
    {
      JOCTET *next_free_byte = const_cast< JOCTET * >
        (smgr_.next_input_byte + smgr_.bytes_in_buffer);

      size_t bytes_free = (jbuf_size_ - (next_free_byte - jbuf_));
      size_t copy_bytes = min (left, bytes_free);

      memcpy (next_free_byte, data, copy_bytes);
      smgr_.bytes_in_buffer += copy_bytes;
      data += copy_bytes;
      left -= copy_bytes;

      if (!read_header ())             return n - left;
      if (!start_decompressing (ctx_)) return n - left;

      // Write as many decompressed scanlines to output as the
      // decompressor is willing to provide.

      int count;
      while (cinfo_.output_scanline < cinfo_.output_height
             && (count = jpeg_read_scanlines (&cinfo_, sample_rows_,
                                              cinfo_.rec_outbuf_height)))
        {
          for (int i = 0; i < count; ++i)
            {
              BOOST_STATIC_ASSERT ((sizeof (JSAMPLE) == sizeof (octet)));

              octet *line = reinterpret_cast< octet * > (sample_rows_[i]);
              size_t cnt  = ctx_.scan_width ();
              size_t n    = output_->write (line, cnt);

              while (0 != n && cnt != n)
                {
                  line += n;
                  cnt  -= n;
                  n = output_->write (line, cnt);
                }

              if (0 == n)
                log::alert ("unable to write decompressed JPEG output,"
                            " dropping %1% octets") % cnt;
            }
        }
    }

  reclaim_space ();

  if (cinfo_.output_scanline < cinfo_.output_height)
    return n - left;

  // If we get here, there is junk between the last image data and the
  // JPEG EOI marker.  We skip this so we can produce proper output as
  // well as continue with the next image, if any.

  int state = !JPEG_REACHED_EOI;
  while (0 < left
         && JPEG_REACHED_EOI != state)
    {
      JOCTET *next_free_byte = const_cast< JOCTET * >
        (smgr_.next_input_byte + smgr_.bytes_in_buffer);

      size_t bytes_free = (jbuf_size_ - (next_free_byte - jbuf_));
      size_t copy_bytes = min (left, bytes_free);

      memcpy (next_free_byte, data, copy_bytes);
      smgr_.bytes_in_buffer += copy_bytes;
      data += copy_bytes;
      left -= copy_bytes;
      state = jpeg_consume_input (&cinfo_);
    }
  return n - left;
}

void
decompressor::bos (const context& ctx)
{
  handle_bos (*option_);
}

void
decompressor::boi (const context& ctx)
{
  ctx_ = handle_boi (ctx);
}

void
decompressor::eoi (const context& ctx)
{
  handle_eoi ();
}

}       // namespace jpeg
}       // namespace _flt_
}       // namespace utsushi
