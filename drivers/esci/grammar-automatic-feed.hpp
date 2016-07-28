//  grammar-automatic-feed.hpp -- component rule declarations
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

#ifndef drivers_esci_grammar_automatic_feed_hpp_
#define drivers_esci_grammar_automatic_feed_hpp_

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

namespace encoding {

namespace karma = boost::spirit::karma;

template< typename Iterator >
class basic_grammar_automatic_feed
  : virtual protected basic_grammar_formats< Iterator >
{
public:
  basic_grammar_automatic_feed ();

  bool
  automatic_feed_(Iterator payload, const quad& mode)
  {
    return this->generate (payload, automatic_feed_rule_, mode);
  }

  using basic_grammar_formats< Iterator >::trace;

protected:
  typedef karma::rule< Iterator, quad () > token_rule_;

  karma::rule< Iterator, quad () > automatic_feed_rule_;

  //! Check mode tokens for validity
  karma::symbols< quad, token_rule_ > mode_token_;
};

extern template class basic_grammar_automatic_feed< default_iterator_type >;

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_automatic_feed_hpp_ */
