//  shell-pipe.cpp -- unit tests for the shell-pipe filter implementation
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

#include "../shell-pipe.hpp"

#include <utsushi/file.hpp>
#include <utsushi/format.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>
#include <utsushi/test/tools.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include <csignal>
#include <list>
#include <string>
#include <utility>

#include <fcntl.h>

namespace fs = boost::filesystem;

using namespace utsushi;
using boost::assign::list_of;

static int pipe_capacity = 0;

class shell_pipe
  : public _flt_::shell_pipe
{
public:
  shell_pipe (const std::string& command)
    : _flt_::shell_pipe (command)
  {}
};

void
test_throughput (const std::pair< streamsize, unsigned >& args)
{
  const streamsize octet_count (args.first);
  const unsigned   image_count (args.second);

  rawmem_idevice dev (octet_count, image_count);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< shell_pipe > ("cat"));
  str.push (make_shared< file_odevice >
             (path_generator ("throughput-%3i.out")));

  idev | str;

  for (unsigned i = 0; i < image_count; ++i)
    {
      format fmt ("throughput-%|03|.out");
      std::string file ((fmt % i).str ());

      BOOST_CHECK (fs::exists (file));
      BOOST_CHECK_EQUAL (fs::file_size (file), octet_count);
      fs::remove (file);
    }
}

void
test_chaining (int length)
{
  test::suffix_test_case_name (boost::lexical_cast< std::string > (length));

  const streamsize  octet_count (2.5 * pipe_capacity);
  const unsigned    image_count (2);
  const std::string file ("chaining.out");

  rawmem_idevice dev (octet_count, image_count);
  idevice& idev (dev);

  stream str;
  for (int i = 0; i < length; ++i)
    {
      str.push (make_shared< shell_pipe > ("cat"));
    }
  str.push (make_shared< file_odevice > (file));

  idev | str;

  BOOST_CHECK (fs::exists (file));
  BOOST_CHECK_EQUAL (fs::file_size (file), octet_count * image_count);
  fs::remove (file);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

#ifdef F_GETPIPE_SZ
  pipe_capacity = fcntl (STDIN_FILENO, F_GETPIPE_SZ);
#endif

  if (0 >= pipe_capacity)
    pipe_capacity = 64 * 1024;  // Linux >= 2.6.11

  BOOST_TEST_MESSAGE ("Running throughput tests for pipes with "
                      << pipe_capacity << " byte capacity");

  std::list< std::pair< streamsize, unsigned > > args;
  boost::assign::push_back (args)
    (    pipe_capacity / 4, 1)  // single image scenarios
    (3 * pipe_capacity / 4, 1)
    (    pipe_capacity - 1, 1)
    (    pipe_capacity    , 1)
    (    pipe_capacity + 1, 1)
    (4 * pipe_capacity    , 1)

    (    pipe_capacity / 4, 2)  // multi-image scenarios
    (    pipe_capacity / 4, 3)
    (    pipe_capacity / 4, 4)
    (    pipe_capacity / 4, 5)
    (3 * pipe_capacity / 4, 2)
    (3 * pipe_capacity / 4, 3)

    (1, 1)                      // corner cases
    ;

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_throughput,
                                 args.begin (), args.end ()));

  std::list< int > lengths = list_of (2)(3)(4)(5);
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (&test_chaining,
                                 lengths.begin (), lengths.end ()));

  return true;
}

#include "utsushi/test/runner.ipp"
