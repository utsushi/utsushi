//  grammar-status.ipp -- component rule definitions
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

#ifndef drivers_esci_grammar_status_ipp_
#define drivers_esci_grammar_status_ipp_

//! \copydoc grammar.ipp

//  decoding::basic_grammar_status<T> implementation requirements
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_attr.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_permutation.hpp>
#include <boost/spirit/include/qi_skip.hpp>

//  *::basic_grammar_status<T> implementation requirements
#include <boost/fusion/include/adapt_struct.hpp>

//  Support for debugging of parser rules
#include <boost/spirit/include/qi_nonterminal.hpp>

#include "grammar-status.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
basic_grammar_status< Iterator >::basic_grammar_status ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::status;

  hardware_status_rule_ %=
    (  *(token_(PSZ) > stat_psz_rule_)
     ^ *(token_(ERR) > stat_err_rule_)
     ^  (token_(FCS) > stat_fcs_rule_)
     ^  (token_(PB ) > this->decimal_)
     ^  (token_(SEP) > stat_sep_rule_)
     ^  (token_(BAT) > stat_bat_rule_)
     ^  (token_(CSL) > stat_csl_rule_)
     )
    > qi::eoi
    ;

  stat_psz_rule_ %=
      stat_psz_part_token_
    > stat_psz_size_token_
    ;

  stat_err_rule_ %=
      stat_err_part_token_
    > stat_err_what_token_
    ;

  stat_fcs_rule_ %=
      (token_(fcs::INVD) > qi::attr (esci_non_int))
    | (token_(fcs::VALD) > this->decimal_)
    ;

  stat_sep_rule_ %=
    &(  token_(sep::ON)
      | token_(sep::OFF)
      )
    > token_
    ;

  stat_bat_rule_ %=
    &( token_(bat::LOW)
      )
    > token_
    ;

  stat_csl_rule_ %=
    &(  token_(csl::ON)
      | token_(csl::OFF)
      )
    > token_
    ;

  stat_psz_part_token_ %=
    &(  token_(psz::ADF)
      | token_(psz::FB)
      )
    > token_
    ;

  stat_psz_size_token_ %=
    &(  token_(psz::A3V)
      | token_(psz::WLT)
      | token_(psz::B4V)
      | token_(psz::LGV)
      | token_(psz::A4V)
      | token_(psz::A4H)
      | token_(psz::LTV)
      | token_(psz::LTH)
      | token_(psz::B5V)
      | token_(psz::B5H)
      | token_(psz::A5V)
      | token_(psz::A5H)
      | token_(psz::B6V)
      | token_(psz::B6H)
      | token_(psz::A6V)
      | token_(psz::A6H)
      | token_(psz::EXV)
      | token_(psz::EXH)
      | token_(psz::HLTV)
      | token_(psz::HLTH)
      | token_(psz::PCV)
      | token_(psz::PCH)
      | token_(psz::KGV)
      | token_(psz::KGH)
      | token_(psz::CKV)
      | token_(psz::CKH)
      | token_(psz::OTHR)
      | token_(psz::INVD)
      )
    > token_
    ;

  stat_err_part_token_ %=
    &(  token_(err::ADF)
      | token_(err::TPU)
      | token_(err::FB)
      )
    > token_
    ;

  stat_err_what_token_ %=
    &(  token_(err::OPN)
      | token_(err::PJ)
      | token_(err::PE)
      | token_(err::ERR)
      | token_(err::LTF)
      | token_(err::LOCK)
      | token_(err::DFED)
      | token_(err::DTCL)
      | token_(err::BTLO)
      )
    > token_
    ;

  ESCI_GRAMMAR_TRACE_NODE (hardware_status_rule_);

  ESCI_GRAMMAR_TRACE_NODE (stat_psz_rule_);
  ESCI_GRAMMAR_TRACE_NODE (stat_err_rule_);
  ESCI_GRAMMAR_TRACE_NODE (stat_fcs_rule_);
  ESCI_GRAMMAR_TRACE_NODE (stat_sep_rule_);

  ESCI_GRAMMAR_TRACE_NODE (stat_psz_part_token_);
  ESCI_GRAMMAR_TRACE_NODE (stat_psz_size_token_);
  ESCI_GRAMMAR_TRACE_NODE (stat_err_part_token_);
  ESCI_GRAMMAR_TRACE_NODE (stat_err_what_token_);
}

}       // namespace decoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#define ESCI_NS utsushi::_drv_::esci

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::hardware_status::result,
 (ESCI_NS::quad, part_)
 (ESCI_NS::quad, what_))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::hardware_status,
 (std::vector< ESCI_NS::hardware_status::result >, medium)
 (std::vector< ESCI_NS::hardware_status::result >, error_)
 (boost::optional< ESCI_NS::integer >, focus)
 (boost::optional< ESCI_NS::integer >, push_button)
 (boost::optional< ESCI_NS::quad >, separation_mode)
 (boost::optional< ESCI_NS::quad >, battery_status)
 (boost::optional< ESCI_NS::quad >, card_slot_lever_status))

#undef ESCI_NS

#endif  /* drivers_esci_grammar_status_ipp_ */
