//  grammar-capabilities.hpp -- component rule declarations
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

#ifndef drivers_esci_grammar_capabilities_hpp_
#define drivers_esci_grammar_capabilities_hpp_

//! \copydoc grammar.hpp

#include <limits>
#include <vector>

#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/variant.hpp>

#include <utsushi/constraint.hpp>
#include <utsushi/store.hpp>

#include "code-token.hpp"
#include "grammar-formats.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

struct capabilities
  : private boost::equality_comparable< capabilities >
{
  bool operator== (const capabilities& rhs) const;

  void clear ();

  operator bool () const;

  bool has_duplex () const;
  bool has_media_end_detection () const;

  bool can_calibrate () const;
  bool can_clean () const;
  bool can_eject () const;
  bool can_load () const;

  bool can_crop (const quad& src) const;

  utsushi::constraint::ptr border_fill () const;
  utsushi::constraint::ptr border_size (const quantity& default_value = quantity ()) const;
  utsushi::constraint::ptr buffer_size (const boost::optional< integer >& default_value) const;
  utsushi::constraint::ptr crop_adjustment () const;
  utsushi::constraint::ptr document_sources (const quad& default_value) const;
  utsushi::constraint::ptr double_feed () const;
  bool has_double_feed_off_command () const;
  utsushi::constraint::ptr dropouts () const;
  quad get_dropout (const quad& gray, const string& color) const;
  bool has_dropout (const quad& gray) const;
  utsushi::constraint::ptr formats (const boost::optional< quad >& default_value) const;
  utsushi::constraint::ptr gamma (const boost::optional< quad >& default_value) const;
  utsushi::constraint::ptr image_count (const boost::optional< integer >& default_value) const;
  utsushi::constraint::ptr image_types (const boost::optional< quad >& default_value) const;
  utsushi::constraint::ptr jpeg_quality (const boost::optional< integer >& default_value) const;
  utsushi::constraint::ptr resolutions (const quad& direction,
                                        const boost::optional< integer >& default_value,
                                        const integer& max = std::numeric_limits< integer >::max ()) const;
  utsushi::constraint::ptr threshold (const boost::optional< integer >& default_value) const;

  struct range
    : private boost::equality_comparable< range >
  {
    range (const integer& lower = integer (),
           const integer& upper = esci_int_max);

    bool operator== (const range& rhs) const;

    integer lower_;
    integer upper_;
  };

  typedef boost::variant< range, std::vector< integer > > constraint;

  struct document_source
    : private boost::equality_comparable< document_source >
  {
    bool operator== (const document_source& rhs) const;

    boost::optional< std::vector< quad > > flags;
    boost::optional< constraint > resolution;
  };

  struct tpu_source
    : document_source
    , private boost::equality_comparable< tpu_source >
  {
    bool operator== (const tpu_source& rhs) const;

    boost::optional< std::vector< quad > > area;
    boost::optional< std::vector< quad > > alternative_area;
  };

  struct focus_control
    : private boost::equality_comparable< focus_control >
  {
    focus_control ();

    bool operator== (const focus_control& rhs) const;

    bool automatic;
    boost::optional< constraint > position;
  };

  boost::optional< document_source > adf;
  boost::optional< tpu_source > tpu;
  boost::optional< document_source > fb;
  boost::optional< std::vector< quad > > col;
  boost::optional< std::vector< quad > > fmt;
  boost::optional< range > jpg;
  boost::optional< range > thr;
  boost::optional< std::vector< quad > > dth;
  boost::optional< std::vector< quad > > gmm;
  boost::optional< std::vector< quad > > gmt;
  boost::optional< std::vector< quad > > cmx;
  boost::optional< std::vector< quad > > sfl;
  boost::optional< std::vector< quad > > mrr;
  boost::optional< constraint > bsz; // private protocol extension
  boost::optional< constraint > pag; // private protocol extension
  boost::optional< constraint > rsm;
  boost::optional< constraint > rss;
  boost::optional< constraint > crp;
  boost::optional< focus_control > fcs;
  boost::optional< std::vector< quad > > flc;
  boost::optional< constraint > fla;
  boost::optional< std::vector< quad > > qit;
  boost::optional< std::vector< quad > > lam;
};

#ifdef ESCI_GRAMMAR_TRACE

template< typename T >
std::basic_ostream< T >&
operator<< (std::basic_ostream< T >& os, const capabilities::range& r)
{
  return os << "[" << r.lower_ << "," << r.upper_ << "]";
}

#endif  /* defined (ESCI_GRAMMAR_TRACE) */

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
class basic_grammar_capabilities
  : virtual protected basic_grammar_formats< Iterator >
{
public:
  basic_grammar_capabilities ();

  //! Deciphers a reply payload from a capability request
  /*! \sa code_token::capability
   */
  bool
  capabilities_(Iterator& head, const Iterator& tail,
                capabilities& caps)
  {
    return this->parse (head, tail, capability_rule_, caps);
  }

protected:
  qi::rule< Iterator, capabilities () > capability_rule_;

  qi::rule< Iterator, capabilities::document_source () > caps_adf_rule_;
  qi::rule< Iterator, capabilities::tpu_source () > caps_tpu_rule_;
  qi::rule< Iterator, capabilities::document_source () > caps_fb_rule_;
  qi::rule< Iterator, capabilities::focus_control () > caps_fcs_rule_;

  //! Codes a list of numbers
  /*! \todo Decide whether we need "pickier" alternatives that only
   *        accept a certain kind of numbers, e.g. decimal_.
   */
  qi::rule< Iterator, std::vector< integer > () > numeric_list_;
  qi::rule< Iterator, std::vector< integer > () > decimal_list_;
  qi::rule< Iterator, std::vector< integer > () > positive_list_;

  //! Codes a pair of lower and upper limits
  /*! \todo Decide whether we need "pickier" alternatives that only
   *        accept a certain kind of numbers, e.g. decimal_.
   */
  qi::rule< Iterator, capabilities::range () > range_;
  qi::rule< Iterator, capabilities::range () > decimal_range_;
  qi::rule< Iterator, capabilities::range () > positive_range_;

  qi::rule< Iterator, quad () > caps_adf_token_;
  qi::rule< Iterator, quad () > caps_tpu_token_;
  qi::rule< Iterator, quad () > caps_tpu_area_token_;
  qi::rule< Iterator, quad () > caps_fb_token_;
  qi::rule< Iterator, quad () > caps_col_token_;
  qi::rule< Iterator, quad () > caps_fmt_token_;
  qi::rule< Iterator, quad () > caps_dth_token_;
  qi::rule< Iterator, quad () > caps_gmm_token_;
  qi::rule< Iterator, quad () > caps_gmt_token_;
  qi::rule< Iterator, quad () > caps_cmx_token_;
  qi::rule< Iterator, quad () > caps_sfl_token_;
  qi::rule< Iterator, quad () > caps_mrr_token_;
  qi::rule< Iterator, quad () > caps_flc_token_;
  qi::rule< Iterator, quad () > caps_qit_token_;
  qi::rule< Iterator, quad () > caps_lam_token_;
};

extern template class basic_grammar_capabilities< default_iterator_type >;

}       // namespace decoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_capabilities_hpp_ */
