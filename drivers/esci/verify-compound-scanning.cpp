//  verify-compound-scanning.cpp -- scenario tests
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*! \file
 *  This file implements a (partial) test suite that attempts to carry
 *  out a number of rather simple scans that one would normally expect
 *  to succeed.  Scan scenarios take device capabilities into account
 *  so that we don't try to do things like requesting TPU scans on a
 *  flatbed only device.
 *
 *  Note that these tests are \e not meant to test our code.  They are
 *  meant to test the \e firmware that is installed on the device.
 *
 *  \todo  Figure out how to deal with ADF media reloading.  Right now
 *         you just have to make sure you reload with the right timing
 *         and the correct number of sheets.
 *
 *  \sa verify-compound-protocol.cpp
 */

#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include <utsushi/connexion.hpp>

#include "code-token.hpp"
#include "scanner-control.hpp"
#include "verify.hpp"

namespace esci = utsushi::_drv_::esci;
using utsushi::connexion;

using namespace esci::code_token;
namespace err = reply::info::err;

//! Fixture that combines a connexion with a scanner_control command
/*! For testing purposes, we would like to have convenient access to a
 *  connexion to the device.  This fixture gives each command instance
 *  its own connexion.
 */
struct controller
  : esci::scanner_control
{
  connexion::ptr cnx;

  controller ()
    : esci::scanner_control (false)
    , cnx (verify::cnx)
  {}

  ~controller ()
  {
    *cnx << finish ();
    cnx_ = 0;
  }
};

// Put all the protocol tests in a single test suite.  This allows us
// to remove the whole test suite in case no devices are detected.

BOOST_AUTO_TEST_SUITE (scanning);

BOOST_FIXTURE_TEST_CASE (flatbed_scan, controller)
{
  *cnx << get_information ();

  BOOST_WARN (info_.flatbed);
  if (info_.flatbed) {
    *cnx << get_parameters ();
    resa_.adf = boost::none;
    resa_.tpu = boost::none;
    resa_.fb  = std::vector< esci::quad > ();
    *cnx << set (resa_);
    *cnx << start ();
    while (++*this);
    BOOST_CHECK (status_.pen);
  }
}

BOOST_FIXTURE_TEST_CASE (tpu_scan, controller)
{
  *cnx << get_information ();

  BOOST_WARN (info_.tpu);
  if (info_.tpu) {
    *cnx << get_parameters ();
    resa_.adf = boost::none;
    resa_.tpu = std::vector< esci::quad > ();
    resa_.fb  = boost::none;
    *cnx << set (resa_);
    *cnx << start ();
    while (++*this);
    BOOST_CHECK (status_.pen);
  }
}

BOOST_FIXTURE_TEST_CASE (adf_simplex_scan, controller)
{
  sleep (1);

  using namespace reply::info::err;
  *cnx << get_information ();

  BOOST_WARN (info_.adf);
  if (info_.adf) {
    *cnx << get_parameters ();
    resa_.adf = std::vector< esci::quad > ();
    resa_.tpu = boost::none;
    resa_.fb  = boost::none;
    *cnx << set (resa_);
    *cnx << start ();
    while (++*this);
    BOOST_CHECK (status_.pen);
    ++*this;
    BOOST_CHECK (media_out (ADF));
  }
}

BOOST_FIXTURE_TEST_CASE (adf_duplex_scan, controller)
{
  sleep (1);

  using namespace reply::info::err;
  namespace adf = parameter::adf;

  *cnx << get_information ();

  BOOST_WARN (info_.adf && info_.adf->duplex_passes);
  if (info_.adf && info_.adf->duplex_passes) {
    *cnx << get_parameters ();
    resa_.adf = std::vector< esci::quad > ();
    resa_.tpu = boost::none;
    resa_.fb  = boost::none;
    resa_.adf->push_back (adf::DPLX);
    *cnx << set (resa_);
    *cnx << start ();
    while (++*this);
    BOOST_CHECK (status_.pen);
    while (++*this);
    BOOST_CHECK (status_.pen);
    ++*this;
    BOOST_CHECK (media_out (ADF));
  }
}

// \todo Test does not seem to account for arbitrary ordering of IMGA
//       and IMGB replies
BOOST_FIXTURE_TEST_CASE (adf_duplex_jpeg_scan, controller)
{
  sleep (1);

  using namespace reply::info::err;
  namespace adf = parameter::adf;
  namespace col = parameter::col;
  namespace fmt = parameter::fmt;

  *cnx << get_information ();

  BOOST_WARN (info_.adf && info_.adf->duplex_passes);
  if (info_.adf && info_.adf->duplex_passes) {
    *cnx << get_parameters ();
    resa_.adf = std::vector< esci::quad > ();
    resa_.tpu = boost::none;
    resa_.fb  = boost::none;
    resa_.adf->push_back (adf::DPLX);
    resa_.col = col::C024;
    resa_.fmt = fmt::JPG;
    *cnx << set (resa_);
    *cnx << start ();
    while (++*this);
    BOOST_CHECK (status_.pen);
    while (++*this);
    BOOST_CHECK (status_.pen);
    ++*this;
    BOOST_CHECK (media_out (ADF));
  }
}

BOOST_AUTO_TEST_SUITE_END ();   // scanning
