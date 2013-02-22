//  jpeg.hpp -- JPEG image format support
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


#ifndef filters_jpeg_hpp_
#define filters_jpeg_hpp_

#include <jpeglib.h>

#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {

//!  Turn a sequence of image data into JPEG format
/*!  Currently only supports 8-bit images of constant size.
 *
 *   \todo Consider function objects instead of static members for the
 *         callbacks that libjpeg wants.
 */
class jpeg
  : public ofilter
{
public:
  jpeg ();
  ~jpeg ();

  streamsize write (const octet *data, streamsize n);

protected:
  void boi (const context& ctx);
  void eoi (const context& ctx);

  struct jpeg_compress_struct cinfo_;
  struct jpeg_error_mgr       jerr_;
  struct jpeg_destination_mgr dmgr_;

  JOCTET *jbuf_;
  size_t  jbuf_size_;

  void    init_destination    (j_compress_ptr cinfo);
  boolean empty_output_buffer (j_compress_ptr cinfo);
  void    term_destination    (j_compress_ptr cinfo);

  static void    init_destination_callback    (j_compress_ptr cinfo);
  static boolean empty_output_buffer_callback (j_compress_ptr cinfo);
  static void    term_destination_callback    (j_compress_ptr cinfo);

  void error_exit (j_common_ptr cinfo) __attribute__((noreturn));

  static void error_exit_callback (j_common_ptr cinfo);
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_jpeg_hpp_ */
