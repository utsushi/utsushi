//  tools.hpp -- for unit test implementation
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

#ifndef utsushi_test_tools_hpp_
#define utsushi_test_tools_hpp_

#include <functional>
#include <iterator>
#include <locale>
#include <string>

#include <boost/test/framework.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

//! Consistency alternative for the official Boost.Test tool name
/*! All of the Boost.Test tools for binary comparison use a two letter
 *  abbreviation for the corresponding comparison operator.  Equality
 *  comparison is the only exception.  This define rectifies that.
 */
#ifndef BOOST_WARN_EQ
#define BOOST_WARN_EQ BOOST_WARN_EQUAL
#endif
//! \copydoc BOOST_WARN_EQ
#ifndef BOOST_CHECK_EQ
#define BOOST_CHECK_EQ BOOST_CHECK_EQUAL
#endif
//! \copydoc BOOST_WARN_EQ
#ifndef BOOST_REQUIRE_EQ
#define BOOST_REQUIRE_EQ BOOST_REQUIRE_EQUAL
#endif

//! Readability macro for change_test_case_name() and friends
#define is_invalid_identifier_char                      \
  std::bind2nd                                          \
  (std::not2 (std::ptr_fun (std::isalnum< char >)),     \
   std::locale::classic ())                             \
    /**/

namespace utsushi {
namespace test {

//! Modify the Boost.Test provided test case name to \a s
/*! When using parameterized test cases, the Boost.Test provided test
 *  case name is not exactly helpful.  All test cases get the name of
 *  the test function so it is often next to impossible to tell which
 *  test case triggered a failure.  This little hack changes the name
 *  of the test case so they are easily told apart.
 *
 *  \note The Boost.Test framework's message for entering a test case
 *        still uses the \e unmodified test case name.  At \c message
 *        level verbosity the act of renaming is logged for clarity.
 *  \note As renaming can only take place after a test case has been
 *        entered the renamed name cannot be used when selecting the
 *        tests to be run.
 */
inline
void
change_test_case_name (const std::string& s)
{
  namespace ut = boost::unit_test;

  if (s.empty ()) return;

  const char underscore ('_');

  ut::test_case& tc =
    const_cast< ut::test_case& > (ut::framework::current_test_case ());

  std::string name;
  std::replace_copy_if (s.begin (), s.end (), std::back_inserter (name),
                        is_invalid_identifier_char, underscore);

  tc.p_name.set (name);

  BOOST_TEST_MESSAGE ("Renaming test case to \"" << name << "\"");
}

//! Prefix \a s to the Boost.Test provided test case name
/*! \copydetails change_test_case_name
 */
inline
void
prefix_test_case_name (const std::string& s)
{
  namespace ut = boost::unit_test;

  if (s.empty ()) return;

  const char underscore ('_');

  ut::test_case& tc =
    const_cast< ut::test_case& > (ut::framework::current_test_case ());

  std::string name;
  std::replace_copy_if (s.begin (), s.end (), std::back_inserter (name),
                        is_invalid_identifier_char, underscore);
  name += underscore;
  name += tc.p_name.get ();

  tc.p_name.set (name);

  BOOST_TEST_MESSAGE ("Renaming test case to \"" << name << "\"");
}

//! Append \a s to the Boost.Test provided test case name
/*! \copydetails change_test_case_name
 */
inline
void
suffix_test_case_name (const std::string& s)
{
  namespace ut = boost::unit_test;

  if (s.empty ()) return;

  const char underscore ('_');

  ut::test_case& tc =
    const_cast< ut::test_case& > (ut::framework::current_test_case ());

  std::string name (tc.p_name.get ());

  name += underscore;
  std::replace_copy_if (s.begin (), s.end (), std::back_inserter (name),
                        is_invalid_identifier_char, underscore);

  tc.p_name.set (name);

  BOOST_TEST_MESSAGE ("Renaming test case to \"" << name << "\"");
}

} // namespace test
} // namespace utsushi

#undef is_invalid_identifier_char

#endif /* utsushi_test_tools_hpp_ */
