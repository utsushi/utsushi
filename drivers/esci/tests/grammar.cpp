//  grammar.cpp -- unit tests for ESC/I grammar API
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

#include <ios>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/static_assert.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include <utsushi/functional.hpp>
#include <utsushi/test/tools.hpp>

#include "../grammar.hpp"

#if __cplusplus >= 201103L
#define NS(bind) std::bind
#define NSPH(_1) std::placeholders::_1
#else
#define NS(bind) bind
#define NSPH(_1) _1
#endif

namespace esci = utsushi::_drv_::esci;

using esci::byte;
using esci::byte_buffer;
using esci::header;
using esci::integer;
using esci::quad;
using esci::status;

enum test_result {
  fail,
  pass,
  exception,
};

struct grammar_tc
{
  std::string name;
  test_result expect;
  byte_buffer payload;
  header      hdr;
  status      stat;
};

static const int header_length = 12;

namespace decoding {

using namespace esci::decoding;

typedef basic_grammar< default_iterator_type > grammar;
typedef qi::expectation_failure< default_iterator_type > expectation_failure;

void
test_grammar (const grammar_tc& tc)
{
  utsushi::test::change_test_case_name (tc.name);

  grammar parse;

  grammar::iterator head = tc.payload.begin ();
  grammar::iterator tail = tc.payload.end ();

  header h (quad (), esci_non_int);
  bool r (fail);

  try
    {
      r = parse.header_(head, tail, h);

      BOOST_REQUIRE_MESSAGE (exception != tc.expect,
                             "was expecting an expectation_failure");

      BOOST_CHECK_MESSAGE (tc.expect == r, parse.trace ());

      if (pass == tc.expect)
        {
          BOOST_CHECK_EQUAL (tc.hdr.code, h.code);
          BOOST_CHECK_EQUAL (tc.hdr.size, h.size);

          grammar::iterator expected = tc.payload.begin ();
          expected += header_length;

          BOOST_CHECK (expected == head);
        }
      else
        {
          BOOST_CHECK (tc.payload.begin () == head);
        }
    }
  catch (expectation_failure& e)
    {
      BOOST_CHECK_MESSAGE (exception == tc.expect,
                           "caught an unexpected expectation_failure");
    }
}

}       // namespace decoding

namespace encoding {

using namespace esci::encoding;

typedef basic_grammar< default_iterator_type > grammar;

void
test_grammar (const grammar_tc& tc)
{
  utsushi::test::change_test_case_name (tc.name);

  grammar generate;

  byte_buffer buf;
  bool r (fail);

  r = generate.header_(std::back_inserter (buf), tc.hdr);

  BOOST_CHECK_MESSAGE (tc.expect == r, generate.trace ());

  if (pass == tc.expect)
    {
      byte_buffer::const_iterator expected = tc.payload.begin ();
      expected += header_length;

      BOOST_CHECK_EQUAL_COLLECTIONS (tc.payload.begin (), expected,
                                     buf.begin (), buf.end ());
    }
  else
    {
      BOOST_CHECK (buf.empty ());
    }
}

}       // namespace encoding

bool
init_test_runner ()
{
  namespace bfs = boost::filesystem;
  namespace but = boost::unit_test;

  bfs::path path (getenv ("srcdir"));
  path /= "grammar.tcs";

  bfs::ifstream file;
  file.open (path);

  if (!file.is_open ())
    {
      std::stringstream ss;
      ss << "\n"
         << __FILE__ << ":"
         << __LINE__ << ": "
         << "failure opening: " << path;

      BOOST_THROW_EXCEPTION (std::ios_base::failure (ss.str ()));
    }

  std::vector< grammar_tc > decoding_tcs;
  std::vector< grammar_tc > encoding_tcs;

  std::string line;
  int cnt (0);

  while (!getline (file, line).eof ())
    {
      ++cnt;

      if (line.empty () || '#' == line[0])
        continue;

      std::istringstream is (line);

      grammar_tc tc;

      is >> tc.name;

      /**/ if (0 == tc.name.find ("Req-"));
      else if (0 == tc.name.find ("Rep-"));
      else break;

      std::string expect;

      is >> expect;
      std::transform (expect.begin (), expect.end (), expect.begin (),
                      NS(bind) (std::tolower< char >, NSPH(_1),
                                std::locale::classic ()));

      /**/ if (expect == "pass") tc.expect = pass;
      else if (expect == "fail") tc.expect = fail;
      else if (expect == "throw") tc.expect = exception;
      else break;

      bool is_decoding_tc = (0 == tc.name.find ("Rep-"));

      char quote;
      while (!is.eof () && '"' != is.peek ())
        is.get (quote);
      if (is.eof ()) break;
      is.get (quote);           // consumed the first double-quote

      if (is_decoding_tc)
        {
          while (!is.eof () && '"' != is.peek ())
            {
              char c;
              is.get (c);
              tc.payload.push_back (c);
            }
          if (is.eof ()) break;
          is.get (quote);       // consumed the second double-quote

          if (pass == tc.expect)
            {
              char quote;
              while (!is.eof () && '"' != is.peek ())
                is.get (quote);
              if (is.eof ()) break;
              is.get (quote);

              byte b1, b2, b3, b4;
              is.get (b1).get (b2).get (b3).get (b4);
              is.get (quote);
              if ('"' != quote) break;

              integer size;
              is >> std::hex >> size;

              tc.hdr = header (CODE_TOKEN (b1, b2, b3, b4), size);
            }
          decoding_tcs.push_back (tc);
        }
      else
        {
          byte b1, b2, b3, b4;
          is.get (b1).get (b2).get (b3).get (b4);
          is.get (quote);
          if ('"' != quote) break;

          integer size;
          is >> std::hex >> size;

          tc.hdr = header (CODE_TOKEN (b1, b2, b3, b4), size);

          if (pass == tc.expect)
            {
              char quote;
              while (!is.eof () && '"' != is.peek ())
                is.get (quote);
              if (is.eof ()) break;
              is.get (quote);

              while (!is.eof () && '"' != is.peek ())
                {
                  char c;
                  is.get (c);
                  tc.payload.push_back (c);
                }
              if (is.eof ()) break;
              is.get (quote);
            }
          encoding_tcs.push_back (tc);
        }
    }

  if (!file.eof ())
    {
      std::stringstream ss;
      ss << "\n"
         << "parse failure in " << path << "\n"
         << path << ":" << cnt << ": " << line;

      BOOST_THROW_EXCEPTION (std::ios_base::failure (ss.str ()));
    }

  if (!decoding_tcs.empty ())
    {
      but::test_suite *decoding_ts = BOOST_TEST_SUITE ("decoder");
      decoding_ts->add (BOOST_PARAM_TEST_CASE (decoding::test_grammar,
                                               decoding_tcs.begin (),
                                               decoding_tcs.end ()));
      but::framework::master_test_suite ().add (decoding_ts);
    }
  if (!encoding_tcs.empty ())
    {
      but::test_suite *encoding_ts = BOOST_TEST_SUITE ("encoder");
      encoding_ts->add (BOOST_PARAM_TEST_CASE (encoding::test_grammar,
                                               encoding_tcs.begin (),
                                               encoding_tcs.end ()));
      but::framework::master_test_suite ().add (encoding_ts);
    }
  return true;
}

#include "utsushi/test/runner.ipp"
