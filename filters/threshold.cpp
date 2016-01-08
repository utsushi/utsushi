//  threshold.cpp -- apply a threshold to 8-bit grayscale data
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

#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>

#include "threshold.hpp"

namespace utsushi {
namespace _flt_ {

using std::invalid_argument;
using std::logic_error;

threshold::threshold ()
{
  option_->add_options ()
    ("threshold", (from< range > ()
                   -> lower (0)
                   -> upper (255)
                   -> default_value (128)
                   ),
     attributes (tag::enhancement),
     SEC_N_("Threshold")
     );
}

streamsize
threshold::write (const octet *data, streamsize n)
{
  octet *out = new octet[n];
  streamsize gray_count = 0;
  streamsize mono_count = 0;
  streamsize rv = 0;
  quantity v = value ((*option_)["threshold"]);
  gray_count = filter (data, out, n, ctx_.width (),
                       v.amount< unsigned char > ());

  // assumption: scanlines = 1
  mono_count = gray_count / 8 + (gray_count % 8 ? 1 : 0);       // ceil()
  rv = output_->write (out, mono_count);

  delete [] out;

  if (rv < mono_count) return rv * 8; // assumption: scanlines = 1
  return gray_count;
}

void
threshold::boi (const context& ctx)
{
  if (8 != ctx.depth ()) {
    BOOST_THROW_EXCEPTION
      (invalid_argument ("8-bits per channel required!"));
  }

  if (1 != ctx.comps ()) {
    BOOST_THROW_EXCEPTION
      (invalid_argument ("Invalid number of components!"));
  }

  ctx_ = ctx;
  ctx_.depth (1);
}

void
threshold::set_bit (octet *data, streamsize bit_index, bool is_below)
{
  streamsize toctet = bit_index / 8;         // target octet
  streamsize tbit   = 7 - (bit_index % 8);   // target bit

  if (is_below) {
    data[toctet] &= ~(1 << tbit);
  } else {
    data[toctet] |=  (1 << tbit);
  }
}

//! Returns the number of pixels consumed from the input
streamsize
threshold::filter (const octet *in_data,
                   octet *out_data,
                   streamsize n,            // size of data buffers
                   streamsize ppl,          // pixels per line
                   unsigned char threshold)
{
  if (0 == n) return 0;
  if (0 == ppl) return 0;
  if (ppl > n)
    BOOST_THROW_EXCEPTION
      (logic_error ("not enough data to generate a line of output"));

  //! \todo fix processing more than a single scanline at a time
  streamsize lines = 1;
  streamsize pad = 8 - ppl%8;

  for (streamsize v=0; v<lines; ++v) {
    for (streamsize h=0; h<ppl; ++h) {
      set_bit (out_data, v*(ppl+pad) + h,
               (unsigned char)in_data[v*ppl + h] < threshold);
    }
  }

  return lines*ppl;
}

}       // namespace _flt_
}       // namespace utsushi
