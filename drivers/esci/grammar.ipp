//  grammar.ipp -- rule definitions for the ESC/I "compound" protocol
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

#ifndef drivers_esci_grammar_ipp_
#define drivers_esci_grammar_ipp_

/*! \file
 *  This file contains the template \e definitions for the grammar's
 *  rules.  Include this file if you want to use such a parser using
 *  an iterator type \e different from the preferred type.  In case
 *  the preferred iterator type suffices, include the corresponding
 *  \c *.hpp file instead.
 *
 *  \sa decoding::default_iterator_type
 *  \sa encoding::default_iterator_type
 */

//  decoding::basic_grammar<T> implementation requirements
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_attr.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/include/qi_difference.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_omit.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/qi_permutation.hpp>

//  encoding::basic_grammar<T> implementation requirements
#include <boost/spirit/include/karma_binary.hpp>
#include <boost/spirit/include/karma_char_.hpp>
#include <boost/spirit/include/karma_sequence.hpp>

//  *::basic_grammar<T> implementation requirements
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

//  Support for debugging of parser and generator rules
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>

#include "grammar.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace decoding {

template< typename Iterator >
basic_grammar< Iterator >::basic_grammar ()
  : basic_grammar_formats< Iterator > ()
  , basic_grammar_information< Iterator > ()
  , basic_grammar_capabilities< Iterator > ()
  , basic_grammar_parameters< Iterator > ()
  , basic_grammar_status< Iterator > ()
{
  using namespace code_token::reply;
  using code_token::reply::CAN;

  header_rule_ %=
    reply_token_
    > this->hexadecimal_
    ;

  reply_token_ %=
    &(  token_(FIN )
      | token_(CAN )
      | token_(UNKN)
      | token_(INVD)
      | token_(INFO)
      | token_(CAPA)
      | token_(CAPB)
      | token_(PARA)
      | token_(PARB)
      | token_(RESA)
      | token_(RESB)
      | token_(STAT)
      | token_(MECH)
      | token_(TRDT)
      | token_(IMG )
      | token_(EXT0)
      | token_(EXT1)
      | token_(EXT2)
      )
    > token_
    ;

  // The protocol specification is quite clear as to what ordering the
  // various tokens are supposed to arrive in but, alas, firmware does
  // whatever it pleases at times.  Cater to an arbitrary ordering but
  // do insist on unique occurences (except for error codes) and defer
  // priority logic to the compound_base::decode_reply_block_hook_()
  // implementation.

  status_rule_ %=
    skip_rule_
    > -(  *((token_(info::ERR) > err_rule_)      > skip_rule_)
        ^  ((token_(info::NRD) > nrd_token_)     > skip_rule_)
        ^  ((token_(info::PST) > pst_rule_)      > skip_rule_)
        ^  ((token_(info::PEN) > pen_rule_)      > skip_rule_)
        ^  ((token_(info::LFT) > this->decimal_) > skip_rule_)
        ^  ((token_(info::TYP) > typ_token_)     > skip_rule_)
        ^  ((token_(info::ATN) > atn_token_)     > skip_rule_)
        ^  ((token_(info::PAR) > par_token_)     > skip_rule_)
        ^  ((token_(info::DOC) > doc_token_)     > skip_rule_)
        )
    > (token_(info::END) | qi::eoi)
    ;

  skip_rule_ %=
    qi::omit [ *(token_ - info_token_) ]
    ;

  info_token_ %=
    &(  token_(info::ERR)
      | token_(info::NRD)
      | token_(info::PST)
      | token_(info::PEN)
      | token_(info::LFT)
      | token_(info::TYP)
      | token_(info::ATN)
      | token_(info::PAR)
      | token_(info::DOC)
      | token_(info::END)
      )
    > token_
    ;

  using namespace info;         // from here on down

  err_rule_ %=
      err_part_token_
    > err_what_token_
    ;

  err_part_token_ %=
    &(  token_(err::ADF )
      | token_(err::TPU )
      | token_(err::FB  )
      )
    > token_
    ;

  err_what_token_ %=
    &(  token_(err::OPN )
      | token_(err::PJ  )
      | token_(err::PE  )
      | token_(err::ERR )
      | token_(err::LTF )
      | token_(err::LOCK)
      | token_(err::DFED)
      | token_(err::DTCL)
      | token_(err::AUTH)
      | token_(err::PERM)
      | token_(err::BTLO)
      )
    > token_
    ;

  nrd_token_ %=
    &(  token_(nrd::RSVD)
      | token_(nrd::BUSY)
      | token_(nrd::WUP )
      | token_(nrd::NONE)
      )
    > token_
    ;

  pst_rule_ %=
    this->positive_
    > this->positive_
    > this->positive_
    ;

  pen_rule_ %=
    this->positive_
    > qi::attr (esci_non_int)   // synthesize padding attribute
    > this->positive_
    ;

  typ_token_ %=
    &(  token_(typ::IMGA)
      | token_(typ::IMGB)
      )
    > token_
    ;

  atn_token_ %=
    &(  token_(atn::CAN )
      | token_(atn::NONE)
      )
    > token_
    ;

  par_token_ %=
    &(  token_(par::OK  )
      | token_(par::FAIL)
      | token_(par::LOST)
      )
    > token_
    ;

  doc_token_ %=
    &(  token_(doc::CRST)
      )
    > token_
    ;

  ESCI_GRAMMAR_TRACE_NODE (header_rule_);
  ESCI_GRAMMAR_TRACE_NODE (status_rule_);

  ESCI_GRAMMAR_TRACE_NODE (err_rule_);
  ESCI_GRAMMAR_TRACE_NODE (pst_rule_);
  ESCI_GRAMMAR_TRACE_NODE (pen_rule_);

  ESCI_GRAMMAR_TRACE_NODE (skip_rule_);

  ESCI_GRAMMAR_TRACE_NODE (reply_token_);
  ESCI_GRAMMAR_TRACE_NODE (info_token_);
  ESCI_GRAMMAR_TRACE_NODE (err_part_token_);
  ESCI_GRAMMAR_TRACE_NODE (err_what_token_);
  ESCI_GRAMMAR_TRACE_NODE (nrd_token_);
  ESCI_GRAMMAR_TRACE_NODE (typ_token_);
  ESCI_GRAMMAR_TRACE_NODE (atn_token_);
  ESCI_GRAMMAR_TRACE_NODE (par_token_);
  ESCI_GRAMMAR_TRACE_NODE (doc_token_);
}

}       // namespace decoding

namespace encoding {

template< typename Iterator >
basic_grammar< Iterator >::basic_grammar ()
  : basic_grammar_formats< Iterator > ()
  , basic_grammar_parameters< Iterator > ()
  , basic_grammar_mechanics< Iterator > ()
{
  using namespace code_token::request;
  using code_token::request::CAN;

  header_rule_ %=
    request_token_
    << this->hexadecimal_
    ;

#define SYMBOL_ENTRY(token) (token, token_(token))

  request_token_.add
    SYMBOL_ENTRY (FIN )
    SYMBOL_ENTRY (CAN )
    SYMBOL_ENTRY (INFO)
    SYMBOL_ENTRY (CAPA)
    SYMBOL_ENTRY (CAPB)
    SYMBOL_ENTRY (PARA)
    SYMBOL_ENTRY (PARB)
    SYMBOL_ENTRY (RESA)
    SYMBOL_ENTRY (RESB)
    SYMBOL_ENTRY (STAT)
    SYMBOL_ENTRY (MECH)
    SYMBOL_ENTRY (TRDT)
    SYMBOL_ENTRY (IMG )
    SYMBOL_ENTRY (EXT0)
    SYMBOL_ENTRY (EXT1)
    SYMBOL_ENTRY (EXT2)
    ;

#undef SYMBOL_ENTRY

  ESCI_GRAMMAR_TRACE_NODE (header_rule_);
}

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#define ESCI_NS utsushi::_drv_::esci

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::header,
 (ESCI_NS::quad   , code)
 (ESCI_NS::integer, size))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::status::error,
 (ESCI_NS::quad, part)
 (ESCI_NS::quad, what))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::status::image,
 (ESCI_NS::integer, width)
 (ESCI_NS::integer, padding)
 (ESCI_NS::integer, height))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::status,
 (std::vector< ESCI_NS::status::error >, err)
 (boost::optional< ESCI_NS::quad >, nrd)
 (boost::optional< ESCI_NS::status::image >, pst)
 (boost::optional< ESCI_NS::status::image >, pen)
 (boost::optional< ESCI_NS::integer >, lft)
 (boost::optional< ESCI_NS::quad >, typ)
 (boost::optional< ESCI_NS::quad >, atn)
 (boost::optional< ESCI_NS::quad >, par)
 (boost::optional< ESCI_NS::quad >, doc))

#undef ESCI_NS

#endif  /* drivers_esci_grammar_ipp_ */
