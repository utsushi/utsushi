//  grammar-information.cpp -- component instantiations
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

#ifndef USE_SINGLE_FILE_GRAMMAR_INSTANTIATION

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//! \copydoc grammar.cpp

#include "grammar-information.ipp"

namespace utsushi {
namespace _drv_ {
namespace esci {

information::information ()
  : has_push_button (false)
  , device_buffer_size ()
  , truncates_at_media_end (false)
  , supports_authentication (false)
  , supports_reinitialization (false)
  , supports_automatic_feed (false)
{}

bool
information::operator== (const information& rhs) const
{
  return (   adf                             == rhs.adf
          && tpu                             == rhs.tpu
          && flatbed                         == rhs.flatbed
          && max_image                       == rhs.max_image
          && has_push_button                 == rhs.has_push_button
          && product                         == rhs.product
          && version                         == rhs.version
          && product_version                 == rhs.product_version
          && device_buffer_size              == rhs.device_buffer_size
          && extension                       == rhs.extension
          && truncates_at_media_end          == rhs.truncates_at_media_end
          && serial_number                   == rhs.serial_number
          && supports_authentication         == rhs.supports_authentication
          && supports_reinitialization       == rhs.supports_reinitialization
          && supports_automatic_feed         == rhs.supports_automatic_feed
          && double_feed_detection_threshold == rhs.double_feed_detection_threshold
          && crop_resolution_constraint      == rhs.crop_resolution_constraint);
}

void
information::clear ()
{
  *this = information ();
}

std::string
information::product_name () const
{
  const char *whitespace = " \t";

  using std::string;

  string name (product.begin (), product.end ());

  string::size_type leading (name.find_first_not_of (whitespace));
  string::size_type trailing (name.find_last_not_of (whitespace));

  if (string::npos == leading)
    return string ();
  if (string::npos == trailing)
    return string (name.begin () + leading, name.end ());

  trailing += 1;                // after last non-whitespace
  return string (name.begin () + leading, name.begin () + trailing);
}

bool
information::is_double_pass_duplexer () const
{
  using namespace code_token::information;

  return (    adf
          &&  adf->duplex_passes
          && *adf->duplex_passes == adf::SCN2);
}

bool
information::supports_size_detection (const quad& src) const
{
    using namespace code_token::information;

    if (src == FB ) return (flatbed && flatbed->supports_size_detection ());
    if (src == ADF) return (adf     && adf->supports_size_detection ());
    if (src == TPU) return (tpu     && tpu->supports_size_detection ());

    return false;
}

information::range::range (const integer& lower, const integer& upper)
  : lower_(lower)
  , upper_(upper)
{}

bool
information::range::operator== (const information::range& rhs) const
{
  return (   lower_ == rhs.lower_
          && upper_ == rhs.upper_);
}

information::source::source ()
  : resolution ()
{}

bool
information::source::operator== (const information::source& rhs) const
{
  return (   resolution == rhs.resolution
          && area       == rhs.area
          && overscan   == rhs.overscan);
}

bool
information::source::supports_size_detection () const
{
  return false;
}

bool
information::tpu_source::operator== (const information::tpu_source& rhs) const
{
  return (source::operator== (rhs)
          && alternative_area == rhs.alternative_area);
}

information::fb_source::fb_source ()
  : detects_width (false)
  , detects_height (false)
  , alignment ()
{}

bool
information::fb_source::operator== (const information::fb_source& rhs) const
{
  return (source::operator== (rhs)
          && detects_width  == rhs.detects_width
          && detects_height == rhs.detects_height
          && alignment      == rhs.alignment);
}

bool
information::fb_source::supports_size_detection () const
{
  return detects_width && detects_height;
}

information::adf_source::adf_source ()
  : type ()
  , doc_order ()
  , prefeeds (false)
  , auto_scans (false)
  , auto_recovers (false)
  , detects_carrier_sheet (false)
{}

bool
information::adf_source::operator== (const information::adf_source& rhs) const
{
  return (fb_source::operator== (rhs)
          && type                  == rhs.type
          && duplex_passes         == rhs.duplex_passes
          && doc_order             == rhs.doc_order
          && prefeeds              == rhs.prefeeds
          && auto_scans            == rhs.auto_scans
          && min_doc               == rhs.min_doc
          && max_doc               == rhs.max_doc
          && auto_recovers         == rhs.auto_recovers
          && detects_carrier_sheet == rhs.detects_carrier_sheet);
}

bool
information::adf_source::supports_long_paper_mode () const
{
  return (    0 != area.size ()
          &&  0 != max_doc.size ()
          && (area[1] < max_doc[1]));
}

template class
decoding::basic_grammar_information< decoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif /* !defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
