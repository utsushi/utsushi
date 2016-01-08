//  grammar-formats.cpp -- unit tests for ESC/I grammar-formats API
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
#include <boost/integer_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/functional.hpp>
#include <utsushi/test/tools.hpp>

#include "../grammar-formats.hpp"

#if __cplusplus >= 201103L
#define NS(bind) std::bind
#define NSPH(_1) std::placeholders::_1
#else
#define NS(bind) bind
#define NSPH(_1) _1
#endif

namespace esci = utsushi::_drv_::esci;

using esci::byte_buffer;
using esci::integer;

namespace {

using utsushi::streamsize;

//  Make sure an integer type has been selected so that it covers all
//  possible values of the "compound" protocol variants.

BOOST_STATIC_ASSERT
((   boost::integer_traits< integer >::const_min <= esci_dec_min
  && boost::integer_traits< integer >::const_max >= esci_dec_max
  && boost::integer_traits< integer >::const_min <= esci_int_min
  && boost::integer_traits< integer >::const_max >= esci_int_max
  && boost::integer_traits< integer >::const_min <= esci_hex_min
  && boost::integer_traits< integer >::const_max >= esci_hex_max
  && boost::integer_traits< integer >::const_min <= esci_bin_min
  && boost::integer_traits< integer >::const_max >= esci_bin_max
  ));

//  Make sure that protocol provided integral values stay below the
//  maximum number of bytes that can be transferred in a single I/O
//  transaction.

BOOST_STATIC_ASSERT
((   boost::integer_traits< streamsize >::const_max >= esci_dec_max
  && boost::integer_traits< streamsize >::const_max >= esci_int_max
  && boost::integer_traits< streamsize >::const_max >= esci_hex_max
  && boost::integer_traits< streamsize >::const_max >= esci_bin_max
  ));

//  Make sure the integer constant definitions make logical sense.

BOOST_STATIC_ASSERT
((   esci_dec_min < esci_dec_max
  && esci_int_min < esci_int_max
  && esci_hex_min < esci_hex_max
  && esci_bin_min < esci_bin_max
  ));

//  Make sure that the non-integer literal is indeed outside the
//  ranges covered by all the coding schemes.

BOOST_STATIC_ASSERT
((   esci_non_int < 0
  && esci_non_int < esci_dec_min
  && esci_non_int < esci_int_min
  && esci_non_int < esci_hex_min
  && esci_non_int < esci_bin_min
  ));

}       // anonymous namespace

enum test_result {
  fail,
  pass,
};

struct grammar_formats_tc
{
  std::string name;
  test_result expect;
  byte_buffer payload;
  integer     value;
};

namespace decoding {

using namespace esci::decoding;

typedef basic_grammar_formats< default_iterator_type > formats;

void
test_grammar_formats (const grammar_formats_tc& tc)
{
  utsushi::test::change_test_case_name (tc.name);

  formats parser;

  formats::iterator head = tc.payload.begin ();
  formats::iterator tail = tc.payload.end ();

  integer value (esci_non_int);
  bool r (fail);

  /**/ if (0 == tc.name.find ("d-"))
    r = parser.parse (head, tail, parser.decimal_, value);
  else if (0 == tc.name.find ("i-"))
    r = parser.parse (head, tail, parser.integer_, value);
  else if (0 == tc.name.find ("x-"))
    r = parser.parse (head, tail, parser.hexadecimal_, value);
  else
    BOOST_REQUIRE_MESSAGE (r, "broken test case: " << tc.name);

  BOOST_CHECK_MESSAGE (tc.expect == r, parser.trace ());

  if (pass == tc.expect)
    {
      BOOST_CHECK_EQUAL (tc.value, value);

      formats::iterator expected = tc.payload.begin ();
      expected += (0 == tc.name.find ("d-") ? 4 : 8);

      BOOST_CHECK (expected == head);
    }
  else
    {
      BOOST_CHECK_EQUAL (value, esci_non_int);
      BOOST_CHECK (tc.payload.begin () == head);
    }
}

}       // namespace decoding

namespace encoding {

using namespace esci::encoding;

typedef basic_grammar_formats< default_iterator_type > formats;

void
test_grammar_formats (const grammar_formats_tc& tc)
{
  utsushi::test::change_test_case_name (tc.name);

  formats generator;
  byte_buffer number;

  bool r (fail);

  /**/ if (0 == tc.name.find ("d-"))
    r = generator.generate (std::back_inserter (number),
                            generator.decimal_, tc.value);
  else if (0 == tc.name.find ("i-"))
    r = generator.generate (std::back_inserter (number),
                            generator.integer_, tc.value);
  else if (0 == tc.name.find ("x-"))
    r = generator.generate (std::back_inserter (number),
                            generator.hexadecimal_, tc.value);
  else
    BOOST_REQUIRE_MESSAGE (r, "broken test case: " << tc.name);

  BOOST_CHECK_MESSAGE (tc.expect == r, generator.trace ());

  if (pass == tc.expect)
    {
      const byte_buffer payload = (0 != tc.payload.find ("i-000000")
                                   ? tc.payload
                                   : "i0000000");

      byte_buffer::const_iterator expected = payload.begin ();
      expected += (0 == tc.name.find ("d-") ? 4 : 8);

      BOOST_CHECK_EQUAL_COLLECTIONS (payload.begin (), expected,
                                     number.begin (), number.end ());
    }
  else
    {
      BOOST_CHECK (number.empty ());
    }
}

}       // namespace encoding

bool
init_test_runner ()
{
  namespace bfs = boost::filesystem;
  namespace but = boost::unit_test;

  bfs::path path (getenv ("srcdir"));
  path /= "grammar-formats.tcs";

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

  std::vector< grammar_formats_tc > decoding_tcs;
  std::vector< grammar_formats_tc > encoding_tcs;

  std::string line;
  int cnt (0);

  while (!getline (file, line).eof ())
    {
      ++cnt;

      if (line.empty () || '#' == line[0])
        continue;

      std::istringstream is (line);

      grammar_formats_tc tc;

      is >> tc.name;

      /**/ if (0 == tc.name.find ("d-"));
      else if (0 == tc.name.find ("i-"));
      else if (0 == tc.name.find ("x-"));
      else break;

      std::string expect;

      is >> expect;
      std::transform (expect.begin (), expect.end (), expect.begin (),
                      NS(bind) (std::tolower< char >, NSPH(_1),
                                std::locale::classic ()));

      /**/ if (expect == "pass") tc.expect = pass;
      else if (expect == "fail") tc.expect = fail;
      else break;

      char quote;
      while (!is.eof () && '"' != is.peek ())
        is.get (quote);
      if (is.eof ()) break;
      is.get (quote);           // consumed the first double-quote

      while (!is.eof () && '"' != is.peek ())
        {
          char c;
          is.get (c);
          tc.payload.push_back (c);
        }
      if (is.eof ()) break;
      is.get (quote);           // consumed the second double-quote

      if (pass == tc.expect)
        {
          if (0 == tc.name.find ("x-"))
            is >> std::hex >> tc.value;
          else
            is >> std::dec >> tc.value;

          encoding_tcs.push_back (tc);
        }
      decoding_tcs.push_back (tc);
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
      decoding_ts->add (BOOST_PARAM_TEST_CASE (decoding::test_grammar_formats,
                                               decoding_tcs.begin (),
                                               decoding_tcs.end ()));
      but::framework::master_test_suite ().add (decoding_ts);
    }
  if (!encoding_tcs.empty ())
    {
      but::test_suite *encoding_ts = BOOST_TEST_SUITE ("encoder");
      encoding_ts->add (BOOST_PARAM_TEST_CASE (encoding::test_grammar_formats,
                                               encoding_tcs.begin (),
                                               encoding_tcs.end ()));
      but::framework::master_test_suite ().add (encoding_ts);
    }
  return true;
}

#include "utsushi/test/runner.ipp"
