//  grammar.cpp -- instantiations for the ESC/I "compound" protocol
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

/*! \file
 *  This file \e instantiates templates using the grammar's preferred
 *  iterator type so they are available throughout the driver library.
 *  This approach prevents individual instantions in the translation
 *  units that refer to grammar components and eases the compiler's
 *  burden.
 */

#include <utsushi/log.hpp>

#include "grammar.ipp"

namespace utsushi {
namespace _drv_ {
namespace esci {

using namespace code_token::reply::info;

bool
header::operator== (const header& rhs) const
{
  return (   code == rhs.code
          && size == rhs.size);
}

bool
status::operator== (const status& rhs) const
{
  return (   err == rhs.err
          && nrd == rhs.nrd
          && pst == rhs.pst
          && pen == rhs.pen
          && lft == rhs.lft
          && typ == rhs.typ
          && atn == rhs.atn
          && par == rhs.par
          && doc == rhs.doc);
}

bool
status::fatal_error () const
{
  if (err.empty ()) return false;

  std::vector< error >::const_iterator it = err.begin ();
  for (; err.end () != it; ++it)
    {
      if (err::PE != it->what) return true;
    }

  return (lft && 0 != *lft);
}

bool
status::is_busy () const
{
  return nrd && nrd::BUSY == *nrd;
}

bool
status::is_cancel_requested () const
{
  return atn && atn::CAN == *atn;
}

bool
status::is_flip_side () const
{
  return typ && typ::IMGB == *typ;
}

bool
status::is_in_use () const
{
  return nrd && nrd::RSVD == *nrd;
}

bool
status::is_parameter_set_okay () const
{
  return !par || par::OK == *par;
}

bool
status::is_ready () const
{
  return !nrd || nrd::NONE == *nrd;
}

bool
status::is_warming_up () const
{
  return nrd && nrd::WUP == *nrd;
}

bool
status::is_normal_sheet () const
{
  return !doc;
}

bool
status::media_out () const
{
  std::vector< error >::const_iterator it = err.begin ();
  for (; err.end () != it; ++it)
    {
      if (err::PE == it->what) return true;
    }
  return (lft && 0 == *lft);
}

bool
status::media_out (const quad& where) const
{
  std::vector< error >::const_iterator it = err.begin ();
  for (; err.end () != it; ++it)
    {
      if (   where   == it->part
          && err::PE == it->what) return true;
    }
  return false;
}

void
status::clear ()
{
  *this = status ();
}

//! \todo Check for which values of reply.code doc is inappropriate
void
status::check (const header& reply) const
{
  using namespace code_token::reply;

  if (!err.empty ()
      && !(IMG == reply.code || TRDT == reply.code || MECH == reply.code))
    log::brief ("unexpected error detected (%1%)") % str (reply.code);

  if (pen && pst)
    {
      log::brief ("simultaneous %1% and %2% not allowed")
        % str (PST)
        % str (PEN)
        ;
    }

  if (lft && !pen)
    log::brief ("orphaned images-left-to-scan info (%1% more)") % *lft;

  if (par)
    {
      format fmt ("unexpected feedback (%1%: %2% = %3%)");

      if (!is_parameter_code (reply.code))
        {
          log::brief (fmt) % str (reply.code) % str (PAR) % str (*par);
        }
      else if (par::OK != *par)
        {
          if (is_get_parameter_code (reply.code) && par::LOST != *par)
            log::brief (fmt) % str (reply.code) % str (PAR) % str (*par);
          if (is_set_parameter_code (reply.code) && par::FAIL != *par)
            log::brief (fmt) % str (reply.code) % str (PAR) % str (*par);
        };
    }

  // The following checks are based on common sense.  They are not
  // documented as hard requirements in the command specification,
  // but they really should.

  if (IMG != reply.code)
    {
      format fmt ("unexpected feedback (%1%: %2%)");

      if (pst) log::brief (fmt) % str (reply.code) % str (PST);
      if (pen) log::brief (fmt) % str (reply.code) % str (PEN);
      if (lft) log::brief (fmt) % str (reply.code) % str (LFT);
      if (typ) log::brief (fmt) % str (reply.code) % str (TYP);

      if (atn && atn::CAN == *atn)
        log::brief ("unexpected cancel request (%1%)") % str (reply.code);
    }
}

bool
status::is_get_parameter_code (const quad& code) const
{
  using namespace code_token::reply;

  return (   RESA == code
          || RESB == code);
}

bool
status::is_set_parameter_code (const quad& code) const
{
  using namespace code_token::reply;

  return (   PARA == code
          || PARB == code);
}

bool
status::is_parameter_code (const quad& code) const
{
  return (   is_get_parameter_code (code)
          || is_set_parameter_code (code));
}

bool
status::error::operator== (const status::error& rhs) const
{
  return (   part == rhs.part
          && what == rhs.what);
}

bool
status::image::operator== (const status::image& rhs) const
{
  return (   width   == rhs.width
          && height  == rhs.height
          && padding == rhs.padding);
}

template class
decoding::basic_grammar< decoding::default_iterator_type >;

template class
encoding::basic_grammar< encoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#ifdef USE_SINGLE_FILE_GRAMMAR_INSTANTIATION
#undef USE_SINGLE_FILE_GRAMMAR_INSTANTIATION

#include "grammar-capabilities.cpp"
#include "grammar-information.cpp"
#include "grammar-mechanics.cpp"
#include "grammar-formats.cpp"
#include "grammar-parameters.cpp"
#include "grammar-status.cpp"
#include "grammar-support.cpp"

#endif /* defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
