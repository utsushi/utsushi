//  environment.hpp -- variable control during test execution
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

#ifndef utsushi_test_environment_hpp_
#define utsushi_test_environment_hpp_

#include <cstdlib>

#include <algorithm>
#include <map>
#include <set>

#include <utsushi/functional.hpp>
#include <utsushi/regex.hpp>

#if __cplusplus >= 201103L && !WITH_INCLUDED_BOOST
#define NS(bind) std::bind
#define NSPH(_1) std::placeholders::_1
#else
#define NS(bind) bind
#define NSPH(_1) _1
#endif

extern "C" {
extern char **environ;
}

namespace utsushi {
namespace test {

//! Sanitize environment variables for testing purposes
/*! When running tests one needs to be able to control the state of
 *  their execution environment.  There is little point in testing
 *  those parts of the software that depend on the value of package
 *  specific environment variables if there is no way to define a
 *  clean slate.  This fixture does precisely that.  Moreover, the
 *  fixture even restores the environment's state to what it was at
 *  the point the fixture was instantiated.
 *
 *  The fixture's API purposely mimicks the POSIX C APIs to get and
 *  set environment variables but is defined in terms of standard \c
 *  string objects.  Naturally, traditional character array pointers
 *  can be passed as well.  This works at no extra (implementation)
 *  cost courtesy of implicit conversion via \c std::string.
 *
 *  \note Users of this class need to link with Boost.Regex unless
 *        compiling against C++11 or later.
 *
 *  \todo Make sure instances are destructed in the reverse order of
 *        their creation.  Out-of-order destruction gives rises to an
 *        unpredictable state (which is exactly what this class ought
 *        to prevent).
 */
class environment
{
public:
  //! Create a "clean" POSIX-like environment
  /*! All package specific environment variable will be removed and
   *  the \a locale set to \c POSIX (by default).
   */
  environment (const std::string locale = "POSIX")
  {
    clearenv_("(" PACKAGE_ENV_VAR_PREFIX "[^=]*)=.*");
    setlocale (locale);
  }

  //! Restore the original environment
  ~environment ()
  {
    std::for_each (vars_set_.begin (), vars_set_.end (),
                   NS(bind) (&environment::unsetenv_, this, NSPH(_1)));
    std::for_each (mod_vars_.begin (), mod_vars_.end (),
                   NS(bind) (&environment::setenv_, this, NSPH(_1)));
  }

  //! Get a pointer to the value of an %environment \a variable
  const char *
  getenv (const std::string& variable) const
  {
    return ::getenv (variable.c_str ());
  }

  //! Set an %environment \a variable to a \a value
  /*! This either introduces a new \a variable (if one did not exist
   *  before) or changes the \a value of an existing one.
   */
  int
  setenv (const std::string& variable, const std::string& value)
  {
    maybe_save_current_(variable);
    vars_set_.insert (variable);

    return ::setenv (variable.c_str (), value.c_str (), 1);
  }

  //! Remove a \a variable from the %environment
  int
  unsetenv (const std::string& variable)
  {
    maybe_save_current_(variable);

    return ::unsetenv (variable.c_str ());
  }

  //! Control the %environment's \a locale
  /*! This clears the \c LC_* environment variables and sets \c LANG
   *  to the requested \a locale.  The \c LANGUAGE variable is unset.
   */
  void
  setlocale (const std::string& locale)
  {
    clearenv_("(LC_[^=]*)=.*");
    setenv ("LANG", locale);
    unsetenv ("LANGUAGE");
  }

protected:
  typedef std::map< std::string, std::string > env_var_map;
  typedef std::set< std::string > env_var_set;

  void
  clearenv_(const std::string& regular_expression)
  {
    regex re (regular_expression);
    cmatch var;

    char **p = environ;
    while (p && *p)
      {
        if (regex_match (*p, var, re))
          {
            unsetenv (var[1]);
          }
        ++p;
      }
  }

  void
  maybe_save_current_(const std::string& variable)
  {
    const char *env_var (getenv (variable));

    if (env_var && !mod_vars_.count (variable))
      mod_vars_[variable] = env_var;
  }

  void
  setenv_(const env_var_map::value_type& v) const
  {
    ::setenv (v.first.c_str (), v.second.c_str (), 1);
  }

  void
  unsetenv_(const env_var_set::value_type& v) const
  {
    ::unsetenv (v.c_str ());
  }

  env_var_map mod_vars_;
  env_var_set vars_set_;
};

#ifdef NS
#undef NS
#endif
#ifdef NSPH
#undef NSPH
#endif

} // namespace test
} // namespace utsushi

#endif /* utsushi_test_environment_hpp_ */
