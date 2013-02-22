//  tiff.hpp -- TIFF image file format support
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

#ifndef outputs_tiff_hpp_
#define outputs_tiff_hpp_

#include <tiffio.h>

#include <boost/scoped_array.hpp>

#include <utsushi/file.hpp>

namespace utsushi {
namespace _out_ {

  class tiff_odevice
    : public odevice
  {
  public:
    tiff_odevice (const fs::path& name);
    tiff_odevice (const path_generator& generator);

    streamsize write (const octet *data, streamsize n);

  protected:
    void bos (const context& ctx);
    void boi (const context& ctx);
    void eoi (const context& ctx);
    void eos (const context& ctx);

    boost::scoped_array< octet > partial_line_;
    streamsize                   partial_size_;

  private:
    fs::path name_;
    path_generator generator_;

    TIFF   *_tiff;
    uint32  _row;
    uint32  _page;
  };

} // namespace _out_
} // namespace utsushi

#endif /* outputs_tiff_hpp_ */
