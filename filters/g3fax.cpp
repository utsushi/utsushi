//  g3fax.cpp -- convert scanlines to G3 facsimile format
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

#include <cstddef>
#include <string>
#include <vector>

#include <boost/assert.hpp>

#include <utsushi/cstdint.hpp>

#include "g3fax.hpp"

#define WHITE true
#define BLACK false

namespace utsushi {
namespace _flt_ {

using std::string;
using std::vector;

static string transform (const octet *scanline, streamsize n,
                         bool is_light_based);
static string transform (vector<size_t>& runs);

streamsize
g3fax::write (const octet *data, streamsize n)
{
  BOOST_ASSERT ((data && 0 < n) || 0 == n);

  streamsize header_size = 0;
  if (!pbm_header_seen_) header_size = skip_pbm_header_(data, n);

  streamsize octets = std::min (ctx_.octets_per_line () - partial_size_,
                                n - header_size);

  {                             // continue with stashed octets
    traits::copy (partial_line_.get () + partial_size_,
                  data, octets);
    partial_size_ += octets;
    if (partial_size_ == ctx_.octets_per_line ())
      {
        string g3_enc = transform (partial_line_.get (), ctx_.width (),
                                   is_light_based_);
        output_->write (g3_enc.data (), g3_enc.size ());

        ctx_.octets_seen () += ctx_.octets_per_line ();
      }
    else
      {
        return n;
      }
  }

  while (octets + ctx_.octets_per_line () <= n - header_size)
    {
      string g3_enc = transform (data + octets, ctx_.width (),
                                 is_light_based_);
      output_->write (g3_enc.data (), g3_enc.size ());

      octets              += ctx_.octets_per_line ();
      ctx_.octets_seen () += ctx_.octets_per_line ();
    }

  partial_size_ = n - header_size - octets;
  if (0 < partial_size_)        // stash left-over octets for next write
    {
      traits::copy (partial_line_.get (), data + octets,
                    partial_size_);
    }

  return n;
}

void
g3fax::boi (const context& ctx)
{
  BOOST_ASSERT (1 == ctx.depth ());
  BOOST_ASSERT (1 == ctx.comps ());
  BOOST_ASSERT (0 == ctx.padding_octets ());

  BOOST_ASSERT (   "image/g3fax"             == ctx.content_type ()
                || "image/x-portable-bitmap" == ctx.content_type ());

  pbm_header_seen_ = ("image/g3fax" == ctx.content_type ());
  is_light_based_ = ("image/g3fax" == ctx.content_type ());

  ctx_ = ctx;
  ctx_.content_type ("image/g3fax");
  ctx_.octets_seen () = 0;

  partial_line_.reset (new octet[ctx_.octets_per_line ()]);
  partial_size_ = 0;
}

void
g3fax::eoi (const context& ctx)
{
  BOOST_ASSERT (partial_size_ == 0);
  BOOST_ASSERT (ctx_.octets_seen () == ctx.octets_per_image ());

  ctx_ = ctx;
  ctx_.content_type ("image/g3fax");
  ctx_.octets_seen () = ctx.octets_per_image ();
}

static inline
bool
is_white_space (char c)
{
  return (' ' == c || '\t' == c || '\r' == c || '\n' == c);
}

static inline
bool
is_ascii_digit (char c)
{
  return ('0' <= c && c <= '9');
}

streamsize
g3fax::skip_pbm_header_(const octet *& data, streamsize n)
{
  // Assume n is suitably large (> ~20 in the absence of comments)
  // FIXME allow for comments

  const char *head = data;
  const char *tail = head + n;

  BOOST_ASSERT (2 < n);
  BOOST_ASSERT ('P' == head[0]);
  BOOST_ASSERT ('4' == head[1]);

  head += 2;

  while (head != tail && is_white_space (*head))
    {
      ++head;
    }
  BOOST_ASSERT (head != tail && '#' != *head);

  while (head != tail && is_ascii_digit (*head))
    {
      ++head;
    }
  while (head != tail && is_white_space (*head))
    {
      ++head;
    }
  BOOST_ASSERT (head != tail && '#' != *head);

  while (head != tail && is_ascii_digit (*head))
    {
      ++head;
    }
  BOOST_ASSERT (head != tail && is_white_space (*head));
  ++head;
  pbm_header_seen_ = true;

  streamsize rv = head - data;
  data = head;
  return rv;
}

  struct code {
    unsigned int bits;
    unsigned int code;
  };

  static const size_t g3_make_up_min = 64;
  static const size_t g3_make_up_inc = 64;
  static const size_t g3_extra_make_up_min = 1792;
  static const size_t g3_extra_make_up_max = 2560;

  //! Terminating codes for white runs of less than 64 pixels.
  static const struct code g3_white_terminal[] =
    {
      { 8, 0x35 },
      { 6, 0x07 },
      { 4, 0x07 },
      { 4, 0x08 },
      { 4, 0x0b },
      { 4, 0x0c },
      { 4, 0x0e },
      { 4, 0x0f },
      { 5, 0x13 },
      { 5, 0x14 },
      { 5, 0x07 },
      { 5, 0x08 },
      { 6, 0x08 },
      { 6, 0x03 },
      { 6, 0x34 },
      { 6, 0x35 },
      { 6, 0x2a },
      { 6, 0x2b },
      { 7, 0x27 },
      { 7, 0x0c },
      { 7, 0x08 },
      { 7, 0x17 },
      { 7, 0x03 },
      { 7, 0x04 },
      { 7, 0x28 },
      { 7, 0x2b },
      { 7, 0x13 },
      { 7, 0x24 },
      { 7, 0x18 },
      { 8, 0x02 },
      { 8, 0x03 },
      { 8, 0x1a },
      { 8, 0x1b },
      { 8, 0x12 },
      { 8, 0x13 },
      { 8, 0x14 },
      { 8, 0x15 },
      { 8, 0x16 },
      { 8, 0x17 },
      { 8, 0x28 },
      { 8, 0x29 },
      { 8, 0x2a },
      { 8, 0x2b },
      { 8, 0x2c },
      { 8, 0x2d },
      { 8, 0x04 },
      { 8, 0x05 },
      { 8, 0x0a },
      { 8, 0x0b },
      { 8, 0x52 },
      { 8, 0x53 },
      { 8, 0x54 },
      { 8, 0x55 },
      { 8, 0x24 },
      { 8, 0x25 },
      { 8, 0x58 },
      { 8, 0x59 },
      { 8, 0x5a },
      { 8, 0x5b },
      { 8, 0x4a },
      { 8, 0x4b },
      { 8, 0x32 },
      { 8, 0x33 },
      { 8, 0x34 },
    };

  //! Terminating codes for black runs of less than 64 pixels.
  static const struct code g3_black_terminal[] =
    {
      { 10, 0x37 },
      {  3, 0x02 },
      {  2, 0x03 },
      {  2, 0x02 },
      {  3, 0x03 },
      {  4, 0x03 },
      {  4, 0x02 },
      {  5, 0x03 },
      {  6, 0x05 },
      {  6, 0x04 },
      {  7, 0x04 },
      {  7, 0x05 },
      {  7, 0x07 },
      {  8, 0x04 },
      {  8, 0x07 },
      {  9, 0x18 },
      { 10, 0x17 },
      { 10, 0x18 },
      { 10, 0x08 },
      { 11, 0x67 },
      { 11, 0x68 },
      { 11, 0x6c },
      { 11, 0x37 },
      { 11, 0x28 },
      { 11, 0x17 },
      { 11, 0x18 },
      { 12, 0xca },
      { 12, 0xcb },
      { 12, 0xcc },
      { 12, 0xcd },
      { 12, 0x68 },
      { 12, 0x69 },
      { 12, 0x6a },
      { 12, 0x6b },
      { 12, 0xd2 },
      { 12, 0xd3 },
      { 12, 0xd4 },
      { 12, 0xd5 },
      { 12, 0xd6 },
      { 12, 0xd7 },
      { 12, 0x6c },
      { 12, 0x6d },
      { 12, 0xda },
      { 12, 0xdb },
      { 12, 0x54 },
      { 12, 0x55 },
      { 12, 0x56 },
      { 12, 0x57 },
      { 12, 0x64 },
      { 12, 0x65 },
      { 12, 0x52 },
      { 12, 0x53 },
      { 12, 0x24 },
      { 12, 0x37 },
      { 12, 0x38 },
      { 12, 0x27 },
      { 12, 0x28 },
      { 12, 0x58 },
      { 12, 0x59 },
      { 12, 0x2b },
      { 12, 0x2c },
      { 12, 0x5a },
      { 12, 0x66 },
      { 12, 0x67 },
    };

  //! Make up codes for white runs of 64 to 1728 + 63 pixels.
  static const struct code g3_white_make_up[] =
    {                             // runlength 64 + index * 64
      { 5, 0x1b },
      { 5, 0x12 },
      { 6, 0x17 },
      { 7, 0x37 },
      { 8, 0x36 },
      { 8, 0x37 },
      { 8, 0x64 },
      { 8, 0x65 },
      { 8, 0x68 },
      { 8, 0x67 },
      { 9, 0xcc },
      { 9, 0xcd },
      { 9, 0xd2 },
      { 9, 0xd3 },
      { 9, 0xd4 },
      { 9, 0xd5 },
      { 9, 0xd6 },
      { 9, 0xd7 },
      { 9, 0xd8 },
      { 9, 0xd9 },
      { 9, 0xda },
      { 9, 0xdb },
      { 9, 0x98 },
      { 9, 0x99 },
      { 9, 0x9a },
      { 6, 0x18 },
      { 9, 0x9b },
    };

  //! Make up codes for black runs of 64 to 1728 + 63 pixels.
  static const struct code g3_black_make_up[] =
    {                             // runlength 64 + index * 64
      { 10, 0x0f },
      { 12, 0xc8 },
      { 12, 0xc9 },
      { 12, 0x5b },
      { 12, 0x33 },
      { 12, 0x34 },
      { 12, 0x35 },
      { 13, 0x6c },
      { 13, 0x6d },
      { 13, 0x4a },
      { 13, 0x4b },
      { 13, 0x4c },
      { 13, 0x4d },
      { 13, 0x72 },
      { 13, 0x73 },
      { 13, 0x74 },
      { 13, 0x75 },
      { 13, 0x76 },
      { 13, 0x77 },
      { 13, 0x52 },
      { 13, 0x53 },
      { 13, 0x54 },
      { 13, 0x55 },
      { 13, 0x5a },
      { 13, 0x5b },
      { 13, 0x64 },
      { 13, 0x65 },
    };

  //! Additional make up codes for run of more than 1792 pixels.
  static const struct code g3_extra_make_up[] =
    {                             // runlength 1792 + index * 64
      { 11, 0x08 },
      { 11, 0x0c },
      { 11, 0x0d },
      { 12, 0x12 },
      { 12, 0x13 },
      { 12, 0x14 },
      { 12, 0x15 },
      { 12, 0x16 },
      { 12, 0x17 },
      { 12, 0x1c },
      { 12, 0x1d },
      { 12, 0x1e },
      { 12, 0x1f },
    };

  //! Converts a \a scanline into FAX G3 encoded data format
  /*! This functions merely collects the run lengths into a vector and
   *  passes that vector off to a secondary transform(), the result of
   *  which is then returned to the caller.
   */
  static string
  transform (const octet* scanline, streamsize bits, bool is_light_based)
  {
    bool   colour = WHITE;
    size_t length = 0;
    vector< size_t > runs;

    uint8_t bit = 0x80;
    octet oct = (is_light_based ? *scanline : ~*scanline);

    while (0 < bits--)
      {
        if (colour == bool ((oct | ~bit) & bit))
          {
            ++length;
          }
        else
          {
            runs.push_back (length);
            colour = (WHITE == colour ? BLACK : WHITE);
            length = 1;
          }

        bit >>= 1;
        if (0 == bit)
          {
            bit = 0x80;
            ++scanline;
            oct = (is_light_based ? *scanline : ~*scanline);
          }
      }
    runs.push_back (length);

    return transform (runs);
  }

  //! Converts a vector of run lengths into FAX G3 encoded data
  /*! The string always starts with an end-of-line marker and will be
   *  filled if necessary.
   */
  static string
  transform (vector< size_t >& runs)
  {
    vector< size_t >::iterator it = runs.begin ();
    bool colour = WHITE;

    string result;
    unsigned char ch = 0x00;
    size_t i = 0;

    unsigned int mask = 1 << 11;
    unsigned int code = 1;

    while (mask)
      {
        if (code & mask) ch |= (1 << (7 - i % 8));
        ++i;
        mask >>= 1;
        if (0 == i % 8)
          {
            result.push_back (ch);
            ch = 0x00;
          }
      }

    while (runs.end () != it)
      {
        bool terminal = false;
        do
          {
            const struct code *c = NULL;

            if (g3_extra_make_up_max <= *it)
              {
                size_t index = ((g3_extra_make_up_max - g3_extra_make_up_min)
                                / g3_make_up_inc);

                c = g3_extra_make_up + index;
                *it -= g3_extra_make_up_max;
              }
            else if (g3_extra_make_up_min <= *it)
              {
                size_t index = (*it - g3_extra_make_up_min) / g3_make_up_inc;

                c = g3_extra_make_up + index;
                *it -= g3_extra_make_up_min + index * g3_make_up_inc;
              }
            else if (g3_make_up_min <= *it)
              {
                size_t index = (*it - g3_make_up_min) / g3_make_up_inc;

                c = ((WHITE == colour ? g3_white_make_up : g3_black_make_up)
                     + index);
                *it -= g3_make_up_min + index * g3_make_up_inc;
              }
            else
              {
                c = ((WHITE == colour ? g3_white_terminal : g3_black_terminal)
                     + *it);
                *it = 0;
                terminal = true;
              }

            unsigned int mask = 1 << (c->bits - 1);
            unsigned int code = c->code;

            while (mask)
              {
                if (code & mask) ch |= (1 << (7 - i % 8));
                ++i;
                mask >>= 1;
                if (0 == i % 8)
                  {
                    result.push_back (ch);
                    ch = 0x00;
                  }
              }
          }
        while (!terminal);

        ++it;
        colour = (WHITE == colour ? BLACK : WHITE);
      }
    if (0 != i % 8)
      result.push_back (ch);

    return result;
  }

}       // namespace _flt_
}       // namespace utsushi
