//  grammar-parameters.ipp -- component rule definitions
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

#ifndef drivers_esci_grammar_parameters_ipp_
#define drivers_esci_grammar_parameters_ipp_

//! \copydoc grammar.ipp

//  decoding::basic_grammar_parameters<T> implementation requirements
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_permutation.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/spirit/include/qi_skip.hpp>

//  encoding::basic_grammar_parameters<T> implementation requirements
#include <boost/spirit/include/karma_alternative.hpp>
#include <boost/spirit/include/karma_binary.hpp>
#include <boost/spirit/include/karma_buffer.hpp>
#include <boost/spirit/include/karma_char_.hpp>
#include <boost/spirit/include/karma_kleene.hpp>
#include <boost/spirit/include/karma_optional.hpp>
#include <boost/spirit/include/karma_plus.hpp>
#include <boost/spirit/include/karma_repeat.hpp>
#include <boost/spirit/include/karma_sequence.hpp>

//  *::basic_grammar_parameters<T> implementation requirements
#include <boost/fusion/include/adapt_struct.hpp>

//  Support for debugging of parser and generator rules
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>

#include "grammar-parameters.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace decoding {

template< typename Iterator >
basic_grammar_parameters< Iterator >::basic_grammar_parameters ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::parameter;

  parameters_rule_ %=
    (  (token_(ADF) > (qi::skip (token_(ADF)) [ *parm_adf_token_ ]))
     ^ (token_(TPU) > (qi::skip (token_(TPU)) [ *parm_tpu_token_ ]))
     ^ (token_(FB ) > (qi::skip (token_(FB )) [ *parm_fb_token_  ]))
     ^ (token_(COL) >  parm_col_token_)
     ^ (token_(FMT) >  parm_fmt_token_)
     ^ (token_(JPG) >  this->decimal_)
     ^ (token_(THR) >  this->decimal_)
     ^ (token_(DTH) >  parm_dth_token_)
     ^ (token_(GMM) >  parm_gmm_token_)
     ^ (token_(GMT) > +gamma_table_rule_)
     ^ (token_(CMX) >  color_matrix_rule_)
     ^ (token_(SFL) >  parm_sfl_token_)
     ^ (token_(MRR) >  parm_mrr_token_)
     ^ (token_(BSZ) >  this->positive_)
     ^ (token_(PAG) >  this->decimal_)
     ^ (token_(RSM) >  this->positive_)
     ^ (token_(RSS) >  this->positive_)
     ^ (token_(CRP) >  this->numeric_)
     ^ (token_(ACQ) >  qi::repeat (4) [ this->positive_ ])
     ^ (token_(FLC) >  parm_flc_token_)
     ^ (token_(FLA) >  qi::repeat (4) [ this->positive_ ])
     ^ (token_(QIT) >  parm_qit_token_)
     ^ (token_(LDF) >  this->positive_)
     ^ (token_(DFA) >  qi::repeat (2) [ this->positive_ ])
     ^ (token_(LAM) >  parm_lam_token_)
     )
    > qi::eoi
    ;

  gamma_table_rule_ %=
    qi::skip (token_(GMT)) [ parm_gmt_token_ > this->bin_hex_data_ ];

  color_matrix_rule_ %=
      (&token_(cmx::UNIT) > token_)
    | (parm_cmx_token_ > this->bin_hex_data_)
    ;

  parm_adf_token_ %=
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
      | token_(adf::CARD)
      )
    > token_
    ;

  parm_tpu_token_ %=
    &(  token_(tpu::ARE1)
      | token_(tpu::ARE2)
      | token_(tpu::NEGL)
      | token_(tpu::IR  )
      | token_(tpu::MAGC)
      | token_(tpu::FAST)
      | token_(tpu::SLOW)
      | token_(tpu::CRP )
      | token_(tpu::SKEW)
      | token_(tpu::OVSN)
      )
    > token_
    ;

  parm_fb_token_ %=
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

  parm_col_token_ %=
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

  parm_fmt_token_ %=
    &(  token_(fmt::RAW)
      | token_(fmt::JPG)
      )
    > token_
    ;

  parm_dth_token_ %=
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

  parm_gmm_token_ %=
    &(  token_(gmm::UG10)
      | token_(gmm::UG18)
      | token_(gmm::UG22)
      )
    > token_
    ;

  parm_gmt_token_ %=
    &(  token_(gmt::RED)
      | token_(gmt::GRN)
      | token_(gmt::BLU)
      | token_(gmt::MONO)
      )
    > token_
    ;

  parm_cmx_token_ %=
    &(  token_(cmx::UNIT)
      | token_(cmx::UM08)
      | token_(cmx::UM16)
      )
    > token_
    ;

  parm_sfl_token_ %=
    &(  token_(sfl::SMT2)
      | token_(sfl::SMT1)
      | token_(sfl::NORM)
      | token_(sfl::SHP1)
      | token_(sfl::SHP2)
      )
    > token_
    ;

  parm_mrr_token_ %=
    &(  token_(mrr::ON)
      | token_(mrr::OFF)
      )
    > token_
    ;

  parm_flc_token_ %=
    &(  token_(flc::WH)
      | token_(flc::BK)
      )
    > token_
    ;

  parm_qit_token_ %=
    &(  token_(qit::PREF)
      | token_(qit::ON  )
      | token_(qit::OFF )
      )
    > token_
    ;

  parm_lam_token_ %=
    &(  token_(lam::ON  )
      | token_(lam::OFF )
      )
    > token_
    ;

  ESCI_GRAMMAR_TRACE_NODE (parameters_rule_);

  ESCI_GRAMMAR_TRACE_NODE (gamma_table_rule_);
  ESCI_GRAMMAR_TRACE_NODE (color_matrix_rule_);

  ESCI_GRAMMAR_TRACE_NODE (parm_adf_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_tpu_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_fb_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_col_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_fmt_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_dth_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_gmm_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_gmt_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_cmx_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_sfl_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_mrr_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_flc_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_qit_token_);
  ESCI_GRAMMAR_TRACE_NODE (parm_lam_token_);
}

}       // namespace decoding

namespace encoding {

template< typename Iterator >
basic_grammar_parameters< Iterator >::basic_grammar_parameters ()
  : basic_grammar_formats< Iterator > ()
{
  using namespace code_token::parameter;

  parameters_rule_ %=
       -(token_(ADF) << *parm_adf_token_)
    << -(token_(TPU) << *parm_tpu_token_)
    << -(token_(FB ) << *parm_fb_token_)
    << - karma::buffer [ token_(COL) << parm_col_token_ ]
    << - karma::buffer [ token_(FMT) << parm_fmt_token_ ]
    << - karma::buffer [ token_(JPG) << this->decimal_ ]
    << - karma::buffer [ token_(THR) << this->decimal_ ]
    << - karma::buffer [ token_(DTH) << parm_dth_token_ ]
    << - karma::buffer [ token_(GMM) << parm_gmm_token_ ]
    << - karma::buffer [ +(token_(GMT) <<
                           parm_gmt_token_ << this->bin_hex_data_) ]
    << - karma::buffer [ token_(CMX) <<
                         ( (parm_cmx_token_ << this->bin_hex_data_)
                           | token_(cmx::UNIT)) ]
    << - karma::buffer [ token_(SFL) << parm_sfl_token_ ]
    << - karma::buffer [ token_(MRR) << parm_mrr_token_ ]
    << - karma::buffer [ token_(BSZ) << this->positive_ ]
    << - karma::buffer [ token_(PAG) << this->decimal_ ]
    << - karma::buffer [ token_(RSM) << this->positive_ ]
    << - karma::buffer [ token_(RSS) << this->positive_ ]
    << - karma::buffer [ token_(CRP) << this->numeric_ ]
    << - karma::buffer [ token_(ACQ)
                         << karma::repeat (4) [ this->positive_ ] ]
    << - karma::buffer [ token_(FLC) << parm_flc_token_ ]
    << - karma::buffer [ token_(FLA)
                         << karma::repeat (4) [ this->positive_ ] ]
    << - karma::buffer [ token_(QIT) << parm_qit_token_ ]
    << - karma::buffer [ token_(LDF) << this->positive_ ]
    << - karma::buffer [ token_(DFA)
                         << karma::repeat (2) [ this->positive_ ] ]
    << - karma::buffer [ token_(LAM) << parm_lam_token_ ]
    ;

  parameter_subset_rule_ %=
    *parameter_token_       //! \bug silently skips non-parameter codes
    ;

#define SYMBOL_ENTRY(token) (token, token_(token))

  parameter_token_.add
    SYMBOL_ENTRY (ADF)
    SYMBOL_ENTRY (TPU)
    SYMBOL_ENTRY (FB )
    SYMBOL_ENTRY (COL)
    SYMBOL_ENTRY (FMT)
    SYMBOL_ENTRY (JPG)
    SYMBOL_ENTRY (THR)
    SYMBOL_ENTRY (DTH)
    SYMBOL_ENTRY (GMM)
    SYMBOL_ENTRY (GMT)
    SYMBOL_ENTRY (CMX)
    SYMBOL_ENTRY (SFL)
    SYMBOL_ENTRY (MRR)
    SYMBOL_ENTRY (BSZ)
    SYMBOL_ENTRY (PAG)
    SYMBOL_ENTRY (RSM)
    SYMBOL_ENTRY (RSS)
    SYMBOL_ENTRY (CRP)
    SYMBOL_ENTRY (ACQ)
    SYMBOL_ENTRY (FLC)
    SYMBOL_ENTRY (FLA)
    SYMBOL_ENTRY (QIT)
    SYMBOL_ENTRY (LDF)
    SYMBOL_ENTRY (DFA)
    SYMBOL_ENTRY (LAM)
    ;

  parm_adf_token_.add
    SYMBOL_ENTRY (adf::DPLX)
    SYMBOL_ENTRY (adf::PEDT)
    SYMBOL_ENTRY (adf::DFL0)
    SYMBOL_ENTRY (adf::DFL1)
    SYMBOL_ENTRY (adf::DFL2)
    SYMBOL_ENTRY (adf::LDF )
    SYMBOL_ENTRY (adf::SDF )
    SYMBOL_ENTRY (adf::SPP )
    SYMBOL_ENTRY (adf::FAST)
    SYMBOL_ENTRY (adf::SLOW)
    SYMBOL_ENTRY (adf::BGWH)
    SYMBOL_ENTRY (adf::BGBK)
    SYMBOL_ENTRY (adf::BGGY)
    SYMBOL_ENTRY (adf::LOAD)
    SYMBOL_ENTRY (adf::EJCT)
    SYMBOL_ENTRY (adf::CRP )
    SYMBOL_ENTRY (adf::SKEW)
    SYMBOL_ENTRY (adf::OVSN)
    SYMBOL_ENTRY (adf::CARD)
    ;

  parm_tpu_token_.add
    SYMBOL_ENTRY (tpu::ARE1)
    SYMBOL_ENTRY (tpu::ARE2)
    SYMBOL_ENTRY (tpu::NEGL)
    SYMBOL_ENTRY (tpu::IR  )
    SYMBOL_ENTRY (tpu::MAGC)
    SYMBOL_ENTRY (tpu::FAST)
    SYMBOL_ENTRY (tpu::SLOW)
    SYMBOL_ENTRY (tpu::CRP )
    SYMBOL_ENTRY (tpu::SKEW)
    SYMBOL_ENTRY (tpu::OVSN)
    ;

  parm_fb_token_.add
    SYMBOL_ENTRY (fb::LMP1)
    SYMBOL_ENTRY (fb::LMP2)
    SYMBOL_ENTRY (fb::FAST)
    SYMBOL_ENTRY (fb::SLOW)
    SYMBOL_ENTRY (fb::CRP )
    SYMBOL_ENTRY (fb::SKEW)
    SYMBOL_ENTRY (fb::OVSN)
    ;

  parm_col_token_.add
    SYMBOL_ENTRY (col::C003)
    SYMBOL_ENTRY (col::C024)
    SYMBOL_ENTRY (col::C048)
    SYMBOL_ENTRY (col::M001)
    SYMBOL_ENTRY (col::M008)
    SYMBOL_ENTRY (col::M016)
    SYMBOL_ENTRY (col::R001)
    SYMBOL_ENTRY (col::R008)
    SYMBOL_ENTRY (col::R016)
    SYMBOL_ENTRY (col::G001)
    SYMBOL_ENTRY (col::G008)
    SYMBOL_ENTRY (col::G016)
    SYMBOL_ENTRY (col::B001)
    SYMBOL_ENTRY (col::B008)
    SYMBOL_ENTRY (col::B016)
    ;

  parm_fmt_token_.add
    SYMBOL_ENTRY (fmt::RAW)
    SYMBOL_ENTRY (fmt::JPG)
    ;

  parm_dth_token_.add
    SYMBOL_ENTRY (dth::NONE)
    SYMBOL_ENTRY (dth::MIDA)
    SYMBOL_ENTRY (dth::MIDB)
    SYMBOL_ENTRY (dth::MIDC)
    SYMBOL_ENTRY (dth::DTHA)
    SYMBOL_ENTRY (dth::DTHB)
    SYMBOL_ENTRY (dth::DTHC)
    SYMBOL_ENTRY (dth::DTHD)
    ;

  parm_gmm_token_.add
    SYMBOL_ENTRY (gmm::UG10)
    SYMBOL_ENTRY (gmm::UG18)
    SYMBOL_ENTRY (gmm::UG22)
    ;

  parm_gmt_token_.add
    SYMBOL_ENTRY (gmt::RED)
    SYMBOL_ENTRY (gmt::GRN)
    SYMBOL_ENTRY (gmt::BLU)
    SYMBOL_ENTRY (gmt::MONO)
    ;

  parm_cmx_token_.add
  //SYMBOL_ENTRY (cmx::UNIT)
    SYMBOL_ENTRY (cmx::UM08)
    SYMBOL_ENTRY (cmx::UM16)
    ;

  parm_sfl_token_.add
    SYMBOL_ENTRY (sfl::SMT2)
    SYMBOL_ENTRY (sfl::SMT1)
    SYMBOL_ENTRY (sfl::NORM)
    SYMBOL_ENTRY (sfl::SHP1)
    SYMBOL_ENTRY (sfl::SHP2)
    ;

  parm_mrr_token_.add
    SYMBOL_ENTRY (mrr::ON)
    SYMBOL_ENTRY (mrr::OFF)
    ;

  parm_flc_token_.add
    SYMBOL_ENTRY (flc::WH)
    SYMBOL_ENTRY (flc::BK)
    ;

  parm_qit_token_.add
    SYMBOL_ENTRY (qit::PREF)
    SYMBOL_ENTRY (qit::ON  )
    SYMBOL_ENTRY (qit::OFF )
    ;

  parm_lam_token_.add
    SYMBOL_ENTRY (lam::ON  )
    SYMBOL_ENTRY (lam::OFF )
    ;

#undef SYMBOL_ENTRY

  ESCI_GRAMMAR_TRACE_NODE (parameters_rule_);
  ESCI_GRAMMAR_TRACE_NODE (parameter_subset_rule_);
}

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#define ESCI_NS utsushi::_drv_::esci

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::parameters::gamma_table,
 (ESCI_NS::quad, component)
 (std::vector< ESCI_NS::byte >, table))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::parameters::color_matrix,
 (ESCI_NS::quad, type)
 (boost::optional< std::vector< ESCI_NS::byte > >, matrix))

BOOST_FUSION_ADAPT_STRUCT
(ESCI_NS::parameters,
 (boost::optional< std::vector< ESCI_NS::quad > >, adf)
 (boost::optional< std::vector< ESCI_NS::quad > >, tpu)
 (boost::optional< std::vector< ESCI_NS::quad > >, fb)
 (boost::optional< ESCI_NS::quad >, col)
 (boost::optional< ESCI_NS::quad >, fmt)
 (boost::optional< ESCI_NS::integer >, jpg)
 (boost::optional< ESCI_NS::integer >, thr)
 (boost::optional< ESCI_NS::quad >, dth)
 (boost::optional< ESCI_NS::quad >, gmm)
 (boost::optional< std::vector< ESCI_NS::parameters::gamma_table > >, gmt)
 (boost::optional< ESCI_NS::parameters::color_matrix >, cmx)
 (boost::optional< ESCI_NS::quad >, sfl)
 (boost::optional< ESCI_NS::quad >, mrr)
 (boost::optional< ESCI_NS::integer >, bsz)
 (boost::optional< ESCI_NS::integer >, pag)
 (boost::optional< ESCI_NS::integer >, rsm)
 (boost::optional< ESCI_NS::integer >, rss)
 (boost::optional< ESCI_NS::integer >, crp)
 (boost::optional< std::vector< ESCI_NS::integer > >, acq)
 (boost::optional< ESCI_NS::quad >, flc)
 (boost::optional< std::vector< ESCI_NS::integer > >, fla)
 (boost::optional< ESCI_NS::quad >, qit)
 (boost::optional< ESCI_NS::integer >, ldf)
 (boost::optional< std::vector< ESCI_NS::integer > >, dfa)
 (boost::optional< ESCI_NS::quad >, lam)
)

#undef ESCI_NS

#endif  /* drivers_esci_grammar_parameters_ipp_ */
