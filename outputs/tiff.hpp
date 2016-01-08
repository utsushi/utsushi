//  tiff.hpp -- TIFF image file format support
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef outputs_tiff_hpp_
#define outputs_tiff_hpp_

#include <utsushi/file.hpp>

#include <boost/scoped_array.hpp>

#include <string>

#include <tiffio.h>

namespace utsushi {
namespace _out_ {

class tiff_odevice
  : public file_odevice
{
public:
  tiff_odevice (const std::string& filename);
  tiff_odevice (const path_generator& generator);

  ~tiff_odevice ();

  streamsize write (const octet *data, streamsize n);

protected:
  void open ();
  void close ();

  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);

private:
  TIFF   *tiff_;
  uint32  page_;
  uint32  row_;

  boost::scoped_array< octet > partial_line_;
  streamsize                   partial_size_;

public:
  static std::string err_msg;
};

}       // namespace _out_
}       // namespace utsushi

#endif  /* outputs_tiff_hpp_ */
