//  grammar-formats.hpp -- component rule declarations
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

#ifndef drivers_esci_grammar_formats_hpp_
#define drivers_esci_grammar_formats_hpp_

//! \copydoc grammar.hpp

#include <sstream>
#include <string>

#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_rule.hpp>

#include <utsushi/cstdint.hpp>

#include "buffer.hpp"

#if ESCI_GRAMMAR_TRACE
#include "grammar-tracer.hpp"
#define ESCI_GRAMMAR_TRACE_NODE(r)                      \
  {                     /* enable tracing support */    \
    r.name (#r);                                        \
    debug (r, grammar_tracer (this->trace_));           \
  }                                                     \
  /**/
#else
#define ESCI_GRAMMAR_TRACE_NODE(r)  /* never mind */
#endif  /* ESCI_GRAMMAR_TRACE */

//! Message to use when grammar tracing support was not enabled
#define ESCI_GRAMMAR_TRACE_DISABLED                             \
  "grammar tracing support was disabled at compile-time"        \
  /**/

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Integral values used in the "compound" protocol variants
/*! The "compound" protocol variants use several different mechanisms
 *  to shuttle integral values between the driver and the device.  In
 *  the driver code one normally cares more about the value than what
 *  particular mechanism needs to be used.  The integer type provides
 *  enough flexibility to handle all values supported by the various
 *  mechanisms.
 */
typedef int32_t integer;

//  Code literals defining extreme values of all four integral coding
//  mechanisms used by the "compound" protocol variants.  Note, these
//  need to be _literals_ so that the compiler can choose an integral
//  type that is sufficiently large to represent the numeric value.

#define esci_dec_min          0
#define esci_dec_max        999
#define esci_int_min    -999999
#define esci_int_max    9999999
#define esci_hex_min  0x0000000
#define esci_hex_max  0xFFFFFFF
#define esci_bin_min      0x000
#define esci_bin_max      0xFFF

//  A code literal value that lies outside all the integral coding
//  ranges.  From the protocol point of view, this is a non-integer.
//  The definition is mostly meant to aid unit test implementations.

#define esci_non_int -esci_hex_max

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
class basic_grammar_formats
{
public:
  typedef Iterator iterator;

  //! Codes a "decimal" number
  /*! Decimal coding starts with a literal \c d and is followed by
   *  three decimal digits (DIGIT_0 through DIGIT_9).  Values that
   *  require less than three digits use leading DIGIT_0's to fill
   *  the remaining slots.
   *
   *  \note This coding only supports non-negative numbers.
   */
  qi::rule< Iterator, integer () > decimal_;

  //! Codes an "integral" number
  /*! This coding starts with a literal \c i and is followed by seven
   *  decimal digits for a positive value \e or a literal minus, \c -,
   *  followed by six decimal digits for negative values.  Any value
   *  requiring less digits uses leading DIGIT_0's to fill remaining
   *  slots.
   *
   *  The decimal digits are DIGIT_0 through DIGIT_9.
   *
   *  \note This is the only numeric_ coding that supports negative_
   *        numbers.
   */
  qi::rule< Iterator, integer () > integer_;

  //! Codes a "hexadecimal" number
  /*! Starting with a literal \c x, the coding continues with seven
   *  hexadecimal digits.  The hexadecimal digits are DIGIT_0 through
   *  DIGIT_9 and UPPER_A through UPPER_F.  The corresponding lower
   *  case code points, LOWER_A through LOWER_F, are not supported.
   *  Values that require less than seven digits fill up remaining
   *  slots with leading DIGIT_0's.
   *
   *  \note This coding only supports non-negative numbers.
   */
  qi::rule< Iterator, integer () > hexadecimal_;

  //! Codes any supported integral value
  qi::rule< Iterator, integer () > numeric_;

  //! Codes any supported integral value not less than zero
  /*! \note Zero is treated as if positive.
   */
  qi::rule< Iterator, integer () > positive_;

  //! Codes any supported integral value less than zero
  qi::rule< Iterator, integer () > negative_;

  //! Codes a non-negative integer_
  qi::rule< Iterator, integer () > positive_integer_;

  //! Codes a negative integer_
  qi::rule< Iterator, integer () > negative_integer_;

  //! Codes a sequence of arbitrary bytes
  qi::rule< Iterator, std::vector< byte > () > bin_hex_data_;

  basic_grammar_formats ();

  template< typename Expression >
  bool
  parse (Iterator& first, Iterator last, const Expression& expr)
  {
    if (ESCI_GRAMMAR_TRACE) trace_.str (std::string ());
    return qi::parse (first, last, expr);
  }

  template< typename Expression, typename Attribute >
  bool
  parse (Iterator& first, Iterator last, const Expression& expr,
         Attribute& attr)
  {
    if (ESCI_GRAMMAR_TRACE) trace_.str (std::string ());
    return qi::parse (first, last, expr, attr);
  }

  std::string trace () const
  {
    return (ESCI_GRAMMAR_TRACE
            ? trace_.str ()
            : ESCI_GRAMMAR_TRACE_DISABLED);
  }

protected:
  //! Codes the numeric part of a positive_integer_
  qi::rule< Iterator, integer () > positive_number_;

  //! Codes the numeric part of a negative_integer_
  qi::rule< Iterator, integer () > negative_number_;

  //! Stores the size of a byte sequence
  /*! This value is used in the bin_hex_payload_ and bin_hex_padding_
   *  rules to determine the number of bytes they need to process.
   */
  integer bin_hex_sz_;

  //! Codes the size of a byte sequence
  qi::rule< Iterator > bin_hex_size_;

  //! Codes a byte sequence's trailing padding bytes
  qi::rule< Iterator > bin_hex_padding_;

  //! Collects a sequence of arbitrary bytes
  qi::rule< Iterator, std::vector< byte > () > bin_hex_payload_;

  std::stringstream trace_;
};

extern template class basic_grammar_formats< default_iterator_type >;

}       // namespace decoding

namespace encoding {

namespace karma = boost::spirit::karma;

template< typename Iterator >
class basic_grammar_formats
{
public:
  typedef Iterator iterator;

  //! \copydoc decoding::basic_grammar_formats::decimal_
  karma::rule< Iterator, integer () > decimal_;

  //! \copydoc decoding::basic_grammar_formats::integer_
  karma::rule< Iterator, integer () > integer_;

  //! \copydoc decoding::basic_grammar_formats::hexadecimal_
  karma::rule< Iterator, integer () > hexadecimal_;

  //! \copydoc decoding::basic_grammar_formats::numeric_
  karma::rule< Iterator, integer () > numeric_;

  //! \copydoc decoding::basic_grammar_formats::positive_
  karma::rule< Iterator, integer () > positive_;

  //! \copydoc decoding::basic_grammar_formats::negative_
  karma::rule< Iterator, integer () > negative_;

  //! \copydoc decoding::basic_grammar_formats::positive_integer_
  karma::rule< Iterator, integer () > positive_integer_;

  //! \copydoc decoding::basic_grammar_formats::negative_integer_
  karma::rule< Iterator, integer () > negative_integer_;

  //! \copydoc decoding::basic_grammar_formats::bin_hex_data_
  karma::rule< Iterator, std::vector< byte > () > bin_hex_data_;

  basic_grammar_formats ();

  template< typename Expression >
  bool
  generate (Iterator sink, const Expression& expr)
  {
    if (ESCI_GRAMMAR_TRACE) trace_.str (std::string ());
    return karma::generate (sink, expr);
  }

  template< typename Expression, typename Attribute >
  bool
  generate (Iterator sink, const Expression& expr, const Attribute& attr)
  {
    if (ESCI_GRAMMAR_TRACE) trace_.str (std::string ());
    return karma::generate (sink, expr, attr);
  }

  inline
  std::string trace () const
  {
    return (ESCI_GRAMMAR_TRACE
            ? trace_.str ()
            : ESCI_GRAMMAR_TRACE_DISABLED);
  }

protected:
  typedef std::vector< byte >::size_type size_type;

  //! \copydoc decoding::basic_grammar_formats::bin_hex_size_
  karma::rule< Iterator, integer () > bin_hex_size_;

  //! \copydoc decoding::basic_grammar_formats::bin_hex_padding_
  karma::rule< Iterator, void (size_type) > bin_hex_padding_;

  std::ostringstream trace_;
};

extern template class basic_grammar_formats< default_iterator_type >;

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_formats_hpp_ */
