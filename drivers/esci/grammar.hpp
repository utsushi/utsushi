//  grammar.hpp -- rule declarations for the ESC/I "compound" protocol
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

#ifndef drivers_esci_grammar_hpp_
#define drivers_esci_grammar_hpp_

/*! \file
 *  This file provides the template \e declarations for the grammar's
 *  rules.  If you only need a parser that uses the preferred iterator
 *  type just include this file.  When another iterator type is called
 *  for, include the corresponding \c *.ipp file instead.
 *
 *  \sa decoding::default_iterator_type
 *  \sa encoding::default_iterator_type
 */

#include <vector>

#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/karma_delimit.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_symbols.hpp>
#include <boost/spirit/include/qi_rule.hpp>

#include "buffer.hpp"
#include "code-token.hpp"
#include "grammar-automatic-feed.hpp"
#include "grammar-capabilities.hpp"
#include "grammar-information.hpp"
#include "grammar-mechanics.hpp"
#include "grammar-formats.hpp"
#include "grammar-parameters.hpp"
#include "grammar-status.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Combine code token and payload size in a single entity
struct header
  : private boost::equality_comparable< header >
{
  header (const quad& q = quad (), integer i = 0)
    : code (q)
    , size (i)
  {}

  bool operator== (const header& rhs) const;

  quad    code;
  integer size;
};

struct status
  : private boost::equality_comparable< status >
{
  bool operator== (const esci::status& rhs) const;

  //! Says whether a fatal error has been detected by the device
  /*! This function only returns true for definitely fatal errors.
   *
   *  Note that running out of media is to be expected.  As such, we
   *  should not treat it as a fatal error without any qualification.
   *  We only signal out-of-media as a fatal error if there is reason
   *  to believe that more images still need to be acquired.  If that
   *  information is not present, out-of-media *may* still be a fatal
   *  error though.  The condition may occur at a point where not all
   *  images have been acquired completely.  However, status objects
   *  lack the information needed to detect this situation.
   *
   *  \sa scanner_control::fatal_error()
   */
  bool fatal_error () const;
  bool is_busy () const;
  bool is_cancel_requested () const;
  bool is_flip_side () const;
  bool is_in_use () const;
  bool is_parameter_set_okay () const;
  bool is_ready () const;
  bool is_warming_up () const;
  bool is_normal_sheet () const;

  //! Indicates whether the device ran out of media
  /*! Note that this may or may not be a fatal_error()
   */
  bool media_out () const;
  bool media_out (const quad& where) const;

  void clear ();
  void check (const header& reply) const;

  struct error
    : private boost::equality_comparable< error >
  {
    error (const quad& part = quad (), const quad& what = quad ())
      : part (part)
      , what (what)
    {}

    bool operator== (const error& rhs) const;

    quad part;
    quad what;
  };

  struct image
    : private boost::equality_comparable< image >
  {
    image (integer width = esci_non_int,
           integer height = esci_non_int,
           integer padding = esci_non_int)
      : width (width)
      , height (height)
      , padding (padding)
    {}

    bool operator== (const image& rhs) const;

    integer width;
    integer height;
    integer padding;
  };

  std::vector< error > err;
  boost::optional< quad > nrd;
  boost::optional< image > pst;
  boost::optional< image > pen;
  boost::optional< integer > lft;
  boost::optional< quad > typ;
  boost::optional< quad > atn;
  boost::optional< quad > par;
  boost::optional< quad > doc;

private:
  bool is_get_parameter_code (const quad& code) const;
  bool is_set_parameter_code (const quad& code) const;
  bool is_parameter_code (const quad& code) const;
};

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
class basic_grammar
  : virtual protected basic_grammar_formats< Iterator >
  , public basic_grammar_information< Iterator >
  , public basic_grammar_capabilities< Iterator >
  , public basic_grammar_parameters< Iterator >
  , public basic_grammar_status< Iterator >
{
public:
  typedef Iterator iterator;
  typedef qi::expectation_failure< Iterator > expectation_failure;

  basic_grammar ();

  bool
  header_(Iterator& head, const Iterator& tail,
          header& h)
  {
    return this->parse (head, tail, header_rule_, h);
  }

  bool
  status_(Iterator& head, const Iterator& tail,
          status& s)
  {
    return this->parse (head, tail, status_rule_, s);
  }

  using basic_grammar_formats< Iterator >::trace;

protected:
  qi::rule< Iterator, header () > header_rule_;
  qi::rule< Iterator, status () > status_rule_;

  qi::rule< Iterator, status::error () > err_rule_;
  qi::rule< Iterator, status::image () > pst_rule_;
  qi::rule< Iterator, status::image () > pen_rule_;

  qi::rule< Iterator > skip_rule_;

  qi::rule< Iterator, quad () > reply_token_;
  qi::rule< Iterator, quad () > info_token_;
  qi::rule< Iterator, quad () > err_part_token_;
  qi::rule< Iterator, quad () > err_what_token_;
  qi::rule< Iterator, quad () > nrd_token_;
  qi::rule< Iterator, quad () > typ_token_;
  qi::rule< Iterator, quad () > atn_token_;
  qi::rule< Iterator, quad () > par_token_;
  qi::rule< Iterator, quad () > doc_token_;
};

typedef basic_grammar< default_iterator_type > grammar;
extern template class basic_grammar< default_iterator_type >;

}       // namespace decoding

namespace encoding {

namespace karma = boost::spirit::karma;

template< typename Iterator >
class basic_grammar
  : virtual protected basic_grammar_formats< Iterator >
  , public basic_grammar_parameters< Iterator >
  , public basic_grammar_automatic_feed< Iterator >
  , public basic_grammar_mechanics< Iterator >
{
public:
  typedef Iterator iterator;

  basic_grammar ();

  //! Ask the device to do something
  /*! This rule creates a protocol compliant request header.  These
   *  headers combine a token from the code_token::request namespace
   *  with the hexadecimally encoded size of an optional payload.
   *
   *  Note that this rule does \e not deal with the encoding of such
   *  payloads.
   */
  bool
  header_(Iterator payload, const header& h)
  {
    return this->generate (payload, header_rule_, h);
  }

  using basic_grammar_formats< Iterator >::trace;

protected:
  typedef karma::rule< Iterator, quad () > token_rule_;

  karma::rule< Iterator, header () > header_rule_;

  //! Check code tokens for validity as a request token
  karma::symbols< quad, token_rule_ > request_token_;
};

typedef basic_grammar< default_iterator_type > grammar;
extern template class basic_grammar< default_iterator_type >;

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_hpp_ */
