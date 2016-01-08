//  run-time.cpp -- unit tests for the utsushi::run_time API
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

#include <boost/test/unit_test.hpp>

#include "utsushi/test/environment.hpp"
#include "../run-time.ipp"

namespace {

using utsushi::run_time;

struct fixture
{
  const char *program_name_;

  fixture ()
    : program_name_("run-time-unit-test-runner")
  {}

  ~fixture ()
  {
    delete run_time::impl::instance_;
    run_time::impl::instance_ = 0;
  }
};

struct program_fixture
  : fixture
{
};

BOOST_FIXTURE_TEST_SUITE (program_name, program_fixture)

BOOST_AUTO_TEST_CASE (unix_in_path)
{
  const char *argv[] = { PACKAGE_TARNAME };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (unix_abs_path)
{
  const char *argv[] = { "/bin/" PACKAGE_TARNAME };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (unix_rel_path)
{
  const char *argv[] = { "../" PACKAGE_TARNAME };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (windows_in_path)
{
  const char *argv[] = { PACKAGE_TARNAME ".exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (windows_abs_path)
{
  const char *argv[] = { "/bin/" PACKAGE_TARNAME ".exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (windows_drive_path)
{
  const char *argv[] = { "c:/bin/" PACKAGE_TARNAME ".exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (windows_rel_path)
{
  const char *argv[] = { "../" PACKAGE_TARNAME ".exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (unix_libtool_wrapper)
{
  const char *argv[] = { "/tmp/builddir/.libs/lt-" PACKAGE_TARNAME };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (windows_libtool_wrapper)
{
  const char *argv[] = { "/tmp/builddir/.libs/lt-" PACKAGE_TARNAME ".exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
}

BOOST_AUTO_TEST_CASE (util_unix_in_path)
{
  const char *argv[] = { "version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_unix_abs_path)
{
  const char *argv[] = { "/bin/version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_unix_rel_path)
{
  const char *argv[] = { "../version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_windows_in_path)
{
  const char *argv[] = { "version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_windows_abs_path)
{
  const char *argv[] = { "/bin/version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_windows_drive_path)
{
  const char *argv[] = { "c:/bin/version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_windows_rel_path)
{
  const char *argv[] = { "../version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_unix_libtool_wrapper)
{
  const char *argv[] = { "/tmp/builddir/.libs/lt-version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (util_windows_libtool_wrapper)
{
  const char *argv[] = { "/tmp/builddir/.libs/lt-version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_SUITE_END ()

//! Make it look as if the utilities have been installed
struct inst_util_fixture
  : fixture
{
  const char *srcdir_;

  inst_util_fixture ()
    : srcdir_(getenv ("srcdir"))
  {
    if (srcdir_) unsetenv ("srcdir");
  }

  ~inst_util_fixture ()
  {
    if (srcdir_) setenv ("srcdir", srcdir_, 0);
  }
};

BOOST_FIXTURE_TEST_SUITE (inst_util, inst_util_fixture)

BOOST_AUTO_TEST_CASE (inst_util_unix_in_path)
{
  const char *argv[] = { PACKAGE_TARNAME "-version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (inst_util_unix_abs_path)
{
  const char *argv[] = { "/bin/" PACKAGE_TARNAME "-version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (inst_util_unix_rel_path)
{
  const char *argv[] = { "../" PACKAGE_TARNAME "-version" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (inst_util_windows_in_path)
{
  const char *argv[] = { PACKAGE_TARNAME "-version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (inst_util_windows_abs_path)
{
  const char *argv[] = { "/bin/" PACKAGE_TARNAME "-version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (inst_util_windows_drive_path)
{
  const char *argv[] = { "c:/bin/" PACKAGE_TARNAME "-version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_CASE (inst_util_windows_rel_path)
{
  const char *argv[] = { "../" PACKAGE_TARNAME "-version.exe" };

  run_time rt (1, argv);

  BOOST_CHECK_EQUAL (PACKAGE_TARNAME, rt.program ());
  BOOST_CHECK_EQUAL ("version", rt.command ());
}

BOOST_AUTO_TEST_SUITE_END ()

struct options_fixture
  : fixture
{
};

BOOST_FIXTURE_TEST_SUITE (command_line_options, options_fixture)

BOOST_AUTO_TEST_CASE (non_std_option)
{
  const char *argv[] = {
    program_name_,
    "--non-std-option",
  };

  run_time rt (sizeof (argv) / sizeof (*argv), argv);

  BOOST_REQUIRE_EQUAL (0, rt.count ("non-std-option"));
  BOOST_CHECK_NE ("--non-std-option", rt.command ());
}

BOOST_AUTO_TEST_CASE (no_option_option_permutations)
{
  const char *argv[] = {
    program_name_,
    "--non-std-option",
    "--help",
  };

  run_time rt (sizeof (argv) / sizeof (*argv), argv);

  BOOST_CHECK_EQUAL (0, rt.count ("help"));
}

BOOST_AUTO_TEST_CASE (no_command_option_permutations)
{
  const char *argv[] = {
    program_name_,
    "version",
    "--help",
  };

  run_time rt (sizeof (argv) / sizeof (*argv), argv);

  BOOST_CHECK_EQUAL (0, rt.count ("help"));
}

BOOST_AUTO_TEST_SUITE_END ()

struct environment_fixture
  : fixture, utsushi::test::environment
{
};

BOOST_FIXTURE_TEST_SUITE (environment_variables, environment_fixture)

BOOST_AUTO_TEST_CASE (get_shell_variable)
{
  const char *shell ("/bin/false");
  const char *argv[] = {
    program_name_,
  };

  setenv (PACKAGE_ENV_VAR_PREFIX "SHELL", shell);

  run_time rt (sizeof (argv) / sizeof (*argv), argv);

  BOOST_CHECK_NE (0, rt.count ("SHELL"));
  BOOST_CHECK (!rt["SHELL"].defaulted ());
  BOOST_CHECK_EQUAL (shell, rt["SHELL"].as< std::string > ());
  BOOST_CHECK_EQUAL (shell, run_time::impl::instance_->shell_);
}

BOOST_AUTO_TEST_SUITE_END ()

} // namespace

#include "utsushi/test/runner.ipp"
