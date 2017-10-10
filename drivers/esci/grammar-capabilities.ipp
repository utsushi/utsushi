//  grammar-capabilities.ipp -- component rule definitions
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

#ifndef drivers_esci_grammar_capabilities_ipp_
#define drivers_esci_grammar_capabilities_ipp_

//! \copydoc grammar.ipp

//  decoding::basic_grammar_capabilities<T> implementation requirements
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

//  *::basic_grammar_capabilities<T> implementation requirements
#include <boost/fusion/include/adapt_struct.hpp>

//  Support for debugging of parser rules
#include <boost/spirit/include/qi_nonterminal.hpp>

#include "grammar-capabilities.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
basic_grammar_capabilities< Iterator >::basic_grammar_capabilities ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::capability;
  namespace value = code_token::value;

  capability_rule_ %=
    (  (token_(ADF) > caps_adf_rule_)
     ^ (token_(TPU) > caps_tpu_rule_)
     ^ (token_(FB ) > caps_fb_rule_)
     ^ (token_(COL) > token_(value::LIST) > +caps_col_token_)
     ^ (token_(FMT) > token_(value::LIST) > +caps_fmt_token_)
     ^ (token_(JPG) > this->decimal_range_)
     ^ (token_(THR) > this->decimal_range_)
     ^ (token_(DTH) > token_(value::LIST) > +caps_dth_token_)
     ^ (token_(GMM) > token_(value::LIST) > +caps_gmm_token_)
     ^ (token_(GMT) > token_(value::LIST) > +caps_gmt_token_)
     ^ (token_(CMX) > token_(value::LIST) > +caps_cmx_token_)
     ^ (token_(SFL) > token_(value::LIST) > +caps_sfl_token_)
     ^ (token_(MRR) > token_(value::LIST) > +caps_mrr_token_)
     ^ (token_(BSZ) > (this->range_ | this->numeric_list_))
     ^ (token_(PAG) > (this->range_ | this->numeric_list_))
     ^ (token_(RSM) > (this->positive_range_ | this->positive_list_))
     ^ (token_(RSS) > (this->positive_range_ | this->positive_list_))
     ^ (token_(CRP) > (this->range_ | this->numeric_list_))
     ^ (token_(FCS) > caps_fcs_rule_)
     ^ (token_(FLC) > token_(value::LIST) > +caps_flc_token_)
     ^ (token_(FLA) > (this->positive_range_ | this->positive_list_))
     ^ (token_(QIT) > token_(value::LIST) > +caps_qit_token_)
     ^ (token_(LAM) > token_(value::LIST) > +caps_lam_token_)
     )
    > qi::eoi
    ;

  caps_adf_rule_ %=
    qi::skip (token_(ADF))
    [   *(caps_adf_token_)
      ^  (token_(adf::RSMS) > (this->positive_range_ | this->positive_list_))
      ]
    ;

  caps_tpu_rule_ %=
    qi::skip (token_(TPU))
    [   *(caps_tpu_token_)
      ^  (token_(tpu::RSMS) > (this->positive_range_ | this->positive_list_))
      ^  (token_(tpu::ARE1) > +caps_tpu_area_token_)
      ^  (token_(tpu::ARE2) > +caps_tpu_area_token_)
      ]
    ;

  caps_fb_rule_ %=
    qi::skip (token_(FB ))
    [   *(caps_fb_token_)
      ^  (token_(fb::RSMS) > (this->positive_range_ | this->positive_list_))
      ]
    ;

  caps_fcs_rule_ %=
    qi::skip (token_(FCS))
    [    qi::matches [ token_(fcs::AUTO) ]
      ^ (this->decimal_range_ | this->decimal_list_)
      ]
    ;

  numeric_list_ %=
    token_(value::LIST)
    > +this->numeric_
    ;

  decimal_list_ %=
    token_(value::LIST)
    > +this->decimal_
    ;

  positive_list_ %=
    token_(value::LIST)
    > +this->positive_
    ;

  range_ %=
    token_(value::RANG)
    > this->numeric_
    > this->numeric_
    ;

  decimal_range_ %=
    token_(value::RANG)
    > this->decimal_
    > this->decimal_
    ;

  positive_range_ %=
    token_(value::RANG)
    > this->positive_
    > this->positive_
    ;

  caps_adf_token_ %=
    &(  token_(adf::DPLX)
      | token_(adf::PEDT)
      | token_(adf::DFL0)
      | token_(adf::DFL1)
      | token_(adf::DFL2)
      | token_(adf::LDF )
      | token_(adf::SDF )
      | token_(adf::SPP )
      | token_(adf::FAST)
      | token_(adf::SLOW)
      | token_(adf::BGWH)
      | token_(adf::BGBK)
      | token_(adf::BGGY)
      | token_(adf::LOAD)
      | token_(adf::EJCT)
      | token_(adf::CRP )
      | token_(adf::SKEW)
      | token_(adf::OVSN)
      | token_(adf::CLEN)
      | token_(adf::CALB)
      )
    > token_
    ;

  caps_tpu_token_ %=
    &(  token_(tpu::MAGC)
      | token_(tpu::FAST)
      | token_(tpu::SLOW)
      | token_(tpu::CRP )
      | token_(tpu::SKEW)
      | token_(tpu::OVSN)
      )
    > token_
    ;

  caps_tpu_area_token_ %=
    &(  token_(tpu::NEGL)
      | token_(tpu::IR  )
      )
    > token_
    ;

  caps_fb_token_ %=
    &(  token_(fb::LMP1)
      | token_(fb::LMP2)
      | token_(fb::FAST)
      | token_(fb::SLOW)
      | token_(fb::CRP )
      | token_(fb::SKEW)
      | token_(fb::OVSN)
      )
    > token_
    ;

  caps_col_token_ %=
    &(  token_(col::C003)
      | token_(col::C024)
      | token_(col::C048)
      | token_(col::M001)
      | token_(col::M008)
      | token_(col::M016)
      | token_(col::R001)
      | token_(col::R008)
      | token_(col::R016)
      | token_(col::G001)
      | token_(col::G008)
      | token_(col::G016)
      | token_(col::B001)
      | token_(col::B008)
      | token_(col::B016)
      )
    > token_
    ;

  caps_fmt_token_ %=
    &(  token_(fmt::RAW)
      | token_(fmt::JPG)
      )
    > token_
    ;

  caps_dth_token_ %=
    &(  token_(dth::NONE)
      | token_(dth::MIDA)
      | token_(dth::MIDB)
      | token_(dth::MIDC)
      | token_(dth::DTHA)
      | token_(dth::DTHB)
      | token_(dth::DTHC)
      | token_(dth::DTHD)
      )
    > token_
    ;

  caps_gmm_token_ %=
    &(  token_(gmm::UG10)
      | token_(gmm::UG18)
      | token_(gmm::UG22)
      )
    > token_
    ;

  caps_gmt_token_ %=
    &(  token_(gmt::RED)
      | token_(gmt::GRN)
      | token_(gmt::BLU)
      | token_(gmt::MONO)
      )
    > token_
    ;

  caps_cmx_token_ %=
    &(  token_(cmx::UNIT)
      | token_(cmx::UM08)
      | token_(cmx::UM16)
      )
    > token_
    ;

  caps_sfl_token_ %=
    &(  token_(sfl::SMT2)
      | token_(sfl::SMT1)
      | token_(sfl::NORM)
      | token_(sfl::SHP1)
      | token_(sfl::SHP2)
      )
    > token_
    ;

  caps_mrr_token_ %=
    &(  token_(mrr::ON)
      | token_(mrr::OFF)
      )
    > token_
    ;

  caps_flc_token_ %=
    &(  token_(flc::WH)
      | token_(flc::BK)
      )
    > token_
    ;

  caps_qit_token_ %=
    &(  token_(qit::PREF)
      | token_(qit::ON  )
      | token_(qit::OFF )
      )
    > token_
    ;

  caps_lam_token_ %=
    &(  token_(lam::ON  )
      | token_(lam::OFF )
      )
    > token_
    ;

  ESCI_GRAMMAR_TRACE_NODE (capability_rule_);

  ESCI_GRAMMAR_TRACE_NODE (caps_adf_rule_);
  ESCI_GRAMMAR_TRACE_NODE (caps_tpu_rule_);
  ESCI_GRAMMAR_TRACE_NODE (caps_fb_rule_);
  ESCI_GRAMMAR_TRACE_NODE (caps_fcs_rule_);

  ESCI_GRAMMAR_TRACE_NODE (numeric_list_);
  ESCI_GRAMMAR_TRACE_NODE (decimal_list_);
  ESCI_GRAMMAR_TRACE_NODE (positive_list_);

  ESCI_GRAMMAR_TRACE_NODE (range_);
  ESCI_GRAMMAR_TRACE_NODE (decimal_range_);
  ESCI_GRAMMAR_TRACE_NODE (positive_range_);

  ESCI_GRAMMAR_TRACE_NODE (caps_adf_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_tpu_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_tpu_area_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_fb_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_col_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_fmt_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_dth_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_gmm_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_gmt_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_cmx_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_sfl_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_mrr_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_flc_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_qit_token_);
  ESCI_GRAMMAR_TRACE_NODE (caps_lam_token_);
}

}       // namespace decoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#define ESCI_NS utsushi::_drv_::esci

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::capabilities::range,
 (ESCI_NS::integer, lower_)
 (ESCI_NS::integer, upper_)
 )

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::capabilities::document_source,
 (boost::optional< std::vector< ESCI_NS::quad > >, flags)
 (boost::optional< ESCI_NS::capabilities::constraint >, resolution))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::capabilities::tpu_source,
 (boost::optional< std::vector< ESCI_NS::quad > >, flags)
 (boost::optional< ESCI_NS::capabilities::constraint >, resolution)
 (boost::optional< std::vector< ESCI_NS::quad > >, area)
 (boost::optional< std::vector< ESCI_NS::quad > >, alternative_area))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::capabilities::focus_control,
 (bool, automatic)
 (boost::optional< ESCI_NS::capabilities::constraint >, position))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::capabilities,
 (boost::optional< ESCI_NS::capabilities::document_source >, adf)
 (boost::optional< ESCI_NS::capabilities::tpu_source >, tpu)
 (boost::optional< ESCI_NS::capabilities::document_source >, fb)
 (boost::optional< std::vector< ESCI_NS::quad > >, col)
 (boost::optional< std::vector< ESCI_NS::quad > >, fmt)
 (boost::optional< ESCI_NS::capabilities::range >, jpg)
 (boost::optional< ESCI_NS::capabilities::range >, thr)
 (boost::optional< std::vector< ESCI_NS::quad > >, dth)
 (boost::optional< std::vector< ESCI_NS::quad > >, gmm)
 (boost::optional< std::vector< ESCI_NS::quad > >, gmt)
 (boost::optional< std::vector< ESCI_NS::quad > >, cmx)
 (boost::optional< std::vector< ESCI_NS::quad > >, sfl)
 (boost::optional< std::vector< ESCI_NS::quad > >, mrr)
 (boost::optional< ESCI_NS::capabilities::constraint >, bsz)
 (boost::optional< ESCI_NS::capabilities::constraint >, pag)
 (boost::optional< ESCI_NS::capabilities::constraint >, rsm)
 (boost::optional< ESCI_NS::capabilities::constraint >, rss)
 (boost::optional< ESCI_NS::capabilities::constraint >, crp)
 (boost::optional< ESCI_NS::capabilities::focus_control>, fcs)
 (boost::optional< std::vector< ESCI_NS::quad > >, flc)
 (boost::optional< ESCI_NS::capabilities::constraint >, fla)
 (boost::optional< std::vector< ESCI_NS::quad > >, qit)
 (boost::optional< std::vector< ESCI_NS::quad > >, lam))

#undef ESCI_NS

#endif  /* drivers_esci_grammar_capabilities_ipp_ */
