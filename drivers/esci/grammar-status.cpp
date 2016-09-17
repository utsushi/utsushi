//  grammar-status.cpp -- component instantiations
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

#include <map>
#include <stdexcept>

#include <boost/assign/list_inserter.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/log.hpp>
#include <utsushi/quantity.hpp>

#include "grammar-status.ipp"

namespace utsushi {
namespace _drv_ {
namespace esci {

// Provide a mapping of protocol tokens to utsushi::media instances.
// We use a custom dictionary for two reasons, 1) we cannot rely on
// all required media sizes to be predefined by the core library, and
// 2) the protocol specification may have its own idea about what the
// media dimensions should be for certain media types independent of
// whatever the various standards dictate.

//! \todo Extract and merge with get-scanner-status.ccp code
namespace {

  using utsushi::length;

  const length inches = 1.0;
  const length mm     = inches / 25.4;

  typedef std::map< quad, media > dictionary;
  dictionary *dict = nullptr;

  void initialize_dictionary ()
  {
    using namespace code_token::status::psz;

    if (dict) return;

    dict = new dictionary;
    boost::assign::insert (*dict)
      (A3V , media (297 * mm, 420 * mm))
      (WLT , media (11.00 * inches, 17.00 * inches))
      (B4V , media (257 * mm, 364 * mm))
      (LGV , media ( 8.50 * inches, 14.00 * inches))
      (A4V , media (210 * mm, 297 * mm))
      (A4H , media (297 * mm, 210 * mm))
      (LTV , media ( 8.50 * inches, 11.00 * inches))
      (LTH , media (11.00 * inches,  8.50 * inches))
      (B5V , media (182 * mm, 257 * mm))
      (B5H , media (257 * mm, 182 * mm))
      (A5V , media (148 * mm, 210 * mm))
      (A5H , media (210 * mm, 148 * mm))
      (B6V , media (128 * mm, 182 * mm))
      (B6H , media (182 * mm, 128 * mm))
      (A6V , media (105 * mm, 148 * mm))
      (A6H , media (148 * mm, 105 * mm))
      (EXV , media ( 7.25 * inches, 10.50 * inches))
      (EXH , media (10.50 * inches,  7.25 * inches))
      (HLTV, media ( 5.50 * inches,  8.50 * inches))
      (HLTH, media ( 8.50 * inches,  5.50 * inches))
      (PCV , media (100 * mm, 148 * mm))
      (PCH , media (148 * mm, 100 * mm))
      (KGV , media (4.00 * inches, 6.00 * inches))
      (KGH , media (6.00 * inches, 4.00 * inches))
      (CKV , media ( 90 * mm, 225 * mm))
      (CKH , media (225 * mm,  90 * mm))
      (OTHR, media (length (), length ()))
      // should INVD throw or return a sentinel value like OTHR?
      ;
  }
}       // namespace

static
void check_bits (const integer& push_button)
{
  if (push_button & ~hardware_status::push_button_mask)
    log::brief ("undefined push-button bits detected (%1%)")
      % (push_button & ~hardware_status::push_button_mask);
}

bool
hardware_status::operator== (const hardware_status& rhs) const
{
  return (   medium                 == rhs.medium
          && error_                 == rhs.error_
          && focus                  == rhs.focus
          && push_button            == rhs.push_button
          && separation_mode        == rhs.separation_mode
          && card_slot_lever_status == rhs.card_slot_lever_status);
}

void
hardware_status::clear ()
{
  *this = hardware_status ();
}

bool
hardware_status::size_detected (const quad& part) const
{
  using namespace code_token::status::psz;

  std::vector< result >::const_iterator it = medium.begin ();

  for (; medium.end () != it && part != it->part_; ++it);
  return medium.end () != it && INVD != it->what_;
}

media
hardware_status::size (const quad& part) const
{
  std::vector< result >::const_iterator it = medium.begin ();

  for (; medium.end () != it && part != it->part_; ++it);
  if (medium.end () == it)
    return utsushi::media (length (), length ());

  if (!dict) initialize_dictionary ();

  return dict->at (it->what_);
}

integer
hardware_status::event () const
{
  if (!push_button) return 0;

  check_bits (*push_button);

  return (0x03 & *push_button);
}

bool
hardware_status::is_duplex () const
{
  if (!push_button) return false;

  check_bits (*push_button);

  return (0x10 & *push_button);
}

quad
hardware_status::media_size () const
{
  if (!push_button) return quad ();

  check_bits (*push_button);

  using namespace code_token::status;

  static const quad size[] =
    {
      psz::OTHR,                // use software side setting
      psz::A4V,
      psz::LTV,
      psz::LGV,
      psz::B4V,
      psz::A3V,
      psz::WLT                  // tabloid
    };

  integer idx = (0xe0 & *push_button);
  idx /= (1 << 5);              // right-shifting is not portable

  if (0 <= idx && idx < integer (sizeof (size) / sizeof (*size)))
    return size[idx];

  BOOST_THROW_EXCEPTION
    (std::out_of_range ("push-button media size"));
}

hardware_status::result::result (const quad& part, const quad& what)
  : part_(part)
  , what_(what)
{}

bool
hardware_status::result::operator== (const hardware_status::result& rhs) const
{
  return (   part_ == rhs.part_
          && what_ == rhs.what_);
}

quad
hardware_status::error (const quad& part) const
{
  std::vector< result >::const_iterator it = error_.begin ();

  for (; error_.end () != it && part != it->part_; ++it);
  return (error_.end () != it ? it->what_ : quad ());
}

bool
hardware_status::is_battery_low (const quad& part) const
{
  using namespace code_token::status;

  bool rv (battery_status && bat::LOW == *battery_status);

  if (part)
    {
      rv |= (err::BTLO == error (part));
    }
  else
    {
      std::vector< result >::const_iterator it = error_.begin ();
      for (; error_.end () !=it; ++it)
        {
          rv |= (err::BTLO == it->what_);
        }
    }
  return rv;
}

const integer hardware_status::push_button_mask = 0xf3;

template class
decoding::basic_grammar_status< decoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif /* !defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
