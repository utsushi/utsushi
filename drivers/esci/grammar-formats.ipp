//  grammar-formats.ipp -- component rule definitions
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_grammar_formats_ipp_
#define drivers_esci_grammar_formats_ipp_

//! \copydoc grammar.ipp

//  decoding::basic_grammar_formats<T> implementation requirements
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_uint.hpp>

//  encoding::basic_grammar_formats<T> implementation requirements
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_alternative.hpp>
#include <boost/spirit/include/karma_binary.hpp>
#include <boost/spirit/include/karma_char_.hpp>
#include <boost/spirit/include/karma_eps.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_repeat.hpp>
#include <boost/spirit/include/karma_right_alignment.hpp>
#include <boost/spirit/include/karma_sequence.hpp>
#include <boost/spirit/include/karma_upper_lower_case.hpp>
#include "upstream/include/no_attribute_directive.hpp"

//  *::basic_grammar_formats<T> implementation requirements
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/support_ascii.hpp>

//  Support for debugging of parser and generator rules
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>

#include "code-point.hpp"
#include "grammar-formats.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace decoding {

template< typename Iterator >
basic_grammar_formats< Iterator >::basic_grammar_formats ()
{
  namespace ascii = boost::spirit::ascii;

  using boost::phoenix::ref;
  using qi::_val;
  using qi::_1;

  decimal_ %=
    qi::byte_(LOWER_D)
    >> qi::uint_parser< integer, 10, 3, 3 > ()
    ;

  integer_ %=
    qi::byte_(LOWER_I)
    >> (positive_number_ | negative_number_)
    ;

  hexadecimal_ %=
    qi::byte_(LOWER_X)
    >> &(qi::repeat (7) [ ascii::digit | ascii::upper ])
    >> qi::uint_parser< integer, 16, 7, 7 > ()
    ;

  numeric_ %=
    decimal_
    | integer_
    | hexadecimal_
    ;

  positive_ %=
    decimal_
    | positive_integer_
    | hexadecimal_
    ;

  negative_ %=
    negative_integer_
    ;

  positive_integer_ %=
    qi::byte_(LOWER_I)
    >> positive_number_
    ;

  negative_integer_ %=
    qi::byte_(LOWER_I)
    >> negative_number_
    ;

  positive_number_ %=
    qi::uint_parser< integer, 10, 7, 7 > ()
    ;

  negative_number_ %=
    qi::byte_(MINUS)
    >> qi::uint_parser< integer, 10, 6, 6 > () [ _val = -_1 ]
    ;

  bin_hex_data_ %=
    bin_hex_size_
    >> bin_hex_payload_
    >> bin_hex_padding_
    ;

  bin_hex_size_ %=
    qi::byte_(LOWER_H)
    >> qi::uint_parser< integer, 16, 3, 3 > () [ ref(bin_hex_sz_) = _1 ]
    ;

  bin_hex_padding_ %=
    qi::repeat (3 - (ref (bin_hex_sz_) + 3) % 4) [ qi::byte_ ]
    ;

  bin_hex_payload_ %=
    qi::repeat (ref (bin_hex_sz_)) [ qi::byte_ ]
    ;

  ESCI_GRAMMAR_TRACE_NODE (decimal_);
  ESCI_GRAMMAR_TRACE_NODE (integer_);
  ESCI_GRAMMAR_TRACE_NODE (hexadecimal_);
  ESCI_GRAMMAR_TRACE_NODE (numeric_);
  ESCI_GRAMMAR_TRACE_NODE (positive_);
  ESCI_GRAMMAR_TRACE_NODE (negative_);
  ESCI_GRAMMAR_TRACE_NODE (positive_integer_);
  ESCI_GRAMMAR_TRACE_NODE (negative_integer_);

  ESCI_GRAMMAR_TRACE_NODE (positive_number_);
  ESCI_GRAMMAR_TRACE_NODE (negative_number_);

  ESCI_GRAMMAR_TRACE_NODE (bin_hex_data_);

  ESCI_GRAMMAR_TRACE_NODE (bin_hex_size_);
  ESCI_GRAMMAR_TRACE_NODE (bin_hex_padding_);
  ESCI_GRAMMAR_TRACE_NODE (bin_hex_payload_);
}

}       // namespace decoding

namespace encoding {

/*! The implementation relies on karma::int_generator because our \c
 *  integer attribute is signed.  The karma::uint_generator can only
 *  be instantiated with \e unsigned integral types.
 *
 *  For some reason, the karma::char_ generator is used somewhere in
 *  the instantiation of our rules (probably by \c ascii::upper) but
 *  the corresponding header file is not pulled in implicitly.  This
 *  may be a Boost.Spirit bug but it is very easily worked around by
 *  explicitly including the relevant file.
 */
template< typename Iterator >
basic_grammar_formats< Iterator >::basic_grammar_formats ()
{
  namespace ascii = boost::spirit::ascii;

  using boost::phoenix::size;
  using karma::_val;
  using karma::_1;
  using karma::_r1;

  decimal_ %=
    karma::eps (esci_dec_min <= _val && _val <= esci_dec_max)
    << karma::byte_(LOWER_D)
    << karma::right_align (3, DIGIT_0)
       [ karma::int_generator< integer, 10 > () ]
    ;

  integer_ %=
    karma::eps (esci_int_min <= _val && _val <= esci_int_max)
    << (positive_integer_ | negative_integer_)
    ;

  hexadecimal_ %=
    karma::eps (esci_hex_min <= _val && _val <= esci_hex_max)
    << karma::byte_(LOWER_X)
    << ascii::upper [ karma::right_align (7, DIGIT_0)
                      [ karma::int_generator< integer, 16 > () ] ]
    ;

  numeric_ %=
    decimal_
    | integer_
    | hexadecimal_
    ;

  positive_ %=
    decimal_
    | positive_integer_
    | hexadecimal_
    ;

  negative_ %=
    negative_integer_
    ;

  positive_integer_ %=
    karma::eps (0 <= _val && _val <= esci_int_max)
    << karma::byte_(LOWER_I)
    << karma::right_align (7, DIGIT_0)
       [ karma::int_generator< integer, 10 > () ]
    ;

  negative_integer_ %=
    karma::eps (esci_int_min <= _val && _val < 0)
    << karma::byte_(LOWER_I)
    << karma::byte_(MINUS)
    << karma::right_align (6, DIGIT_0)
       [ karma::int_generator< integer, 10 > () [ _1 = -_val ] ]
    ;

  bin_hex_data_ %=
    custom_generator::no_attribute [ bin_hex_size_[ _1 = size (_val) ] ]
    << *karma::byte_
    << bin_hex_padding_(size (_val))
    ;

  bin_hex_size_ %=
    karma::eps (esci_hex_min <= _val && _val <= esci_hex_max)
    << karma::byte_(LOWER_H)
    << ascii::upper [ karma::right_align (3, DIGIT_0)
                      [ karma::int_generator< integer, 16 > () ] ]
    ;

  bin_hex_padding_ %=
    karma::repeat (3 - (_r1 + 3) % 4) [ karma::byte_(0) ]
    ;

  ESCI_GRAMMAR_TRACE_NODE (decimal_);
  ESCI_GRAMMAR_TRACE_NODE (integer_);
  ESCI_GRAMMAR_TRACE_NODE (hexadecimal_);
  ESCI_GRAMMAR_TRACE_NODE (numeric_);
  ESCI_GRAMMAR_TRACE_NODE (positive_);
  ESCI_GRAMMAR_TRACE_NODE (negative_);
  ESCI_GRAMMAR_TRACE_NODE (positive_integer_);
  ESCI_GRAMMAR_TRACE_NODE (negative_integer_);

  ESCI_GRAMMAR_TRACE_NODE (bin_hex_data_);

  ESCI_GRAMMAR_TRACE_NODE (bin_hex_size_);
  ESCI_GRAMMAR_TRACE_NODE (bin_hex_padding_);
}

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_formats_ipp_ */
