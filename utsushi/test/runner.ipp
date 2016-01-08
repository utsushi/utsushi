//  test/runner.ipp -- for utsushi test programs
//  Copyright (C) 2012  EPSON AVASYS CORPORATION
//
//  License: GPL-3.0+
//  Author : Olaf Meeuwissen
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

#ifndef utsushi_test_runner_ipp_
#define utsushi_test_runner_ipp_

/*! \file
 *  Boost.Test provides a mechanism to have a test runner's \c ::main
 *  function generated automatically.  @PACKAGE_NAME@'s test harness,
 *  however, needs to do a few things Boost.Test's \c ::main doesn't
 *  provide for so, this file implements a custom version.
 *
 *  Include this file at the \e end of each test runner implementation
 *  and define the right mix of preprocessor macros on the compiler's
 *  command-line when doing so.  The most convenient way to do that is
 *  by including \c boost-test.am in the \c Makefile.am that compiles
 *  the test runner.
 */

#include <boost/test/unit_test.hpp>

#ifdef ENABLE_TEST_REPORTS

#include <boost/test/results_reporter.hpp>
#include <fstream>

//! Set up reporting infra-structure
/*! This opens a file named after the test suite (via a preprocessor
 *  macro passed on the compilation command-line).  When running the
 *  test runner it is assumed that XML output was requested, either
 *  through the \c BOOST_TEST_REPORT_FORMAT environment variable or
 *  the corresponding command-line option.
 *
 *  The report will be saved as \c UTSUSHI_TEST_SUITE \c -report.xml
 *  irrespective of the report format requested.
 */
struct report_redirector
{
  std::ofstream report;

  report_redirector ()
    : report (UTSUSHI_TEST_SUITE "-report.xml")
  {
    assert (report.is_open ());
    boost::unit_test::results_reporter::set_stream (report);
  }
};

static report_redirector fixture;

#endif  /* ENABLE_TEST_REPORTS */

// This is defined in <boost/test/parameterized_test.hpp>.  When that
// file is included before this one, users need to implement their own
// init_test_runner().

#ifndef BOOST_PARAM_TEST_CASE
bool
init_test_runner ()
{
  return true;
}
#endif  /* BOOST_PARAM_TEST_CASE */

//! Run a test runner
/*! This \c ::main "template" deals with module name initialization
 *  and calls an init_test_runner().  When parameterized test suites
 *  are used init_test_runner() needs to be provided by the user but
 *  a no-op stub will be provided otherwise.
 */
int
main (int argc, char *argv[])
{
  namespace but = boost::unit_test;

  std::string test_module = UTSUSHI_TEST_MODULE;
  test_module += "::";
  test_module += UTSUSHI_TEST_SUITE;

  but::framework::master_test_suite ().p_name.value = test_module;

  return but::unit_test_main (init_test_runner, argc, argv);
}

#endif /* utsushi_test_runner_ipp_ */
