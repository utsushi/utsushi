//  grammar-parameters.cpp -- component instantiations
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

#include <stdexcept>

#include "grammar-parameters.ipp"

namespace utsushi {
namespace _drv_ {
namespace esci {

bool
parameters::operator== (const parameters& rhs) const
{
  return (   adf == rhs.adf
          && tpu == rhs.tpu
          && fb  == rhs.fb
          && col == rhs.col
          && fmt == rhs.fmt
          && jpg == rhs.jpg
          && thr == rhs.thr
          && dth == rhs.dth
          && gmm == rhs.gmm
          && gmt == rhs.gmt
          && cmx == rhs.cmx
          && sfl == rhs.sfl
          && mrr == rhs.mrr
          && bsz == rhs.bsz
          && pag == rhs.pag
          && rsm == rhs.rsm
          && rss == rhs.rss
          && crp == rhs.crp
          && acq == rhs.acq
          && flc == rhs.flc
          && fla == rhs.fla
          && qit == rhs.qit
          && ldf == rhs.ldf
          && dfa == rhs.dfa
          && lam == rhs.lam);
}

void
parameters::clear ()
{
  *this = parameters ();
}

bool
parameters::is_bilevel () const
{
  using namespace code_token::parameter::col;

  if (!col) return false;

  return (   C003 == *col
          || M001 == *col
          || R001 == *col
          || G001 == *col
          || B001 == *col);
}

bool
parameters::is_color () const
{
  using namespace code_token::parameter::col;

  if (!col) return false;

  return (   C003 == *col
          || C024 == *col
          || C048 == *col);
}

/*! \note When only a subset of all parameters has been requested,
 *        this function may return the default token.
 *  \bug The source should be kept in a variant as only one can ever
 *       be set at any given time.
 */
quad
parameters::source () const
{
  using namespace code_token::parameter;

  if (adf) return ADF;
  if (tpu) return TPU;
  if (fb ) return FB;

  return quad ();
}

quantity
parameters::border_left (const quantity& default_value) const
{
  return (fla ? double ((*fla)[0]) / 100 : default_value);
}

quantity
parameters::border_right (const quantity& default_value) const
{
  return (fla ? double ((*fla)[1]) / 100 : default_value);
}

quantity
parameters::border_top (const quantity& default_value) const
{
  return (fla ? double ((*fla)[2]) / 100 : default_value);
}

quantity
parameters::border_bottom (const quantity& default_value) const
{
  return (fla ? double ((*fla)[3]) / 100 : default_value);
}

parameters::gamma_table::gamma_table (const quad& q)
  : component (q)
{}

bool
parameters::gamma_table::operator== (const parameters::gamma_table& rhs) const
{
  return (   component == rhs.component
          && table     == rhs.table);
}

parameters::color_matrix::color_matrix (const quad& q)
  : type (q)
{}

bool
parameters::color_matrix::operator== (const parameters::color_matrix& rhs) const
{
  return (   type   == rhs.type
          && matrix == rhs.matrix);
}

template class
decoding::basic_grammar_parameters< decoding::default_iterator_type >;

template class
encoding::basic_grammar_parameters< encoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif /* !defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
