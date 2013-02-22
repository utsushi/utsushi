//  lut.cpp -- look-up table based filtering support
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

#include <algorithm>
#include <stdexcept>

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>

#include "lut.hpp"

namespace utsushi {
namespace _flt_ {

using std::logic_error;

lut::lut ()
  : lut_(NULL), rows_(256), opr_(1)
{}

streamsize
lut::write (const octet *data, streamsize n)
{
  index_type i;
  octet *tmp = new octet[n];
  streamsize mapped = 0;

  for (streamsize j=0; j<n; j+=opr_)
  {
    i = octets2index ((data+j), opr_);
    i = std::min (i, rows_ - 1);
    index2octets ((tmp+j), lut_[i], opr_);
    mapped += opr_;
  }

  streamsize rv = io_->write (tmp, mapped);
  delete [] tmp;
  return rv;
}

lut::index_type
lut::octets2index (const octet *o, streamsize n)
{
  index_type i (0);

  for (streamsize j=0; j<n; ++j)
  {
    i <<= 8;
    i |= o[j] & 0xff;
  }

  return i;
}

void
lut::index2octets (octet *o, index_type i, streamsize n)
{
  for (streamsize j=n-1; j>=0; --j)
  {
    o[j] = i & 0xff;
    i >>= 8;
  }

  return;
}

void
lut::init_lut ()
{
  for (index_type i=0; i<rows_; ++i)
  {
    lut_[i] = i;
  }
}

void
lut::boi (const context& ctx)
{
  if (8 != ctx.depth () && 16 != ctx.depth ())
    BOOST_THROW_EXCEPTION
      (logic_error (_("lut filter supports 8 or 16 bit only.")));

  rows_ = 1 << ctx.depth ();
  opr_ = ctx.depth () / 8;

  lut_ = new index_type[rows_];

  init_lut ();

  ctx_ = ctx;
}

void
lut::eoi (const context& ctx)
{
  delete [] lut_;
}

bc_lut::bc_lut (double brightness, double contrast)
{
  option_->add_options ()
    ( "brightness", (from< range > ()
                     -> lower (-1.0)
                     -> upper (1.0)
                     -> default_value (brightness)
      ),
      attributes (tag::enhancement),
      N_("Brightness"),
      N_("Change brightness of the acquired image.")
      )
    ( "contrast", (from< range > ()
                     -> lower (-1.0)
                     -> upper (1.0)
                     -> default_value (contrast)
      ),
      attributes (tag::enhancement),
      N_("Contrast"),
      N_("Change contrast of the acquired image.")
      );
}

void
bc_lut::init_lut ()
{
  quantity bv = value ((*option_)["brightness"]);
  quantity cv = value ((*option_)["contrast"]);

  index_type cap (rows_ - 1);
  index_type b = bv.amount< double > () * 0.5 * cap;
  index_type c = cv.amount< double > () * 0.5 * cap;

  for (index_type i=0; i<rows_; ++i)
  {
    index_type val =  (cap * (i - c)) / (cap - 2 * c) + b;

    lut_[i] = std::min (cap, std::max (val, index_type (0)));
  }
}

}       // namespace _flt_
}       // namespace utsushi
