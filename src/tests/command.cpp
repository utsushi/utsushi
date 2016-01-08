//  command.cpp -- unit tests for free-standing command requirements
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#include <cstdlib>

#include <algorithm>
#include <iterator>
#include <string>

#include <boost/filesystem.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>

#include "utsushi/test/command-line.hpp"
#include "utsushi/test/tools.hpp"

namespace fs = boost::filesystem;
namespace ut = boost::unit_test;

namespace {

using std::string;
using utsushi::test::command_line;
using utsushi::test::suffix_test_case_name;

typedef std::istream::traits_type traits;
typedef std::istream_iterator< string > stream_iterator;

/*! Free-standing command implementations need to support the GNU
 *  standard options: \c --help and \c --version.  However, these
 *  options should \e not be documented in the output they create.
 *  This helper function flags the "forbidden" tokens.
 */
static
void
gnu_standard_option_is_not_documented (const string& token)
{
  BOOST_CHECK_NE ("--help", token);
  BOOST_CHECK_NE ("--version", token);
}

/*! Free-standing command implementations need to support the GNU
 *  standard options: \c --help and \c --version.  These options
 *  should produce output (and no errors) following well-defined
 *  patterns.  This function implements the checks for output and
 *  no errors as well as the common part of the output patterns.
 *
 *  Output should start with the package archive's base name and be
 *  followed by the command name.  The \c main "command" should not
 *  output a command name as this is the user-space program.
 */
static
void
test_gnu_standard_option (const fs::path& cmd, const string& option)
{
  command_line cli (cmd.string (), option);

  BOOST_CHECK_EQUAL (EXIT_SUCCESS, cli.execute ());

  BOOST_CHECK_MESSAGE (traits::eof () != cli.out ().peek (),
                       "non-empty stdout");
  BOOST_CHECK_MESSAGE (traits::eof () == cli.err ().peek (),
                       "empty stderr");

  stream_iterator token (cli.out ());

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, *token);

  ++token;
  if ("main" == cmd.stem ())
    BOOST_CHECK_NE (cmd.stem (), *token);
  else
    BOOST_CHECK_EQUAL (cmd.stem (), *token);

  std::for_each (token, stream_iterator (),
                 gnu_standard_option_is_not_documented);
}

static
void
test_help_option (const fs::path& cmd)
{
  suffix_test_case_name (cmd.stem ().string ());
  test_gnu_standard_option (cmd, "--help");
}

static
void
test_version_option (const fs::path& cmd)
{
  suffix_test_case_name (cmd.stem ().string ());
  test_gnu_standard_option (cmd, "--version");
}

//! Standard option used to test_command_invocation scenarios
static
const string invocation_option ("--help");

/*! Free-standing command implementations should produce the same
 *  results irrespective of how they are invoked.  That is, there
 *  should be no difference whether invoked using a relative path
 *  specification from the \c tests/ directory and any other kind
 *  of invocation.  This helper function checks for discrepancies
 *  between the baseline output we \a expect and the \a result of
 *  an alternate invocation.
 *
 *  These alternate invocation patterns aim to make sure that the
 *  command can be used both before and after installation and be
 *  runnable from any location in the developer's working copy of
 *  the source tree.
 *
 *  \note  POSIX compliant shells must invoke a shell's matching
 *         builtin for for commands without a slash.  Hence, if
 *         such a shell provides builtins that match any Utsushi
 *         command it is the latter that looses out if it were to
 *         rely on system PATH based command lookup.  This makes
 *         any system PATH based lookup tests pointless.
 *
 *         By the way, \c bash provides a \c help builtin.
 */
static
void
test_command_invocation (command_line& expect, command_line& result)
{
  BOOST_REQUIRE_EQUAL (EXIT_SUCCESS, expect.execute ());

  BOOST_CHECK_EQUAL (EXIT_SUCCESS, result.execute ());

  BOOST_CHECK_MESSAGE (traits::eof () != result.out ().peek (),
                       "non-empty stdout");
  BOOST_CHECK_MESSAGE (traits::eof () == result.err ().peek (),
                       "empty stderr");

  stream_iterator expect_token (expect.out ());
  stream_iterator result_token (result.out ());

  BOOST_CHECK_EQUAL_COLLECTIONS (expect_token, stream_iterator (),
                                 result_token, stream_iterator ());
}

//! Runs a command after switching to the directory it "lives" in
/*! Normally, all tests execute a command from a \c tests/ directory
 *  below the directory where the executable resides.  When hacking
 *  away at the code, it is common to run commands directly from the
 *  directory they were built in.
 */
static
void
test_current_directory_invocation (const fs::path& p)
{
  suffix_test_case_name (p.stem ().string ());

  BOOST_REQUIRE_NE (fs::path ("."), p.parent_path ());

  BOOST_TEST_MESSAGE ("test: ../cmd --help == cd .. && ./cmd --help");

  fs::path cmd (".");
  cmd /= p.filename ();

  command_line expect (p.string (), invocation_option);
  command_line result ("cd " + p.parent_path ().string ()
                       + " && " + cmd.string (),
                       invocation_option);

  test_command_invocation (expect, result);
}

//! Runs a command using an absolute path invocation
/*! Rather than rely on the relative path to a free-standing command,
 *  a \c main user-space program implementation may prefer to use an
 *  absolute path instead.
 */
static
void
test_absolute_path_invocation (const fs::path& p)
{
  suffix_test_case_name (p.stem ().string ());

  BOOST_REQUIRE (getenv ("PWD"));

  BOOST_TEST_MESSAGE ("test: ../cmd --help == `pwd`/../cmd --help");

  fs::path cmd (getenv ("PWD"));
  cmd /= p;

  command_line expect (p.string (), invocation_option);
  command_line result (cmd.string (), invocation_option);

  test_command_invocation (expect, result);
}

static
bool
is_not_executable_command (const fs::directory_entry& d)
{
  return !(fs::is_regular_file (d)
           && 0 == access (d.path ().c_str (), X_OK)
           // FIXME reinstate testing of `scan` command
           && std::string::npos == d.path ().stem ().string ().find ("scan"));
}

//! Register free-standing command implementations for all tests
/*! All free-standing command implementations need to pass the full
 *  set of test suites.  Command implementations are registered in a
 *  fixed order, irrespective of any command interdependencies.
 */
bool
init_test_runner ()
{
  using namespace std;

  fs::path p ("..");
  vector< fs::path > executable;

  remove_copy_if (fs::directory_iterator (p), fs::directory_iterator (),
                  back_inserter (executable), is_not_executable_command);

  sort (executable.begin (), executable.end ());

  vector< fs::path >::const_iterator it (executable.begin ());
  for (; executable.end () != it; ++it)
    BOOST_TEST_MESSAGE ("registering " << *it << " for testing");

  {
    ut::test_suite *ts = BOOST_TEST_SUITE ("options");

    ts->add (BOOST_PARAM_TEST_CASE (test_help_option,
                                    executable.begin (), executable.end ()));
    ts->add (BOOST_PARAM_TEST_CASE (test_version_option,
                                    executable.begin (), executable.end ()));

    ut::framework::master_test_suite ().add (ts);
  }

  {
    ut::test_suite *ts = BOOST_TEST_SUITE ("invocation");

    ts->add (BOOST_PARAM_TEST_CASE (test_current_directory_invocation,
                                    executable.begin (), executable.end ()));
    ts->add (BOOST_PARAM_TEST_CASE (test_absolute_path_invocation,
                                    executable.begin (), executable.end ()));

    ut::framework::master_test_suite ().add (ts);
  }

  return true;
}

} // namespace

#include "utsushi/test/runner.ipp"
