//  grammar-information.hpp -- component rule declarations
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

#ifndef drivers_esci_grammar_information_hpp_
#define drivers_esci_grammar_information_hpp_

//! \copydoc grammar.hpp

#include <string>
#include <vector>

#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/qi_rule.hpp>

#include "code-token.hpp"
#include "grammar-formats.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

struct information
  : private boost::equality_comparable< information >
{
  information ();

  bool operator== (const information& rhs) const;

  void clear ();

  //! A product name free of leading and trailing whitespace
  std::string product_name () const;

  bool is_double_pass_duplexer () const;

  bool supports_size_detection (const quad& src) const;

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

  struct source
    : private boost::equality_comparable< source >
  {
    source ();

    bool operator== (const source& rhs) const;

    virtual bool supports_size_detection () const;

    integer resolution;
    std::vector< integer > area;
    std::vector< integer > overscan;
  };

  struct tpu_source
    : source
    , private boost::equality_comparable< tpu_source >
  {
    bool operator== (const tpu_source& rhs) const;

    std::vector< integer > alternative_area;
  };

  struct fb_source
    : source
    , private boost::equality_comparable< fb_source >
  {
    fb_source ();

    bool operator== (const fb_source& rhs) const;

    virtual bool supports_size_detection () const;

    bool detects_width;
    bool detects_height;
    quad alignment;
  };

  struct adf_source
    : fb_source
    , private boost::equality_comparable< adf_source >
  {
    adf_source ();

    bool operator== (const adf_source& rhs) const;

    bool supports_long_paper_mode () const;

    quad type;
    boost::optional< quad > duplex_passes;
    quad doc_order;
    bool prefeeds;
    bool auto_scans;
    std::vector< integer > min_doc;
    std::vector< integer > max_doc;
    bool auto_recovers;
    bool detects_carrier_sheet;
    bool supports_plastic_card;
  };

  boost::optional< adf_source > adf;
  boost::optional< tpu_source > tpu;
  boost::optional< fb_source > flatbed;
  std::vector< integer > max_image;
  bool has_push_button;
  std::vector< byte > product;
  std::vector< byte > version;
  std::vector< byte > product_version;
  integer device_buffer_size;
  std::vector< quad > extension;
  bool truncates_at_media_end;
  boost::optional< std::vector< byte > > serial_number;
  bool supports_authentication;
  bool supports_reinitialization;
  bool supports_automatic_feed;
  boost::optional< integer > double_feed_detection_threshold;
  boost::optional< constraint > crop_resolution_constraint;
};

namespace decoding {

namespace qi = boost::spirit::qi;

template< typename Iterator >
class basic_grammar_information
  : virtual protected basic_grammar_formats< Iterator >
{
public:
  basic_grammar_information ();

  //! Decodes a reply payload for an information request
  /*! \sa code_token::information
   */
  bool
  information_(Iterator& head, const Iterator& tail,
               information& info)
  {
    return this->parse (head, tail, information_rule_, info);
  }

protected:
  qi::rule< Iterator, information () > information_rule_;

  qi::rule< Iterator, information::adf_source () > info_adf_rule_;
  qi::rule< Iterator, information::tpu_source () > info_tpu_rule_;
  qi::rule< Iterator, information::fb_source () > info_fb_rule_;

  //! Codes a width and height
  qi::rule< Iterator, std::vector< integer > () > extent_;

  qi::rule< Iterator, std::vector< integer > () > positive_list_;
  qi::rule< Iterator, information::range     () > positive_range_;

  qi::rule< Iterator, quad () > info_adf_type_token_;
  qi::rule< Iterator, quad () > info_adf_dplx_token_;
  qi::rule< Iterator, quad () > info_adf_ford_token_;
  qi::rule< Iterator, quad () > info_adf_algn_token_;
  qi::rule< Iterator, quad () > info_fb_algn_token_;
  qi::rule< Iterator, quad () > info_ext_token_;
  qi::rule< Iterator, quad () > info_job_token_;
};

extern template class basic_grammar_information< default_iterator_type >;

}       // namespace decoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_information_hpp_ */
