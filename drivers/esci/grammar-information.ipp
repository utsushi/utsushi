//  grammar-information.ipp -- component rule definitions
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#ifndef drivers_esci_grammar_information_ipp_
#define drivers_esci_grammar_information_ipp_

//! \copydoc grammar.ipp

//  decoding::basic_grammar_information<T> implementation requirements
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/include/qi_difference.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_matches.hpp>
#include <boost/spirit/include/qi_permutation.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_skip.hpp>

//  *::basic_grammar_information<T> implementation requirements
#include <boost/fusion/include/adapt_struct.hpp>

//  Support for debugging of parser rules
#include <boost/spirit/include/qi_nonterminal.hpp>

#include "grammar-information.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
basic_grammar_information< Iterator >::basic_grammar_information ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::information;
  namespace value = code_token::value;

  information_rule_ %=
    (  (token_(ADF) > info_adf_rule_)
     ^ (token_(TPU) > info_tpu_rule_)
     ^ (token_(FB ) > info_fb_rule_)
     ^ (token_(IMX) > this->extent_)
     ^  qi::matches [ token_(PB) ]
     ^ (token_(PRD) > this->bin_hex_data_)
     ^ (token_(VER) > this->bin_hex_data_)
     ^ (token_(DSZ) > this->positive_)
     ^ (token_(EXT) > token_(value::LIST) > +info_ext_token_)
     ^  qi::matches [ token_(DLS) ]
     ^ (token_(S_N) > this->bin_hex_data_)
       )
    > qi::eoi
    ;

  info_adf_rule_ %=
    qi::skip (token_(ADF))
    [   (token_(adf::TYPE) > info_adf_type_token_)
      ^ (token_(adf::DPLX) > info_adf_dplx_token_)
      ^ (token_(adf::FORD) > info_adf_ford_token_)
      ^  qi::matches [ token_(adf::PREF) ]
      ^  qi::matches [ token_(adf::DETX) ]
      ^  qi::matches [ token_(adf::DETY) ]
      ^ (token_(adf::ALGN) > info_adf_algn_token_)
      ^  qi::matches [ token_(adf::ASCN) ]
      ^ (token_(adf::AREA) > this->extent_)
      ^ (token_(adf::AMIN) > this->extent_)
      ^ (token_(adf::AMAX) > this->extent_)
      ^ (token_(adf::RESO) > this->positive_)
      ^  qi::matches [ token_(adf::RCVR) ]
      ^ (token_(adf::OVSN) > this->extent_)
     ];

  info_tpu_rule_ %=
    qi::skip (token_(TPU))
    [   (token_(tpu::ARE1) > this->extent_)
      ^ (token_(tpu::ARE2) > this->extent_)
      ^ (token_(tpu::RESO) > this->positive_)
      ^ (token_(tpu::OVSN) > this->extent_)
     ];

  info_fb_rule_ %=
    qi::skip (token_(FB))
    [    qi::matches [ token_(fb::DETX) ]
      ^  qi::matches [ token_(fb::DETY) ]
      ^ (token_(fb::ALGN) > info_fb_algn_token_)
      ^ (token_(fb::AREA) > this->extent_)
      ^ (token_(fb::RESO) > this->positive_)
      ^ (token_(fb::OVSN) > this->extent_)
     ];

  extent_ %=
    this->positive_
    > this->positive_
    ;

  info_adf_type_token_ %=
    &(  token_(adf::PAGE)
      | token_(adf::FEED)
      )
    > token_
    ;

  info_adf_dplx_token_ %=
    &(  token_(adf::SCN1)
      | token_(adf::SCN2)
      )
    > token_
    ;

  info_adf_ford_token_ %=
    &(  token_(adf::PF1N)
      | token_(adf::PFN1)
      )
    > token_
    ;

  info_adf_algn_token_ %=
    &(  token_(adf::LEFT)
      | token_(adf::CNTR)
      | token_(adf::RIGT)
      )
    > token_
    ;

  info_fb_algn_token_ %=
    &(  token_(fb::LEFT)
      | token_(fb::CNTR)
      | token_(fb::RIGT)
      )
    > token_
    ;

  info_ext_token_ %=
    &(  token_(ext::EXT0)
      | token_(ext::EXT1)
      | token_(ext::EXT2)
      )
    > token_
    ;

  ESCI_GRAMMAR_TRACE_NODE (information_rule_);

  ESCI_GRAMMAR_TRACE_NODE (info_adf_rule_);
  ESCI_GRAMMAR_TRACE_NODE (info_tpu_rule_);
  ESCI_GRAMMAR_TRACE_NODE (info_fb_rule_);

  ESCI_GRAMMAR_TRACE_NODE (extent_);

  ESCI_GRAMMAR_TRACE_NODE (info_adf_type_token_);
  ESCI_GRAMMAR_TRACE_NODE (info_adf_dplx_token_);
  ESCI_GRAMMAR_TRACE_NODE (info_adf_ford_token_);
  ESCI_GRAMMAR_TRACE_NODE (info_adf_algn_token_);
  ESCI_GRAMMAR_TRACE_NODE (info_fb_algn_token_);
  ESCI_GRAMMAR_TRACE_NODE (info_ext_token_);
}

}       // namespace decoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#define ESCI_NS utsushi::_drv_::esci

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::information::tpu_source,
 (std::vector< ESCI_NS::integer >, area)
 (std::vector< ESCI_NS::integer >, alternative_area)
 (ESCI_NS::integer, resolution)
 (std::vector< ESCI_NS::integer >, overscan))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::information::fb_source,
 (bool, detects_width)
 (bool, detects_height)
 (ESCI_NS::quad, alignment)
 (std::vector< ESCI_NS::integer >, area)
 (ESCI_NS::integer, resolution)
 (std::vector< ESCI_NS::integer >, overscan))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::information::adf_source,
 (ESCI_NS::quad, type)
 (boost::optional< ESCI_NS::quad >, duplex_passes)
 (ESCI_NS::quad, doc_order)
 (bool, prefeeds)
 (bool, detects_width)
 (bool, detects_height)
 (ESCI_NS::quad, alignment)
 (bool, auto_scans)
 (std::vector< ESCI_NS::integer >, area)
 (std::vector< ESCI_NS::integer >, min_doc)
 (std::vector< ESCI_NS::integer >, max_doc)
 (ESCI_NS::integer, resolution)
 (bool, auto_recovers)
 (std::vector< ESCI_NS::integer >, overscan))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::information,
 (boost::optional< ESCI_NS::information::adf_source >, adf)
 (boost::optional< ESCI_NS::information::tpu_source >, tpu)
 (boost::optional< ESCI_NS::information::fb_source >, flatbed)
 (std::vector< ESCI_NS::integer >, max_image)
 (bool, have_push_button)
 (std::vector< ESCI_NS::byte >, product)
 (std::vector< ESCI_NS::byte >, version)
 (ESCI_NS::integer, device_buffer_size)
 (std::vector< ESCI_NS::quad >, extension)
 (bool, truncates_at_media_end)
 (boost::optional< std::vector< ESCI_NS::byte > >, serial_number))

#undef ESCI_NS

#endif  /* drivers_esci_grammar_information_ipp_ */
