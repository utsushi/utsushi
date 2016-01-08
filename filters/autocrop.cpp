//  autocrop.cpp -- leaving only the (reoriented) scanned documents
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

#include "autocrop.hpp"

#include <utsushi/range.hpp>
#include <utsushi/run-time.hpp>

#include <boost/lexical_cast.hpp>

namespace utsushi {
namespace _flt_ {

using boost::lexical_cast;

const streamsize PNM_HEADER_SIZE = 50;

autocrop::autocrop ()
  : shell_pipe (run_time ().exec_file (run_time::pkg, "doc-locate"))
{
  option_->add_options ()
    ("lo-threshold", (from< range > ()  // percentage
                      -> lower (  0.0)
                      -> upper (100.0)
                      -> default_value (45.0)))
    ("hi-threshold", (from< range > ()  // percentage
                      -> lower (  0.0)
                      -> upper (100.0)
                      -> default_value (55.0)))
    ("trim", toggle (false))
    ;
  freeze_options ();   // initializes option tracking member variables
}

void
autocrop::mark (traits::int_type c, const context& ctx)
{
  if (traits::boi () == c)
    {
      traits::assign (header_buf_, header_buf_size_, 0x00);
      header_buf_used_ = 0;
      header_seen_ = false;

      output::mark (c, ctx);
    }
  else
    {
      shell_pipe::mark (c, ctx);
    }
}

void
autocrop::freeze_options ()
{
  quantity threshold;

  threshold = value ((*option_)["lo-threshold"]);
  lo_threshold_ = threshold.amount< double > ();
  threshold = value ((*option_)["hi-threshold"]);
  hi_threshold_ = threshold.amount< double > ();

  toggle t = value ((*option_)["trim"]);
  trim_ = t;
}

context
autocrop::estimate (const context& ctx)
{
  width_  = context::unknown_size;
  height_ = context::unknown_size;

  context rv (ctx);

  rv.width  (width_);
  rv.height (height_);
  rv.content_type ("image/x-portable-anymap");

  return rv;
}

context
autocrop::finalize (const context& ctx)
{
  context rv (ctx);

  rv.width  (width_);
  rv.height (height_);
  rv.content_type ("image/x-portable-anymap");

  return rv;
}

std::string
autocrop::arguments (const context& ctx)
{
  using std::string;

  string argv;

  // Set up input data characteristics

  argv += " " + lexical_cast< string > (lo_threshold_ / 100);
  argv += " " + lexical_cast< string > (hi_threshold_ / 100);
  argv += (trim_ ? " trim" : " crop");
  argv += " " + lexical_cast< string > (ctx.octets_per_image ()
                                        + PNM_HEADER_SIZE);
  argv += " -";
  argv += " pnm:-";

  return argv;
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

void
autocrop::checked_write (octet *data, streamsize n)
{
  if (!header_seen_)            // parse PNM header
    {
      // FIXME allow for comments
      size_t m = std::min (n, header_buf_size_ - header_buf_used_);

      traits::copy (header_buf_ + header_buf_used_, data, m);
      header_buf_used_ += m;

      if (header_buf_used_ < header_buf_size_) return;

      char *head = header_buf_;
      char *tail = head + header_buf_used_;

      BOOST_ASSERT (2 < n);
      BOOST_ASSERT ('P' == head[0]);
      BOOST_ASSERT ('4' == head[1] || '5' == head[1] || '6' == head[1]);

      bool has_max_value = ('4' != head[1]);

      head += 2;

      while (head != tail && is_white_space (*head))
        {
          ++head;
        }
      BOOST_ASSERT (head != tail && '#' != *head);

      width_ = 0;
      while (head != tail && is_ascii_digit (*head))
        {
          width_ *= 10;
          width_ += *head - '0';
          ++head;
        }
      while (head != tail && is_white_space (*head))
        {
          ++head;
        }
      BOOST_ASSERT (head != tail && '#' != *head);

      height_ = 0;
      while (head != tail && is_ascii_digit (*head))
        {
          height_ *= 10;
          height_ += *head - '0';
          ++head;
        }
      if (has_max_value)
        {
          while (head != tail && is_white_space (*head))
            {
              ++head;
            }
          BOOST_ASSERT (head != tail && '#' != *head);
          while (head != tail && is_ascii_digit (*head))
            {
              ++head;
            }
        }
      BOOST_ASSERT (head != tail && is_white_space (*head));
      ++head;
      header_seen_ = true;

      ctx_ = finalize (ctx_);
      output_->mark (last_marker_, ctx_);
      signal_marker_(last_marker_);

      output_->write (header_buf_, header_buf_size_);

      n    -= m;       // don't duplicate octets from header_buf_
      data += m;
    }
  if (header_seen_) output_->write (data, n);
}

}       // namespace _flt_
}       // namespace utsushi
