//  log.cpp -- unit tests for the utsushi::log API
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

#include <algorithm>
#include <sstream>
#include <string>

#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#ifndef UTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED
#error "Makefile.am is supposed to set this to all possible values"
#endif

// Tie log::quark behaviour to the argument count checking behaviour.
// This is not a problem because we are not interested in the argument
// count checking for log quarks.  We are solely interested in whether
// a log::quark does or does not produce output.

#ifdef ENABLE_LOG_QUARK
#undef ENABLE_LOG_QUARK
#endif
#define ENABLE_LOG_QUARK UTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED

#include "utsushi/log.hpp"

using namespace utsushi;

struct fixture
{
  std::ostringstream s;
  std::streambuf *buf;

  //!  Ensure something gets logged
  fixture ()
  {
    log::threshold = log::BRIEF;
    log::matching  = log::ALL;

    buf = log::basic_logger<char>::os_.rdbuf (s.rdbuf ());
  }
  ~fixture ()
  {
    log::basic_logger<char>::os_.rdbuf (buf);
  }
};

BOOST_FIXTURE_TEST_CASE (format_overflow, fixture)
{
  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (log::message (log::FATAL, "%1%") % 1 % 2,
                         boost::io::too_many_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (log::message (log::FATAL, "%1%") % 1 % 2);
    }
}

BOOST_FIXTURE_TEST_CASE (format_reuse_overflow, fixture)
{
  log::message fmt (log::FATAL, "%1%");

  s << fmt % 1;
  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (fmt % 1 % 2, boost::io::too_many_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (fmt % 1 % 2);
    }
}

BOOST_FIXTURE_TEST_CASE (format_underflow, fixture)
{
  log::message fmt (log::FATAL, "%1% %2%");

  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (s << fmt % 1, boost::io::too_few_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (s << fmt % 1);
    }
}

BOOST_FIXTURE_TEST_CASE (format_reuse_underflow, fixture)
{
  log::message fmt (log::FATAL, "%1% %2%");

  s << fmt % 1 % 2;
  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (s << fmt % 1, boost::io::too_few_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (s << fmt % 1);
    }
}

BOOST_FIXTURE_TEST_CASE (noisy_named_ctor_overflow, fixture)
{
  BOOST_REQUIRE (log::threshold >= log::ALERT);

  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (s << log::alert ("%1%") % 1 % 2,
                         boost::io::too_many_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (s << log::alert ("%1%") % 1 % 2);
    }
}

BOOST_FIXTURE_TEST_CASE (noisy_named_ctor_underflow, fixture)
{
  BOOST_REQUIRE (log::threshold >= log::ALERT);

  log::message fmt (log::ALERT, "%1%");

  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (s << fmt, boost::io::too_few_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (s << fmt);
    }
}

BOOST_FIXTURE_TEST_CASE (quiet_named_ctor_overflow, fixture)
{
  BOOST_REQUIRE (log::threshold < log::TRACE);

  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (s << log::trace ("%1%") % 1 % 2,
                         boost::io::too_many_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (s << log::trace ("%1%") % 1 % 2);
    }
}

BOOST_FIXTURE_TEST_CASE (quiet_named_ctor_underflow, fixture)
{
  BOOST_REQUIRE (log::threshold < log::TRACE);

  log::message fmt (log::TRACE, "%1%");

  if (log::arg_count_checking)
    {
      BOOST_CHECK_THROW (s << fmt, boost::io::too_few_args);
    }
  else
    {
      BOOST_CHECK_NO_THROW (s << fmt);
    }
}

BOOST_FIXTURE_TEST_CASE (quark_verbosity, fixture)
{
  log::threshold = log::QUARK;

  { log::quark (); }

  BOOST_CHECK_EQUAL (ENABLE_LOG_QUARK, !s.str ().empty ());
}

template <typename charT, typename fmtT>
void
verbosity (log::priority level)
{
  log::threshold = level;
  log::matching  = log::ALL;

  // construct an empty message format of a certain length
  // in a way that works for all supported charT's
  std::basic_string<charT> str;
  const int length = 5;
  str.resize (length);

  fmtT fmt (str);

  std::basic_ostringstream<charT> s;
  std::basic_streambuf<charT> *buf
    = log::basic_logger<charT>::os_.rdbuf (s.rdbuf ());

  // Make sure all messages are out of scope by the time we start
  // checking things.
  {
    log::fatal (fmt);
    log::alert (fmt);
    log::error (fmt);
    log::brief (fmt);
    log::trace (fmt);
    log::debug (fmt);
  }

  // This assumes that the message format does not add any charT()
  // into the string it generates.
  int charT_expect = length * (level + 1);
  std::basic_string<charT> msg = s.str ();
  int charT_result = std::count (msg.begin (), msg.end (), charT ());

  BOOST_CHECK_EQUAL (charT_expect, charT_result);

  log::basic_logger<charT>::os_.rdbuf (buf);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

  std::list<log::priority> levels;
  levels.push_back (log::FATAL);
  levels.push_back (log::TRACE);
  levels.push_back (log::ERROR);
  levels.push_back (log::DEBUG);
  levels.push_back (log::ALERT);
  levels.push_back (log::BRIEF);

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE ((verbosity<char, std::string>),
                                 levels.begin (), levels.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE ((verbosity<wchar_t, std::wstring>),
                                 levels.begin (), levels.end ()));

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE ((verbosity<char, boost::format>),
                                 levels.begin (), levels.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE ((verbosity<wchar_t, boost::wformat>),
                                 levels.begin (), levels.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE ((verbosity<char, utsushi::format>),
                                 levels.begin (), levels.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE ((verbosity<wchar_t, utsushi::wformat>),
                                 levels.begin (), levels.end ()));

  return true;
}

#include "utsushi/test/runner.ipp"
