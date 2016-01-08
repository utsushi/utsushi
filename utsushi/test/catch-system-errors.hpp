//  catch-system-errors.hpp -- global fixtures for unit testing
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

#ifndef utsushi_test_catch_system_errors_hpp_
#define utsushi_test_catch_system_errors_hpp_

namespace utsushi {
namespace test {

//! Control asynchronous system event handling for a test runner
/*! Boost.Test runners have a toggle that controls their behaviour in
 *  the face of asynchronous system events.  The toggle can be set via
 *  the \c --catch-system-errors command-line option as well as the \c
 *  BOOST_TEST_CATCH_SYSTEM_ERRORS environment variable.  Sufficient
 *  when executing test runners individually, this approach does not
 *  lend itself to the \c check target provided by \c automake.  That
 *  target runs all test programs in the \e same environment so one
 *  can no longer control the toggle on a per test runner basis.
 *
 *  This fixture addresses this issue by providing the individual test
 *  runners with a means to set \c BOOST_TEST_CATCH_SYSTEM_ERRORS to a
 *  desired value.  All a test runner has to do is add the appropriate
 *  subclass to \c BOOST_GLOBAL_FIXTURE.
 *
 *  \note This fixture only works at global scope.  Boost.Test does
 *        not reset environment information between test suites and
 *        test cases.
 */
class catch_system_errors
{
public:
  //! Set the appropriate environment variable to \a yes (or no)
  catch_system_errors (bool yes)
    : env_var_("BOOST_TEST_CATCH_SYSTEM_ERRORS")
  {
    const char *ev (getenv (env_var_.c_str ()));

    if (ev) old_val_ = ev;

    setenv (env_var_.c_str (), yes ? "yes" : "no", true);
  }

  //! Reset the environment variable to its original value
  ~catch_system_errors ()
  {
    if (!old_val_.empty ())
      {
        setenv (env_var_.c_str (), old_val_.c_str (), true);
      }
  }

protected:
  std::string env_var_;
  std::string old_val_;
};

//! Ignore asynchronous system events
class catch_system_errors_no
  : public catch_system_errors
{
public:
  catch_system_errors_no ()
    : catch_system_errors (false)
  {}
};

//! Process asynchronous system events
class catch_system_errors_yes
  : public catch_system_errors
{
public:
  catch_system_errors_yes ()
    : catch_system_errors (true)
  {}
};

} // namespace test
} // namespace utsushi

#endif /* utsushi_test_catch_system_errors_hpp_ */
