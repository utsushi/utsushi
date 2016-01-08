//  main.cpp -- tests of the entry point to the software
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
#include <unistd.h>

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
using utsushi::test::suffix_test_case_name;

typedef std::istream::traits_type traits;
typedef std::istream_iterator< string > stream_iterator;

struct command_line
  : utsushi::test::command_line
{
  command_line (const string& executable)
    : utsushi::test::command_line (executable)
  {}

  command_line (const string& executable, const string& argument)
    : utsushi::test::command_line (executable, argument)
  {}

  int execute ()
  {
    BOOST_TEST_CHECKPOINT (command_);
    return utsushi::test::command_line::execute ();
  }
};

struct utsushi_main
  : command_line
{
  utsushi_main (const string& command, const string& argument)
    : command_line ((fs::path ("..") / "main").string ())
  {
    fs::path p (ut::framework::master_test_suite ().argv[0]);
    string ext = p.extension ().string ();

    if (".utr" != ext) command_ += ext;

    *this += command;
    *this += argument;
  }
};

static
void
test_command (const string& command, const fs::path& p)
{
  const string arg = p.stem ().string ();

  command_line expect (p.string (), "--" + command);
  utsushi_main result (command, arg);

  BOOST_CHECK_EQUAL (EXIT_SUCCESS, expect.execute ());
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

static
void
test_help_command (const fs::path& p)
{
  suffix_test_case_name ("help " + p.stem ().string ());

  BOOST_TEST_MESSAGE ("test: ../main help cmd == ../cmd --help");
  test_command ("help", p);
}

static
void
test_version_command (const fs::path& p)
{
  suffix_test_case_name ("version " + p.stem ().string ());

  BOOST_TEST_MESSAGE ("test: ../main version cmd == ../cmd --version");
  test_command ("version", p);
}

static
const string invocation_command ("help");

static
void
test_command_invocation (utsushi_main& expect, command_line& result)
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

static
void
test_current_directory_invocation (const fs::path& p)
{
  suffix_test_case_name (invocation_command + " " + p.stem ().string ());

  BOOST_REQUIRE_NE (fs::path ("."), p.parent_path ());

  BOOST_TEST_MESSAGE ("test: ../main help cmd == cd .. && ./main help cmd");

  fs::path cmd (".");
  cmd /= "main";
  cmd.replace_extension (p.extension ());
  string arg = p.stem ().string ();

  utsushi_main expect (invocation_command, arg);
  command_line result ("cd " + p.parent_path ().string ()
                       + " && " + cmd.string (), invocation_command);
  result += arg;

  test_command_invocation (expect, result);
}

static
void
test_absolute_path_invocation (const fs::path& p)
{
  suffix_test_case_name (invocation_command + " " + p.stem ().string ());

  BOOST_REQUIRE (getenv ("PWD"));

  BOOST_TEST_MESSAGE ("test: ../main help cmd == `pwd`/../main help cmd");

  fs::path cmd (getenv ("PWD"));
  cmd /= "..";
  cmd /= "main";
  cmd.replace_extension (p.extension ());
  string arg = p.stem ().string ();

  utsushi_main expect (invocation_command, arg);
  command_line result (cmd.string (), invocation_command);

  result += arg;

  test_command_invocation (expect, result);
}

static
void
test_in_system_path_invocation (const fs::path& p)
{
  suffix_test_case_name (invocation_command + " " + p.stem ().string ());

  BOOST_TEST_MESSAGE ("test: ../main help cmd == PATH=..:$PATH main help cmd");

  string arg = p.stem ().string ();

  utsushi_main expect (invocation_command, arg);
  command_line result ("PATH=" + p.parent_path ().string () + ":$PATH "
                       + "main" + p.extension ().string (),
                       invocation_command);
  result += arg;

  test_command_invocation (expect, result);
}

static
bool
is_not_executable_file (const fs::directory_entry& d)
{
  return !(fs::is_regular_file (d)
           && 0 == access (d.path ().c_str (), X_OK));
}

static
bool
init_test_runner ()
{
  using namespace std;

  fs::path p ("..");
  vector< fs::path > command;

  remove_copy_if (fs::directory_iterator (p), fs::directory_iterator (),
                  back_inserter (command), is_not_executable_file);

  sort (command.begin (), command.end ());

  {
    ut::test_suite *ts = BOOST_TEST_SUITE ("commands");

    ts->add (BOOST_PARAM_TEST_CASE (test_help_command,
                                    command.begin (), command.end ()));
    ts->add (BOOST_PARAM_TEST_CASE (test_version_command,
                                    command.begin (), command.end ()));

    ut::framework::master_test_suite ().add (ts);
  }

  {
    ut::test_suite *ts = BOOST_TEST_SUITE ("invocation");

    ts->add (BOOST_PARAM_TEST_CASE (test_current_directory_invocation,
                                    command.begin (), command.end ()));
    ts->add (BOOST_PARAM_TEST_CASE (test_absolute_path_invocation,
                                    command.begin (), command.end ()));
    ts->add (BOOST_PARAM_TEST_CASE (test_in_system_path_invocation,
                                    command.begin (), command.end ()));

    ut::framework::master_test_suite ().add (ts);
  }

  return true;
}

} // namespace

#include "utsushi/test/runner.ipp"
