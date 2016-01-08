//  grammar-mechanics.ipp -- component rule definitions
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

#ifndef drivers_esci_grammar_mechanics_ipp_
#define drivers_esci_grammar_mechanics_ipp_

//! \copydoc grammar.ipp

//  encoding::basic_grammar_mechanics<T> implementation requirements
#include <boost/spirit/include/karma_alternative.hpp>
#include <boost/spirit/include/karma_and_predicate.hpp>
#include <boost/spirit/include/karma_binary.hpp>
#include <boost/spirit/include/karma_bool.hpp>
#include <boost/spirit/include/karma_buffer.hpp>
#include <boost/spirit/include/karma_char_.hpp>
#include <boost/spirit/include/karma_optional.hpp>
#include <boost/spirit/include/karma_sequence.hpp>

//  *::basic_grammar_mechanics<T> implementation requirements
#include <boost/fusion/include/adapt_struct.hpp>

//  Support for debugging of parser and generator rules
#include <boost/spirit/include/karma_nonterminal.hpp>

#include "code-point.hpp"
#include "grammar-mechanics.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace encoding {

template< typename Iterator >
basic_grammar_mechanics< Iterator >::basic_grammar_mechanics ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::mechanic;

  hardware_control_rule_ %=
       - karma::buffer [ token_(ADF) << mech_adf_token_ ]
    << - karma::buffer [ token_(FCS) <<
                         ( (mech_fcs_token_ << this->numeric_)
                          | token_(fcs::AUTO)) ]
    << - karma::buffer [ &karma::true_ << token_(INI) ]
    ;

#define SYMBOL_ENTRY(token) (token, token_(token))

  mech_adf_token_.add
    SYMBOL_ENTRY (adf::LOAD)
    SYMBOL_ENTRY (adf::EJCT)
    SYMBOL_ENTRY (adf::CLEN)
    SYMBOL_ENTRY (adf::CALB)
    ;

  mech_fcs_token_.add
  //SYMBOL_ENTRY (fcs::AUTO)
    SYMBOL_ENTRY (fcs::MANU)
    ;
#undef SYMBOL_ENTRY

  ESCI_GRAMMAR_TRACE_NODE (hardware_control_rule_);
}

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#define ESCI_NS utsushi::_drv_::esci

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::hardware_request::focus,
 (ESCI_NS::quad, type)
 (boost::optional< ESCI_NS::integer >, position))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::hardware_request,
 (boost::optional< ESCI_NS::quad >, adf)
 (boost::optional< ESCI_NS::hardware_request::focus >, fcs)
 (bool, ini))

#undef ESCI_NS

#endif  /* drivers_esci_grammar_mechanics_ipp_ */
