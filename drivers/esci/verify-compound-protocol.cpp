//  verify-compound-protocol.cpp -- compliance tests
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*! \file
 *  This file implements a (partial) test suite to check hardware for
 *  protocol compliance.  The tests are based on our understanding and
 *  assumptions regarding the protocol.
 *
 *  Note that these tests are \e not meant to test our code.  They are
 *  meant to test the \e firmware that is installed on the device.
 *
 *  \sa verify-compound-scanning.cpp
 */

#include <algorithm>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <utsushi/connexion.hpp>

#include "code-token.hpp"
#include "scanner-control.hpp"
#include "scanner-inquiry.hpp"
#include "verify.hpp"

namespace esci = utsushi::_drv_::esci;
using utsushi::connexion;

using namespace esci::code_token;
namespace par = reply::info::par;

//! Fixture that extends a compound command \c T
/*! For testing purposes, we would like to have convenient access to a
 *  connexion to the device.  This fixture gives each command instance
 *  its own connexion and allows us instrument operator>>() with a few
 *  additional compliance checks.
 *
 *  Templating over the compound commands makes for convenient testing
 *  of all of them without a lot of code duplication.
 */
template< typename T >
struct test_compound
  : public T
{
  BOOST_STATIC_ASSERT ((boost::is_base_of< esci::compound_base, T >::value));

  connexion::ptr cnx;

  using T::status_;

  using T::info_;
  using T::capa_;
  using T::capb_;
  using T::resa_;
  using T::resb_;
  using T::stat_;

  //! Creates a connexion and initializes the command session
  test_compound ()
    : T (false)
    , cnx (verify::cnx)
  {
    *cnx << *this;
  }

  //! Terminates the command session
  ~test_compound ()
  {
    *cnx << this->finish ();
  }

  //! Runs a command on the other end of a connexion.
  /*! Override to add byte alignment checking of all requests as well
   *  as all but the \c IMG replies.  Parsing expectation failures are
   *  intercepted and flagged as errors.  So are device side errors.
   */
  void
  operator>> (connexion& cnx)
  {
    try
      {
        BOOST_CHECK_MESSAGE (0 == this->request_.size % 4,
                             esci::str (this->request_.code)
                             << " request fails 4-byte alignment");

        T::operator>> (cnx);
      }
    catch (typename T::expectation_failure& e)
      {
        esci::byte_buffer& buf (this->dat_ref_.get ());

        BOOST_ERROR ("\n  " << e.what () << " @ "
                     << std::hex << e.first - buf.begin ()
                     << "\n  Expecting " << e.what_
                     << "\n  Got: " << std::string (e.first, e.last));
      }

    if (reply::IMG != this->reply_.code)
      BOOST_CHECK_MESSAGE (0 == this->reply_.size % 4,
                           esci::str (this->reply_.code)
                           << " reply fails 4-byte alignment");

    BOOST_REQUIRE (this->status_.err.empty ());
  }
};

// Readability type definitions
typedef test_compound< esci::scanner_control > test_control;
typedef test_compound< esci::scanner_inquiry > test_inquiry;

// Type list of compound commands to instantiate test cases for
typedef boost::mpl::list< test_control,
                          test_inquiry
                          > compound_types;

// For lack of operator<< (std::ostream&, T) implementations
BOOST_TEST_DONT_PRINT_LOG_VALUE (utsushi::_drv_::esci::status);
BOOST_TEST_DONT_PRINT_LOG_VALUE (utsushi::_drv_::esci::information);
BOOST_TEST_DONT_PRINT_LOG_VALUE (utsushi::_drv_::esci::capabilities);
BOOST_TEST_DONT_PRINT_LOG_VALUE (utsushi::_drv_::esci::parameters);

// Put all the protocol tests in a single test suite.  This allows us
// to remove the whole test suite in case no devices are detected.

BOOST_AUTO_TEST_SUITE (protocol);

BOOST_FIXTURE_TEST_CASE_TEMPLATE (finish_request, T, compound_types, T)
{
  *this->cnx << this->finish ();
}

BOOST_AUTO_TEST_SUITE (getters);

//! Helper function to check capability related assumptions
/*! We assume that at least one document source is available and that
 *  a select few capabilities are present.  Without these capabilities
 *  it would be rather difficult to implement a meaningful driver.
 */
static void
check_caps (const esci::capabilities& caps)
{
  BOOST_CHECK (caps.adf || caps.tpu || caps.fb);
  BOOST_CHECK (caps.col && !caps.col->empty ());
  BOOST_CHECK (caps.fmt && !caps.fmt->empty ());
  BOOST_CHECK (caps.rsm);
  BOOST_CHECK (caps.rss);
}

//! Helper function to check whether a \e single document source is set
static void
check_doc_src (const esci::parameters& parm)
{
  BOOST_CHECK (parm.adf || parm.tpu || parm.fb);
  BOOST_CHECK (!(parm.adf && parm.tpu));
  BOOST_CHECK (!(parm.tpu && parm.fb ));
  BOOST_CHECK (!(parm.fb  && parm.adf));
}

//! Helper function to check parameter related assumptions
/*! We assume that exactly one document source is selected at any time
 *  and that we can rely on the presence of a select few parameters.
 *  Without the parameters we require it would be very difficult to do
 *  anything meaningful in a driver implementation.
 */
static void
check_parm (const esci::parameters& parm, const esci::status stat)
{
  BOOST_REQUIRE (!stat.par || par::OK == *stat.par);
  check_doc_src (parm);
  BOOST_CHECK (parm.col);
  BOOST_CHECK (parm.fmt);
  BOOST_CHECK (parm.rsm);
  BOOST_CHECK (parm.rss);
  BOOST_CHECK (parm.acq && 4 == parm.acq->size ());
}

//! Verify assumptions regarding device information
/*! We assume that at least one document source is available and that
 *  a select few pieces of information are present.  Without those it
 *  would become quite difficult to implement a meaningful driver.
 *  Additionally, any guarantees made by the generic command protocol
 *  specification are checked as well.
 */
BOOST_FIXTURE_TEST_CASE_TEMPLATE (information, T, compound_types, T)
{
  *this->cnx << this->get (this->info_);

  BOOST_CHECK (this->info_.adf || this->info_.tpu || this->info_.flatbed);
  BOOST_CHECK (2 == this->info_.max_image.size ());
  BOOST_CHECK (!this->info_.product.empty ());
  BOOST_CHECK (!this->info_.version.empty ());
  BOOST_CHECK (1536 <= this->info_.device_buffer_size);
}

//! Verify assumptions regarding device capabilities
BOOST_FIXTURE_TEST_CASE_TEMPLATE (capabilities, T, compound_types, T)
{
  *this->cnx << this->get (this->capa_);

  check_caps (this->capa_);
}

//! Verify assumptions regarding device flip-side capabilities, if any
BOOST_FIXTURE_TEST_CASE_TEMPLATE (capabilities_flip, T, compound_types, T)
{
  *this->cnx << this->get (this->capb_, true);

  if (verify::caps_flip)
    {
      check_caps (this->capb_);
    }
  else
    {
      BOOST_CHECK_EQUAL (0, this->reply_.size);
    }
}

//! Verify assumptions regarding scan settings
BOOST_FIXTURE_TEST_CASE_TEMPLATE (parameters, T, compound_types, T)
{
  *this->cnx << this->get (this->resa_);

  BOOST_CHECK (!this->status_.par || par::OK == *this->status_.par);

  check_parm (this->resa_, this->status_);
}

//! Verify assumptions regarding flip-side scan settings, if any
BOOST_FIXTURE_TEST_CASE_TEMPLATE (parameters_flip, T, compound_types, T)
{
  *this->cnx << this->get (this->resb_, true);

  BOOST_CHECK (!this->status_.par || par::OK == *this->status_.par);

  if (verify::parm_flip)
    {
      check_parm (this->resb_, this->status_);
    }
  else
    {
      BOOST_CHECK_EQUAL (0, this->reply_.size);
    }
}

//! Verify assumptions regarding device status
/*! There is not really anything that we can check for because we can
 *  not expect there to be any status in need of reporting.
 */
BOOST_FIXTURE_TEST_CASE_TEMPLATE (status, T, compound_types, T)
{
  *this->cnx << this->get (this->stat_);
}

//! Retrieve document source settings only
/*! Exactly one document source should be selected and no other scan
 *  settings should be returned.
 */
BOOST_FIXTURE_TEST_CASE_TEMPLATE (doc_src, T, compound_types, T)
{
  using namespace esci::code_token::parameter;

  std::set< esci::quad > ts;
  ts.insert (ADF);
  ts.insert (TPU);
  ts.insert (FB);

  *this->cnx << this->get_parameters (ts);

  check_doc_src (this->resa_);

  esci::parameters parm;
  parm.adf = this->resa_.adf;
  parm.tpu = this->resa_.tpu;
  parm.fb  = this->resa_.fb;

  BOOST_CHECK_EQUAL (parm, this->resa_);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE (color_mode, T, compound_types, T)
{
  using namespace esci::code_token::parameter;

  std::set< esci::quad > ts;
  ts.insert (COL);

  *this->cnx << this->get_parameters (ts);

  BOOST_REQUIRE (!this->status_.par || par::OK == *this->status_.par);

  BOOST_CHECK ( this->resa_.col);

  esci::parameters parm;
  parm.col = this->resa_.col;

  BOOST_CHECK_EQUAL (parm, this->resa_);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE (format, T, compound_types, T)
{
  using namespace esci::code_token::parameter;

  std::set< esci::quad > ts;
  ts.insert (FMT);

  *this->cnx << this->get_parameters (ts);

  BOOST_REQUIRE (!this->status_.par || par::OK == *this->status_.par);

  BOOST_CHECK ( this->resa_.fmt);

  esci::parameters parm;
  parm.fmt = this->resa_.fmt;

  BOOST_CHECK_EQUAL (parm, this->resa_);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE (resolution, T, compound_types, T)
{
  using namespace esci::code_token::parameter;

  std::set< esci::quad > ts;
  ts.insert (RSM);
  ts.insert (RSS);

  *this->cnx << this->get_parameters (ts);

  BOOST_REQUIRE (!this->status_.par || par::OK == *this->status_.par);

  BOOST_CHECK ( this->resa_.rsm);
  BOOST_CHECK ( this->resa_.rss);

  esci::parameters parm;
  parm.rsm = this->resa_.rsm;
  parm.rss = this->resa_.rss;

  BOOST_CHECK_EQUAL (parm, this->resa_);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE (scan_area, T, compound_types, T)
{
  using namespace esci::code_token::parameter;

  std::set< esci::quad > ts;
  ts.insert (ACQ);

  *this->cnx << this->get_parameters (ts);

  BOOST_REQUIRE (!this->status_.par || par::OK == *this->status_.par);

  BOOST_CHECK ( this->resa_.acq && 4 == this->resa_.acq->size ());

  esci::parameters parm;
  parm.acq = this->resa_.acq;

  BOOST_CHECK_EQUAL (parm, this->resa_);
}

//! Retrieve gamma information only
/*! We do not assume that gamma information is available.  However, if
 *  gamma tables are returned, we check that there is at least one and
 *  that all of them contain exactly 256 elements.
 *
 *  We also check for the absence of those settings that we normally
 *  assume present.
 */
BOOST_FIXTURE_TEST_CASE_TEMPLATE (gamma_info, T, compound_types, T)
{
  using namespace esci::code_token::parameter;

  std::set< esci::quad > ts;
  ts.insert (GMM);
  ts.insert (GMT);

  *this->cnx << this->get_parameters (ts);

  BOOST_CHECK (!this->status_.par || par::OK == *this->status_.par);

  if (this->resa_.gmt)
    {
      BOOST_CHECK (!this->resa_.gmt->empty ());

      std::vector< esci::parameters::gamma_table >::const_iterator it;
      for (it = this->resa_.gmt->begin ();
           this->resa_.gmt->end () != it;
           ++it)
        {
          BOOST_CHECK_MESSAGE (256 == it->table.size (),
                               esci::str (it->component)
                               << ": 256 != " << it->table.size ());
        }
    }

  esci::parameters parm;
  parm.gmm = this->resa_.gmm;
  parm.gmt = this->resa_.gmt;

  BOOST_CHECK_EQUAL (parm, this->resa_);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE (request_loop, T, compound_types, T)
{
  const int loop_count (100);
  int par_count (0);

  for (int i = 0; i < loop_count; ++i)
    {
      *this->cnx << this->get_information ();
      *this->cnx << this->get_capabilities ();
      *this->cnx << this->get_parameters ();
      if (this->status_.par) ++par_count;
      BOOST_CHECK (!this->status_.par || par::OK == *this->status_.par);
      *this->cnx << this->get_status ();
    }
  BOOST_CHECK (!par_count || loop_count == par_count);
}

//! Verify that default settings are identical for both sides
/*! If the device returns flip-side settings, check that these are the
 *  same as the settings that would apply to both sides.
 */
BOOST_FIXTURE_TEST_CASE_TEMPLATE (same_parameters, T, compound_types, T)
{
  *this->cnx << this->get_parameters (true);

  if (0 < this->reply_.size)
    {
      BOOST_REQUIRE (!this->status_.par || par::OK == *this->status_.par);

      *this->cnx << this->get_parameters ();

      BOOST_REQUIRE (!this->status_.par || par::OK == *this->status_.par);

      BOOST_CHECK_EQUAL (this->resa_, this->resb_);
    }
}

//! Verify that both compound commands return the same information
BOOST_AUTO_TEST_CASE (same_reply_information)
{
  esci::information tc_info  , ti_info;
  esci::status      tc_status, ti_status;
  {
    test_control tc;
    *tc.cnx << tc.get (tc_info);
    tc_status = tc.status_;
  }
  {
    test_inquiry ti;
    *ti.cnx << ti.get (ti_info);
    ti_status = ti.status_;
  }
  BOOST_CHECK_EQUAL (tc_info  , ti_info);
  BOOST_CHECK_EQUAL (tc_status, ti_status);
}

//! Verify that both compound commands return the same capabilities
BOOST_AUTO_TEST_CASE (same_reply_capabilities)
{
  esci::capabilities tc_caps  , ti_caps;
  esci::status       tc_status, ti_status;
  {
    test_control tc;
    *tc.cnx << tc.get (tc_caps);
    tc_status = tc.status_;
  }
  {
    test_inquiry ti;
    *ti.cnx << ti.get (ti_caps);
    ti_status = ti.status_;
  }
  BOOST_CHECK_EQUAL (tc_caps  , ti_caps);
  BOOST_CHECK_EQUAL (tc_status, ti_status);
}

//! Verify that both compound commands return the same capabilities
/*! This test queries for flip-side capabilities.  Independent of
 *  whether these are supported, the replies to the requests ought
 *  to be identical.
 */
BOOST_AUTO_TEST_CASE (same_reply_capabilities_flip)
{
  esci::capabilities tc_caps  , ti_caps;
  esci::status       tc_status, ti_status;
  {
    test_control tc;
    *tc.cnx << tc.get (tc_caps, true);
    tc_status = tc.status_;
  }
  {
    test_inquiry ti;
    *ti.cnx << ti.get (ti_caps, true);
    ti_status = ti.status_;
  }
  BOOST_CHECK_EQUAL (tc_caps  , ti_caps);
  BOOST_CHECK_EQUAL (tc_status, ti_status);
}

//! Verify that both compound commands return the same scan settings
BOOST_AUTO_TEST_CASE (same_reply_parameters)
{
  esci::parameters tc_parm  , ti_parm;
  esci::status     tc_status, ti_status;
  {
    test_control tc;
    *tc.cnx << tc.get (tc_parm);
    tc_status = tc.status_;
  }
  {
    test_inquiry ti;
    *ti.cnx << ti.get (ti_parm);
    ti_status = ti.status_;
  }
  BOOST_CHECK_EQUAL (tc_parm  , ti_parm);
  BOOST_CHECK_EQUAL (tc_status, ti_status);
}

//! Verify that both compound commands return the same scan settings
/*! This test queries for flip-side scan settings.  Independent of
 *  whether these are supported, the replies to the requests ought
 *  to be identical.
 */
BOOST_AUTO_TEST_CASE (same_reply_parameters_flip)
{
  esci::parameters tc_parm  , ti_parm;
  esci::status     tc_status, ti_status;
  {
    test_control tc;
    *tc.cnx << tc.get (tc_parm, true);
    tc_status = tc.status_;
  }
  {
    test_inquiry ti;
    *ti.cnx << ti.get (ti_parm, true);
    ti_status = ti.status_;
  }
  BOOST_CHECK_EQUAL (tc_parm  , ti_parm);
  BOOST_CHECK_EQUAL (tc_status, ti_status);
}

BOOST_AUTO_TEST_SUITE_END ();   // "getters" test suite

BOOST_AUTO_TEST_SUITE (setters);

//! Reset the parameters to those just obtained
/*! This test implementation initializes the parameter block with the
 *  content of the data block, bypassing all decoding and encoding.
 */
BOOST_FIXTURE_TEST_CASE (self_consistency_direct, test_control)
{
  *cnx << get_parameters ();

  BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

  par_blk_ = dat_blk_;
  encode_request_block_(request::PARA, par_blk_.size ());

  *cnx << *this;

  BOOST_CHECK (status_.par);
  BOOST_CHECK (status_.par && par::OK == *status_.par);
}

//! Reset the flip-side parameters to those just obtained, if any
/*! This test implementation initializes the parameter block with the
 *  content of the data block, bypassing all decoding and encoding.
 */
BOOST_FIXTURE_TEST_CASE (self_consistency_direct_flip, test_control)
{
  *cnx << get_parameters (true);

  if (0 < reply_.size)
    {
      BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

      par_blk_ = dat_blk_;
      encode_request_block_(request::PARB, par_blk_.size ());

      *cnx << *this;

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);
    }
}

//! Reset the parameters to those just obtained
/*! This test implementation decodes the data block and re-encodes the
 *  result to initialize the parameter block.
 */
BOOST_FIXTURE_TEST_CASE (self_consistency_grammar, test_control)
{
  esci::parameters parm;

  *cnx << get (parm);

  BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

  *cnx << set (parm);

  BOOST_CHECK (status_.par);
  BOOST_CHECK (status_.par && par::OK == *status_.par);
}

//! Reset the flip-side parameters to those just obtained, if any
/*! This test implementation decodes the data block and re-encodes the
 *  result to initialize the parameter block.
 */
BOOST_FIXTURE_TEST_CASE (self_consistency_grammar_flip, test_control)
{
  esci::parameters parm;

  *cnx << get (parm, true);

  if (0 < reply_.size)
    {
      BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

      *cnx << set (parm, true);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);
    }
}

//! Check the default ADF scan settings
/*! Note, this is only tested if the device supports an ADF.
 */
BOOST_FIXTURE_TEST_CASE (default_adf, test_control)
{
  *cnx << get (info_);

  if (info_.adf)
    {
      esci::parameters parm;
      parm.adf = std::vector< esci::quad > ();
      parm.tpu = boost::none;
      parm.fb  = boost::none;

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);
      BOOST_REQUIRE (resa_.adf);
      BOOST_CHECK_EQUAL_COLLECTIONS (parm.adf->begin (), parm.adf->end (),
                                     resa_.adf->begin (), resa_.adf->end ());
    }
}

//! Check the default TPU scan settings
/*! Note, this is only tested if the device supports a TPU.
 */
BOOST_FIXTURE_TEST_CASE (default_tpu, test_control)
{
  *cnx << get (info_);

  if (info_.tpu)
    {
      esci::parameters parm;
      parm.adf = boost::none;
      parm.tpu = std::vector< esci::quad > ();
      parm.fb  = boost::none;

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);
      BOOST_REQUIRE (resa_.tpu);
      BOOST_CHECK_EQUAL_COLLECTIONS (parm.tpu->begin (), parm.tpu->end (),
                                     resa_.tpu->begin (), resa_.tpu->end ());
    }
}

//! Check the default flatbed scan settings
/*! Note, this is only tested if the device supports a flatbed.
 */
BOOST_FIXTURE_TEST_CASE (default_flatbed, test_control)
{
  *cnx << get (info_);

  if (info_.flatbed)
    {
      esci::parameters parm;
      parm.adf = boost::none;
      parm.tpu = boost::none;
      parm.fb  = std::vector< esci::quad > ();

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);
      BOOST_REQUIRE (resa_.fb);
      BOOST_CHECK_EQUAL_COLLECTIONS (parm.fb->begin (), parm.fb->end (),
                                     resa_.fb->begin (), resa_.fb->end ());
    }
}

BOOST_FIXTURE_TEST_CASE (color_mode, test_control)
{
  *cnx << get (capa_);

  std::vector< esci::quad >::const_iterator it;

  for (it = capa_.col->begin (); capa_.col->end () != it; ++it)
    {
      esci::parameters parm;
      parm.col = *it;

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);
      BOOST_REQUIRE (resa_.col);

      BOOST_CHECK_MESSAGE (*it == *resa_.col,
                           "check *it == *resa_.col failed "
                           << "[ " << esci::str (*it)
                           << " != "
                           << esci::str (*resa_.col) << " ]");
    }
}

//! Attempt to set a non-supported or invalid color mode
/*! The implementation bypasses the regular encoding machinery for the
 *  parameter block so that we can easily set a totally whacked value.
 */
BOOST_FIXTURE_TEST_CASE (color_mode_invalid, test_control)
{
  *cnx << get (capa_);
  *cnx << get (resa_);

  BOOST_REQUIRE (!status_.par || par::OK == *status_.par);
  BOOST_REQUIRE (resa_.col);
  BOOST_REQUIRE (capa_.col && 1 < capa_.col->size ());
  BOOST_REQUIRE (!std::count (capa_.col->begin (), capa_.col->end (),
                              reply::info::END));

  par_blk_ = "#COL----";
  encode_request_block_(request::PARA, par_blk_.size ());

  *this >> *cnx;

  BOOST_CHECK (status_.par);
  BOOST_CHECK (status_.par && par::FAIL == *status_.par);

  esci::parameters parm;
  *cnx << get (parm);

  BOOST_CHECK (!status_.par || par::OK == *status_.par);
  BOOST_CHECK (parm.col);
  BOOST_CHECK (parm.col && *parm.col == *resa_.col);
  BOOST_CHECK_EQUAL (parm, resa_);
}

BOOST_FIXTURE_TEST_CASE (get_set_get_default, test_control)
{
  esci::parameters parm;

  *cnx << get (parm);

  BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

  *cnx << set (parm);

  BOOST_CHECK (status_.par);
  BOOST_CHECK (status_.par && par::OK == *status_.par);

  *cnx << get (resa_);

  BOOST_CHECK (!status_.par || par::OK == *status_.par);

  BOOST_CHECK_EQUAL (parm, resa_);
}

BOOST_FIXTURE_TEST_CASE (get_set_get_flatbed, test_control)
{
  *cnx << get (info_);

  if (info_.flatbed)
    {
      esci::parameters parm;
      parm.adf = boost::none;
      parm.tpu = boost::none;
      parm.fb  = std::vector< esci::quad > ();

      *cnx << set (parm);

      BOOST_REQUIRE (status_.par);
      BOOST_REQUIRE (status_.par && par::OK == *status_.par);

      *cnx << get (parm);

      BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);

      BOOST_CHECK_EQUAL (parm, resa_);
    }
}

BOOST_FIXTURE_TEST_CASE (get_set_get_adf, test_control)
{
  *cnx << get (info_);

  if (info_.adf)
    {
      esci::parameters parm;
      parm.adf = std::vector< esci::quad > ();
      parm.tpu = boost::none;
      parm.fb  = boost::none;

      *cnx << set (parm);

      BOOST_REQUIRE (status_.par);
      BOOST_REQUIRE (status_.par && par::OK == *status_.par);

      *cnx << get (parm);

      BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);

      BOOST_CHECK_EQUAL (parm, resa_);
    }
}

BOOST_FIXTURE_TEST_CASE (get_set_get_tpu, test_control)
{
  *cnx << get (info_);

  if (info_.tpu)
    {
      esci::parameters parm;
      parm.adf = boost::none;
      parm.tpu = std::vector< esci::quad > ();
      parm.fb  = boost::none;

      *cnx << set (parm);

      BOOST_REQUIRE (status_.par);
      BOOST_REQUIRE (status_.par && par::OK == *status_.par);

      *cnx << get (parm);

      BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

      *cnx << set (parm);

      BOOST_CHECK (status_.par);
      BOOST_CHECK (status_.par && par::OK == *status_.par);

      *cnx << get (resa_);

      BOOST_CHECK (!status_.par || par::OK == *status_.par);

      BOOST_CHECK_EQUAL (parm, resa_);
    }
}

BOOST_FIXTURE_TEST_CASE (jpeg_quality_override, test_control)
{
  using namespace esci::code_token::capability;

  *cnx << get (capb_, true);
  if (0 < reply_.size)
    {
      *cnx << get (capb_, true);
      *cnx << get (resb_, true);

      BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

      BOOST_REQUIRE (resb_.jpg);

      if (capb_.fmt)
        {
          std::vector< esci::quad >::const_iterator it;
          for (it = capb_.fmt->begin (); capb_.fmt->end () != it; ++it)
            {
              if (fmt::JPG == *it) continue;

              esci::parameters parm;
              parm.jpg = resb_.jpg;
              parm.fmt = *it;

              *cnx << set (parm, true);

              BOOST_REQUIRE (status_.par);
              BOOST_REQUIRE (status_.par && par::OK == *status_.par);

              *cnx << get (resb_, true);

              BOOST_CHECK (!status_.par || par::OK == *status_.par);

              BOOST_CHECK (resb_.fmt);
              BOOST_CHECK_MESSAGE (resb_.fmt && resb_.fmt == parm.fmt,
                                   "check resb_.fmt == parm.fmt failed "
                                   << "[ " << esci::str (*resb_.fmt)
                                   << " != "
                                   << esci::str (*parm.fmt) << " ]");
            }
        }
    }

  *cnx << get (capa_);
  *cnx << get (resa_);

  BOOST_REQUIRE (!status_.par || par::OK == *status_.par);

  BOOST_REQUIRE (resa_.jpg);

  if (capa_.fmt)
    {
      std::vector< esci::quad >::const_iterator it;
      for (it = capa_.fmt->begin (); capa_.fmt->end () != it; ++it)
        {
          if (fmt::JPG == *it) continue;

          esci::parameters parm;
          parm.col = col::C024;
          parm.jpg = resa_.jpg;
          parm.fmt = *it;

          *cnx << set (parm);

          BOOST_REQUIRE (status_.par);
          BOOST_REQUIRE (status_.par && par::OK == *status_.par);

          *cnx << get (resa_);

          BOOST_CHECK (!status_.par || par::OK == *status_.par);

          BOOST_CHECK (resa_.fmt);
          BOOST_CHECK_MESSAGE (resa_.fmt && resa_.fmt == parm.fmt,
                               "check resa_.fmt == parm.fmt failed "
                               << "[ " << esci::str (*resa_.fmt)
                               << " != "
                               << esci::str (*parm.fmt) << " ]");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END ();   // "setters" test suite

BOOST_AUTO_TEST_SUITE_END ();   // "protocol" test suite
