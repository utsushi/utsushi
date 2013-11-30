//  image-skip.cpp -- conditionally suppress images in the output
//  Copyright (C) 2013  SEIKO EPSON CORPORATION
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
     N_("Blank Image Threshold")
     )
    ;
}

void
image_skip::mark (traits::int_type c, const context& ctx)
{
  ctx_ = ctx;
  output::mark (c, ctx_);
  if (traits::eos () == c)
    {
      if (traits::eos () == last_marker_)
        io_->mark (traits::bos (), ctx_);
      io_->mark (c, ctx_);
    }
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
              io_->mark (last_marker_, ctx_);
            }
          if (   traits::bos () == last_marker_
              || traits::eoi () == last_marker_)
            {
              last_marker_ = traits::boi ();
              io_->mark (last_marker_, ctx_);
            }
        }
      while (!pool_.empty ())
        {
          shared_ptr< bucket > p = pool_.front ();
          pool_.pop_front ();
          if (p)
            io_->write (p->data_, p->size_);
        }
      if (last_marker_ == traits::boi ())
        {
          last_marker_ = traits::eoi ();
          io_->mark (last_marker_, ctx_);
        }
    }
  else
    {
      pool_.clear ();
    }
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

iimage_skip::iimage_skip ()
  : release_(false)
{
  option_->add_options ()
    ("blank-threshold", (from< range > ()
                         -> lower (  0.)
                         -> upper (100.)
                         -> default_value (0.)
                         ),
     attributes (tag::enhancement)(level::standard),
     N_("Blank Image Threshold")
     )
    ;
}

streamsize
iimage_skip::read (octet *data, streamsize n)
{
  if (release_)
    {
      if (pool_.empty ()) return traits::eoi ();
      if (!data || 0 > n) return traits::not_marker (0);

      octet *p = pool_.front ()->data_;
      streamsize rv = std::min (n, pool_.front ()->size_);

      traits::copy (data, p, rv);
      if (rv < pool_.front ()->size_)
        {
          traits::move (p, p + rv, pool_.front ()->size_ - rv);
          pool_.front ()->size_ -= rv;
        }
      else
        {
          pool_.pop_front ();
        }
      return rv;
    }

  streamsize rv = io_->read (data, n); // abuse caller's buffer

  if (traits::is_marker (rv))
    {
      handle_marker (rv);
      return last_marker_;
    }
  else if (0 < rv)
    {
      pool_.push_back (make_shared< bucket > (data, rv));

      if (context::unknown_size != ctx_.width ())
        {
          process_(pool_.back ());
        }
    }

  return 0;
}

streamsize
iimage_skip::marker ()
{
  ifilter::marker ();
  return last_marker_;
}

void
iimage_skip::handle_marker (traits::int_type c)
{
  ctx_ = io_->get_context ();

  /**/ if (traits::bos () == c)
    {
      quantity q = value ((*option_)["blank-threshold"]);
      threshold_ = q.amount< double > ();
    }
  else if (traits::boi () == c)
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
      release_ = false;

      octet data[default_buffer_size];
      while (!release_ && c != traits::eos () && c != traits::eof ())
        c = read (data, default_buffer_size);

      if (release_) c = traits::boi ();
    }
  else if (traits::eoi () == c)
    {
      release_ = !skip_();
      if (!release_) pool_.clear ();
    }
  else if (   traits::eos () == c
           || traits::eof () == c)
    {
      pool_.clear ();
    }
  last_marker_ = c;
}

bool
iimage_skip::skip_()
{
  std::deque< shared_ptr< bucket > >::iterator it;
  for (it = pool_.begin (); pool_.end () != it; ++it)
    {
      if (!(*it)->seen_) process_(*it);
    }

  return 100 * darkness_ <= threshold_ * ctx_.octets_per_image ();
}

void
iimage_skip::process_(shared_ptr< bucket > bp)
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
