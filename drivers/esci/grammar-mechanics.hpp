//  grammar-mechanics.hpp -- component rule declarations
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

#ifndef drivers_esci_grammar_mechanics_hpp_
#define drivers_esci_grammar_mechanics_hpp_

//! \copydoc grammar.hpp

#include <boost/operators.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_symbols.hpp>

#include "buffer.hpp"
#include "code-token.hpp"
#include "grammar-formats.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

struct hardware_request
  : private boost::equality_comparable< hardware_request >
{
  hardware_request ();

  bool operator== (const hardware_request& rhs) const;

  void clear ();

  struct focus
    : private boost::equality_comparable< focus >
  {
    focus ();
    focus (const integer& pos);

    bool operator== (const focus& rhs) const;

    quad type;
    boost::optional< integer > position;
  };

  boost::optional< quad > adf;
  boost::optional< focus > fcs;
  bool ini;
};

namespace encoding {

namespace karma = boost::spirit::karma;

template< typename Iterator >
class basic_grammar_mechanics
  : virtual protected basic_grammar_formats< Iterator >
{
public:
  basic_grammar_mechanics ();

  //! Prepares a payload to go with a \c MECH request
  /*! \sa code_token::mechanic
   */
  bool hardware_control_(Iterator payload, const hardware_request& request)
  {
    return this->generate (payload, hardware_control_rule_, request);
  }

protected:
  typedef karma::rule< Iterator, quad () > token_rule_;

  karma::rule< Iterator, hardware_request () > hardware_control_rule_;

  karma::symbols< quad, token_rule_ > mech_adf_token_;
  karma::symbols< quad, token_rule_ > mech_fcs_token_;
};

extern template class basic_grammar_mechanics< default_iterator_type >;

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_mechanics_hpp_ */
