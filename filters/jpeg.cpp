//  jpeg.cpp -- JPEG image format support
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
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

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>

#include "jpeg.hpp"

namespace utsushi {
namespace _flt_ {

using std::invalid_argument;
using std::runtime_error;

static const size_t buf_size = 512 * 1024;

jpeg::jpeg ()
  : jbuf_size_(buf_size)
{
  jbuf_ = new JOCTET[jbuf_size_];

  // Set up the minimal information that might be useful for our error
  // handler before creating a compressor.  The jpeg_create_compress()
  // call tries to allocate memory and will jump to the error handler
  // when memory is not available.

  cinfo_.client_data = this;
  cinfo_.err = jpeg_std_error (&jerr_);
  jerr_.error_exit = &jpeg::error_exit_callback;

  jpeg_create_compress (&cinfo_);

  // Set up the destination manager

  dmgr_.next_output_byte = jbuf_;
  dmgr_.free_in_buffer   = jbuf_size_;

  dmgr_.init_destination    = &jpeg::init_destination_callback;
  dmgr_.empty_output_buffer = &jpeg::empty_output_buffer_callback;
  dmgr_.term_destination    = &jpeg::term_destination_callback;

  cinfo_.dest = &dmgr_;

  // The default JPEG image quality value is buried in the libjpeg.txt
  // file somewhere.

  option_->add_options ()
    ( "quality", (from< range > ()
                     -> lower (0)
                     -> upper (100)
                     -> default_value (75)
      ),
      attributes (),
      N_("Image Quality")
      );
}

jpeg::~jpeg ()
{
  delete [] jbuf_;
}

streamsize
jpeg::write (const octet *data, streamsize n)
{
  BOOST_ASSERT ((data && 0 < n) || 0 == n);

  JDIMENSION row_stride = 0;
  JDIMENSION in_rows    = 0;
  JDIMENSION out_rows   = 0;

  row_stride = cinfo_.image_width * cinfo_.input_components;
  BOOST_ASSERT (0 != row_stride);
  in_rows    = n / row_stride;

  if (1 > in_rows) return 0; // need at least one line of data

  /*  :FIXME: writing >1 scanline at a time fails with memory access
   *  violation (because the address of an octet* is \e not the same
   *  kind of thing as the address of an array of JSAMPLE*'s).
   */
  const JSAMPLE **jsample = reinterpret_cast<const JSAMPLE **> (&data);
  out_rows = jpeg_write_scanlines (&cinfo_,
                                   const_cast<JSAMPLE **> (jsample), 1);
  return out_rows * row_stride;
}

void
jpeg::boi (const context& ctx)
{
  if (ctx.height () <= 0) {
    BOOST_THROW_EXCEPTION
      (invalid_argument (_("JPEG requires a height!")));
  }

  if (ctx.depth () != 8) {
    BOOST_THROW_EXCEPTION
      (invalid_argument (_("JPEG requires 8-bits per channel!")));
  }

  if (1 == ctx.comps ()) {
    cinfo_.in_color_space = JCS_GRAYSCALE;
    cinfo_.input_components = 1;
  } else if (3 == ctx.comps ()) {
    cinfo_.in_color_space = JCS_RGB;
    cinfo_.input_components = 3;
  } else {
    BOOST_THROW_EXCEPTION
      (invalid_argument (_("Invalid number of components for JPEG!")));
  }

  cinfo_.image_width = ctx.width ();
  cinfo_.image_height = ctx.height ();

  quantity v = value ((*option_)["quality"]);
  jpeg_set_defaults (&cinfo_);
  jpeg_set_quality (&cinfo_, v.amount< int > (), true);

  jpeg_start_compress (&cinfo_, true);

  ctx_ = ctx;
  ctx_.media_type ("image/jpeg");
}

void
jpeg::eoi (const context& ctx)
{
  jpeg_finish_compress (&cinfo_);
}

void
jpeg::init_destination (j_compress_ptr cinfo)
{
  cinfo->dest->next_output_byte = jbuf_;
  cinfo->dest->free_in_buffer = jbuf_size_;
}

/*! \todo Need to handle the case where n != jbuf_size_ by keeping the
 *        unwritten bytes in the buffer for next time
 */
boolean
jpeg::empty_output_buffer (j_compress_ptr cinfo)
{
  streamsize n = io_->write (reinterpret_cast<octet *> (jbuf_),
                             jbuf_size_);
  if (n != streamsize (jbuf_size_))
    BOOST_THROW_EXCEPTION
      (std::ios_base::failure (_("JPEG filter octet count mismatch "
                                 "while emptying the output buffer")));

  cinfo->dest->next_output_byte = jbuf_;
  cinfo->dest->free_in_buffer = jbuf_size_;

  return true;
}

/*! \todo Need to handle the case where n != s somehow.
 *        Attempting to write in a loop could be problematic if the next
 *        filter in the chain is never satisfied with the number of bytes
 *        it receives.
 */
void
jpeg::term_destination (j_compress_ptr cinfo)
{
  size_t s = jbuf_size_ - cinfo->dest->free_in_buffer;
  size_t n = io_->write (reinterpret_cast<octet *> (jbuf_),
                         s);
  if (n != s)
    BOOST_THROW_EXCEPTION
      (std::ios_base::failure (_("JPEG filter octet count mismatch "
                                 "while finishing up the image")));
}

void
jpeg::init_destination_callback (j_compress_ptr cinfo)
{
  jpeg *j = static_cast<jpeg *> (cinfo->client_data);
  j->init_destination (cinfo);
}

boolean
jpeg::empty_output_buffer_callback (j_compress_ptr cinfo)
{
  jpeg *j = static_cast<jpeg *> (cinfo->client_data);
  return j->empty_output_buffer (cinfo);
}

void
jpeg::term_destination_callback (j_compress_ptr cinfo)
{
  jpeg *j = static_cast<jpeg *> (cinfo->client_data);
  j->term_destination (cinfo);
}

//! \todo unstub this
void
jpeg::error_exit (j_common_ptr cinfo)
{
  BOOST_THROW_EXCEPTION
    (runtime_error (_("JPEG filter encountered fatal error")));
}

void
jpeg::error_exit_callback (j_common_ptr cinfo)
{
  jpeg *j = static_cast<jpeg *> (cinfo->client_data);
  j->error_exit (cinfo);
}

}       // namespace _flt_
}       // namespace utsushi
