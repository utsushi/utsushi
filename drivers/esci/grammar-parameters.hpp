//  grammar-parameters.hpp -- component rule declarations
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_grammar_parameters_hpp_
#define drivers_esci_grammar_parameters_hpp_

//! \copydoc grammar.hpp

#include <set>
#include <vector>

#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_symbols.hpp>
#include <boost/spirit/include/qi_rule.hpp>

#include <utsushi/cstdint.hpp>
#include <utsushi/quantity.hpp>

#include "buffer.hpp"
#include "code-point.hpp"
#include "code-token.hpp"
#include "grammar-formats.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

struct parameters
  : private boost::equality_comparable< parameters >
{
  bool operator== (const parameters& rhs) const;

  void clear ();

  bool is_bilevel () const;
  bool is_color () const;
  quad source () const;

  quantity border_left (const quantity& default_value) const;
  quantity border_right (const quantity& default_value) const;
  quantity border_top (const quantity& default_value) const;
  quantity border_bottom (const quantity& default_value) const;

  struct gamma_table
    : private boost::equality_comparable< gamma_table >
  {
    gamma_table (const quad& q = quad ());

    bool operator== (const gamma_table& rhs) const;

    quad component;
    std::vector< byte > table;
  };

  struct color_matrix
    : private boost::equality_comparable< color_matrix >
  {
    color_matrix (const quad& q = quad ());

    bool operator== (const color_matrix& rhs) const;

    quad type;
    boost::optional< std::vector< byte > > matrix;
  };

  boost::optional< std::vector< quad > > adf;
  boost::optional< std::vector< quad > > tpu;
  boost::optional< std::vector< quad > > fb;
  boost::optional< quad > col;
  boost::optional< quad > fmt;
  boost::optional< integer > jpg;
  boost::optional< integer > thr;
  boost::optional< quad > dth;
  boost::optional< quad > gmm;
  boost::optional< std::vector< gamma_table > > gmt;
  boost::optional< color_matrix > cmx;
  boost::optional< quad > sfl;
  boost::optional< quad > mrr;
  boost::optional< integer > bsz;
  boost::optional< integer > pag;
  boost::optional< integer > rsm;
  boost::optional< integer > rss;
  boost::optional< integer > crp;
  boost::optional< std::vector< integer > > acq;
  boost::optional< quad > flc;
  boost::optional< std::vector< integer > > fla;
  boost::optional< quad > qit;
  boost::optional< integer > ldf;
  boost::optional< std::vector< integer > > dfa;
  boost::optional< quad > lam;
};

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
class basic_grammar_parameters
  : virtual protected basic_grammar_formats< Iterator >
{
public:
  basic_grammar_parameters ();

  //! Parses a reply payload for a scan parameter getter request
  /*! \sa code_token::parameter
   */
  bool
  scan_parameters_(Iterator& head, const Iterator& tail,
                   parameters& values)
  {
    return this->parse (head, tail, parameters_rule_, values);
  }

protected:
  qi::rule< Iterator, parameters () > parameters_rule_;

  qi::rule< Iterator, parameters::gamma_table () > gamma_table_rule_;
  qi::rule< Iterator, parameters::color_matrix () > color_matrix_rule_;

  qi::rule< Iterator, quad () > parm_adf_token_;
  qi::rule< Iterator, quad () > parm_tpu_token_;
  qi::rule< Iterator, quad () > parm_fb_token_;
  qi::rule< Iterator, quad () > parm_col_token_;
  qi::rule< Iterator, quad () > parm_fmt_token_;
  qi::rule< Iterator, quad () > parm_dth_token_;
  qi::rule< Iterator, quad () > parm_gmm_token_;
  qi::rule< Iterator, quad () > parm_gmt_token_;
  qi::rule< Iterator, quad () > parm_cmx_token_;
  qi::rule< Iterator, quad () > parm_sfl_token_;
  qi::rule< Iterator, quad () > parm_mrr_token_;
  qi::rule< Iterator, quad () > parm_flc_token_;
  qi::rule< Iterator, quad () > parm_qit_token_;
  qi::rule< Iterator, quad () > parm_lam_token_;
};

extern template class basic_grammar_parameters< default_iterator_type >;

}       // namespace decoding

namespace encoding {

namespace karma = boost::spirit::karma;

template< typename Iterator >
class basic_grammar_parameters
  : virtual protected basic_grammar_formats< Iterator >
{
public:
  basic_grammar_parameters ();

  //! Put together a request payload to set scan parameters
  /*! \sa code_token::parameter
   */
  bool
  scan_parameters_(Iterator payload, const parameters& values)
  {
    return this->generate (payload, parameters_rule_, values);
  }

  //! Preps a request payload to fetch selected scan parameters
  /*! \sa code_token::parameter
   */
  bool
  parameter_subset_(Iterator payload, const std::set< quad >& tokens)
  {
    return this->generate (payload, parameter_subset_rule_, tokens);
  }

protected:
  typedef karma::rule< Iterator, quad () > token_rule_;

  karma::rule< Iterator, parameters () > parameters_rule_;
  karma::rule< Iterator, std::set< quad > () > parameter_subset_rule_;

  //! Check code tokens for validity as a parameter code
  karma::symbols< quad, token_rule_ > parameter_token_;

  karma::symbols< quad, token_rule_ > parm_adf_token_;
  karma::symbols< quad, token_rule_ > parm_tpu_token_;
  karma::symbols< quad, token_rule_ > parm_fb_token_;
  karma::symbols< quad, token_rule_ > parm_col_token_;
  karma::symbols< quad, token_rule_ > parm_fmt_token_;
  karma::symbols< quad, token_rule_ > parm_dth_token_;
  karma::symbols< quad, token_rule_ > parm_gmm_token_;
  karma::symbols< quad, token_rule_ > parm_gmt_token_;
  karma::symbols< quad, token_rule_ > parm_cmx_token_;
  karma::symbols< quad, token_rule_ > parm_sfl_token_;
  karma::symbols< quad, token_rule_ > parm_mrr_token_;
  karma::symbols< quad, token_rule_ > parm_flc_token_;
  karma::symbols< quad, token_rule_ > parm_qit_token_;
  karma::symbols< quad, token_rule_ > parm_lam_token_;
};

extern template class basic_grammar_parameters< default_iterator_type >;

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_parameters_hpp_ */
