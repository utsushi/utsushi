//  pnm.cpp -- PNM image format support
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

#include <stdexcept>

#include <boost/scoped_array.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/format.hpp>

#include "pnm.hpp"

namespace utsushi {
namespace _flt_ {

streamsize
pnm::write (const octet *data, streamsize n)
{
  if (1 != ctx_.depth ())       // PGM or PPM
    return output_->write (data, n);

  boost::scoped_array< octet > tmp (new octet[n]);      // PBM
  for (streamsize i = 0; i < n; ++i)
    {
      tmp[i] = ~data[i];
    }
  return output_->write (tmp.get (), n);
}

void
pnm::boi (const context& ctx)
{
  using std::logic_error;

  logic_error e ("'pnm' needs to know image size upfront");

  if (context::unknown_size == ctx.width  ()) { BOOST_THROW_EXCEPTION (e); }
  if (context::unknown_size == ctx.height ()) { BOOST_THROW_EXCEPTION (e); }

  format fmt;

  if (8 == ctx.depth()) {
    if (3 == ctx.comps()) {
      fmt = format ("P6 %1% %2% 255\n");
    } else if (1 == ctx.comps()) {
      fmt = format ("P5 %1% %2% 255\n");
    }
  } else if (1 == ctx.depth() && 1 == ctx.comps()) {
    fmt = format ("P4 %1% %2%\n");
  }

  if (0 == fmt.size()) {
    BOOST_THROW_EXCEPTION
      (logic_error ((format ("'pnm' cannot handle images with "
                             "%1% pixel components each using "
                             " a bit depth of %2%")
                     % ctx.comps()
                     % ctx.depth()).str ()));
  }

  ctx_ = ctx;
  ctx_.content_type ("image/x-portable-anymap");

  std::string header = (fmt % ctx_.width() % ctx_.height()).str();
  output_->write (header.c_str (), header.length ());
}

}       // namespace _flt_
}       // namespace utsushi
