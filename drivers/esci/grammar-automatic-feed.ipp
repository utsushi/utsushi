//  grammar-automatic-feed.ipp -- component rule definitions
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_grammar_automatic_feed_ipp_
#define drivers_esci_grammar_automatic_feed_ipp_

//! \copydoc grammar.ipp

//  encoding::basic_grammar_automatic_feed<T> implementation requirements
#include <boost/spirit/include/karma_binary.hpp>
#include <boost/spirit/include/karma_char_.hpp>

//  Support for debugging of parser and generator rules
#include <boost/spirit/include/karma_nonterminal.hpp>

#include "code-point.hpp"
#include "grammar-automatic-feed.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace encoding {

template< typename Iterator >
basic_grammar_automatic_feed< Iterator >::basic_grammar_automatic_feed ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::automatic_feed;

  automatic_feed_rule_ %=
    mode_token_
    ;

#define SYMBOL_ENTRY(token) (token, token_(token))

  mode_token_.add
    SYMBOL_ENTRY (ON)
    SYMBOL_ENTRY (OFF)
    ;

#undef SYMBOL_ENTRY

  ESCI_GRAMMAR_TRACE_NODE (automatic_feed_rule_);
}

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_automatic_feed_ipp_ */
