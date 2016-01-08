//  jpeg.hpp -- JPEG image format support
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

#ifndef filters_jpeg_hpp_
#define filters_jpeg_hpp_

#include <jpeglib.h>

#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {
namespace jpeg {

namespace detail {

struct common
{
  common ();
  ~common ();

  //! Work buffer for use by the destination or source manager
  JOCTET *jbuf_;

  //! Work buffer size
  /*! The value is configurable at run-time.  At the start of each
   *  sequence an attempt to increase the work buffer's size is made
   *  if necessary.  In case of failure, the existing work buffer is
   *  used as is.
   */
  size_t  jbuf_size_;

  //! Attempt to grow the work buffer to \a buf_size octets
  /*! If the attempt fails, jbuf_ and jbuf_size_ remain unchanged.
   *  Otherwise, these variables will be updated to correspond to the
   *  resized buffer.
   */
  void resize (size_t buf_size);

  struct jpeg_error_mgr jerr_;

  void error_exit (j_common_ptr cinfo) __attribute__((noreturn));
  void output_message (j_common_ptr cinfo);

  static void add_buffer_size_(option::map::ptr om);
};

}       // namespace detail

//!  Turn a sequence of image data into JPEG format
class compressor
  : public filter
  , protected detail::common
{
public:
  compressor ();
  ~compressor ();

  streamsize write (const octet *data, streamsize n);

protected:
  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);

  //! JPEG image quality to use during a single sequence
  /*! The value is configurable at run-time and fixed at start of
   *  sequence.
   */
  int quality_;

  struct jpeg_compress_struct cinfo_;
  struct jpeg_destination_mgr dmgr_;

  void    init_destination ();
  boolean empty_output_buffer ();
  void    term_destination ();

  octet     *cache_;
  streamsize cache_size_;
  streamsize cache_fill_;

  friend struct callback;
};

namespace detail {

struct decompressor
  : common
{
  decompressor ();
  ~decompressor ();

  void    init_source ();
  boolean fill_input_buffer ();
  void    skip_input_data (long num_bytes);
  void    term_source ();

  //! Try to reclaim unused work buffer space
  /*! Returns \c true if there is \e usable free space in the work
   *  buffer after reclamation.
   *
   *  \note This does \e not return resources to the system.  It only
   *        rearranges buffer content so that the decompressor stands
   *        a chance when it tries to append data.
   */
  bool reclaim_space ();

  bool read_header ();
  bool start_decompressing (const context& ctx);

  void    handle_bos (const option::map& om);
  context handle_boi (const context& ctx);
  void    handle_eoi ();

  struct jpeg_decompress_struct cinfo_;
  struct jpeg_source_mgr        smgr_;

  bool header_done_;
  bool decompressing_;
  bool flushing_;

  streamsize bytes_to_skip_;

  JSAMPROW *sample_rows_;

  friend struct callback;
};

}       // namespace detail

//! Turn a sequence of JPEG data into raw image data
class decompressor
  : public filter
  , protected detail::decompressor
{
public:
  decompressor ();

  streamsize write (const octet *data, streamsize n);

protected:
  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
};

}       // namespace jpeg
}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_jpeg_hpp_ */
