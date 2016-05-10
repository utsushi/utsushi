//  image-skip.cpp -- conditionally suppress images in the output
//  Copyright (C) 2013-2015  SEIKO EPSON CORPORATION
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

#include "image-skip.hpp"

#include <utsushi/cstdint.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/quantity.hpp>
#include <utsushi/range.hpp>
#include <utsushi/store.hpp>

#include <limits>

namespace utsushi {
namespace _flt_ {

struct bucket
{
  octet     *data_;
  streamsize size_;
  bool       seen_;

  bucket (const octet *data, streamsize size)
    : data_(new octet[size])
    , size_(size)
    , seen_(false)
  {
    traits::copy (data_, data, size);
  }

  ~bucket ()
  {
    delete [] data_;
  }
};

image_skip::image_skip ()
{
  option_->add_options ()
    ("blank-threshold", (from< range > ()
                         -> lower (  0.)
                         -> upper (100.)
                         -> default_value (0.)
                         ),
     attributes (tag::enhancement)(level::standard),
     SEC_N_("Skip Blank Pages Settings")
     )
    ;
}

// Our marker handlers decide when to call output_->mark() and produce any
// image data.  We always use the most up-to-date context information.
// That means that the end-of context replaces the begin-of one.
void
image_skip::mark (traits::int_type c, const context& ctx)
{
  ctx_ = ctx;
  output::mark (c, ctx_);       // calls our marker handlers
}

streamsize
image_skip::write (const octet *data, streamsize n)
{
  pool_.push_back (make_shared< bucket > (data, n));

  // When area of interest is supported we need to know the width
  // before we can do any processing.
  if (context::unknown_size != ctx_.width ())
    {
      process_(pool_.back ());

      // for a tile based algorithm we can start writing data as soon
      // as the first non-blank tile has been found.
    }
  return n;
}

void
image_skip::bos (const context& ctx)
{
  quantity q = value ((*option_)["blank-threshold"]);
  threshold_ = q.amount< double > ();

  last_marker_ = traits::eos ();
}

void
image_skip::boi (const context& ctx)
{
  // \todo remove limitations
  BOOST_ASSERT (8 == ctx_.depth ());

  // Achieved via e.g. jpeg::decompressor
  BOOST_ASSERT (ctx_.is_raster_image ());
  // These are easily achieved by using a padding filter!
  BOOST_ASSERT (0 == ctx_.padding_octets ());
  BOOST_ASSERT (0 == ctx_.padding_lines ());

  BOOST_ASSERT (pool_.empty ());

  darkness_ = 0;
}

void
image_skip::eoi (const context& ctx)
{
  if (!skip_())
    {
      if (!pool_.empty ())
        {
          if (traits::eos () == last_marker_)
            {
              last_marker_ = traits::bos ();
              output_->mark (last_marker_, ctx_);
            }
          if (   traits::bos () == last_marker_
              || traits::eoi () == last_marker_)
            {
              last_marker_ = traits::boi ();
              output_->mark (last_marker_, ctx_);
            }
        }
      while (!pool_.empty ())
        {
          shared_ptr< bucket > p = pool_.front ();
          pool_.pop_front ();
          if (p)
            output_->write (p->data_, p->size_);
        }
      if (last_marker_ == traits::boi ())
        {
          last_marker_ = traits::eoi ();
          output_->mark (last_marker_, ctx_);
        }
    }
  else
    {
      pool_.clear ();
    }
}

void
image_skip::eos (const context& ctx)
{
  if (traits::eos () == last_marker_)
    output_->mark (traits::bos (), ctx_);
  output_->mark (traits::eos (), ctx);
}

void
image_skip::eof (const context& ctx)
{
  output_->mark (traits::eof (), ctx);
}

bool
image_skip::skip_()
{
  std::deque< shared_ptr< bucket > >::iterator it;
  for (it = pool_.begin (); pool_.end () != it; ++it)
    {
      if (!(*it)->seen_) process_(*it);
    }

  return 100 * darkness_ <= threshold_ * ctx_.octets_per_image ();
}

void
image_skip::process_(shared_ptr< bucket > bp)
{
  if (!bp) return;

  int scale_factor = std::numeric_limits< uint8_t >::max ();
  int bucket_sum = bp->size_ * scale_factor;

  octet *p = bp->data_;
  while (p < bp->data_ + bp->size_)
    {
      bucket_sum -= uint8_t (*p++);
    }
  bp->seen_ = true;

  darkness_ += double (bucket_sum) / scale_factor;
}

}       // namespace _flt_
}       // namespace utsushi
