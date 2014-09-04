//  lut.cpp -- unit tests for LUT filtering support implementation
//  Copyright (C) 2012-2014  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>

#include "../pnm.hpp"
#include "../lut.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using namespace utsushi;
using namespace _flt_;

struct fixture : public bc_lut
{
  const std::string name_;

  fixture () : name_("lut.pnm") {}
  ~fixture () { fs::remove (name_); }
};

BOOST_FIXTURE_TEST_CASE (gray_to_binary, fixture)
{
  octet o[] = {'P', '5', ' ', '2', ' ', '2', ' ', '2', '5', '5', '\n',
               0x40, 0x40, 0x40, 0x40};

  context ctx (2, 2, context::GRAY8);
  shared_ptr<setmem_idevice::generator> gen
    = make_shared< const_generator > (0x7f);

  setmem_idevice dev (gen, ctx, 1);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< bc_lut > (-0.5, -0.5));
  str.push (make_shared< pnm > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  std::ifstream file;
  file.open (name_.c_str ());
  octet *buf = new octet [sizeof (o)];
  file.read (buf, sizeof (o));
  file.close ();
  BOOST_CHECK_EQUAL_COLLECTIONS (o, o + sizeof (o),
                                 buf, buf + sizeof (o));
  delete [] buf;
}

BOOST_FIXTURE_TEST_CASE (octets2index_test, lut)
{
  octet o[] = {0x01, 0x23, 0x45, 0x67};
  uint32_t i;

  i = octets2index (o, 0);
  BOOST_CHECK_EQUAL (i, 0);

  i = octets2index (o, 1);
  BOOST_CHECK_EQUAL (i, 0x01);

  i = octets2index (o, 2);
  BOOST_CHECK_EQUAL (i, 0x0123);

  i = octets2index (o, 3);
  BOOST_CHECK_EQUAL (i, 0x012345);

  i = octets2index (o, 4);
  BOOST_CHECK_EQUAL (i, 0x01234567);

  o[0] = 0xff;
  i = octets2index (o, 1);
  BOOST_CHECK_EQUAL (i, 0xff);

  i = octets2index (o, 2);
  BOOST_CHECK_EQUAL (i, 0xff23);
}

BOOST_FIXTURE_TEST_CASE (index2octets_test, lut)
{
  octet o[5] = {0,};
  octet result[5] = {0,};

  result[0] = 0x01;
  index2octets (o, 0x01, 1);
  BOOST_CHECK_EQUAL (o, result);

  result[1] = 0x23;
  index2octets (o, 0x0123, 2);
  BOOST_CHECK_EQUAL (o, result);

  result[2] = 0x45;
  index2octets (o, 0x012345, 3);
  BOOST_CHECK_EQUAL (o, result);

  result[3] = 0x67;
  index2octets (o, 0x01234567, 4);
  BOOST_CHECK_EQUAL (o, result);

  result[0] = 0xfe;
  result[1] = 0xdc;
  result[2] = 0xba;
  result[3] = 0x98;
  index2octets (o, 0xfedcba98, 4);
  BOOST_CHECK_EQUAL (o, result);
}


struct lut_test : public bc_lut
{
  using bc_lut::boi;
  using bc_lut::eoi;

  using bc_lut::option_;
  using bc_lut::lut_;
};

#define ARRAY_END(a)  ((a) + sizeof (a) / sizeof (a[0]))

struct param
{
  const char *key;
  double val;
  int depth;
  int in;
  int out;
};

void
test_bc_lut (param &arg)
{
  BOOST_TEST_MESSAGE (__FUNCTION__ << " (" << arg.key << ", "
                      << arg.val  << ", " << arg.depth << ")");
  lut_test lt;
  context ctx (1, 1, (arg.depth == 16
                      ? context::RGB16
                      : context::RGB8));

  (*lt.option_)[arg.key] = arg.val;
  lt.boi (ctx);
  BOOST_CHECK_EQUAL (arg.out, lt.lut_[arg.in]);
  lt.eoi (ctx);
}

bool
init_test_runner ()
{
  namespace but =::boost::unit_test;

  param arg[] = {
    // key, value, depth, in, out
    {"brightness",  1.0,  8,   0, 127},
    {"brightness",  1.0,  8,   1, 128},
    {"brightness",  1.0,  8, 127, 254},
    {"brightness",  1.0,  8, 128, 255},
    {"brightness",  1.0,  8, 254, 255},
    {"brightness",  1.0,  8, 255, 255},

    {"brightness",  0.0,  8,   0,   0},
    {"brightness",  0.0,  8,   1,   1},
    {"brightness",  0.0,  8, 127, 127},
    {"brightness",  0.0,  8, 128, 128},
    {"brightness",  0.0,  8, 254, 254},
    {"brightness",  0.0,  8, 255, 255},

    {"brightness", -1.0,  8,   0,   0},
    {"brightness", -1.0,  8,   1,   0},
    {"brightness", -1.0,  8, 127,   0},
    {"brightness", -1.0,  8, 128,   1},
    {"brightness", -1.0,  8, 254, 127},
    {"brightness", -1.0,  8, 255, 128},

    {"brightness",  1.0, 16,     0, 32767+    0},
    {"brightness",  1.0, 16,     1, 32767+    1},
    {"brightness",  1.0, 16, 32767, 32767+32767},
    {"brightness",  1.0, 16, 32768, 65535},
    {"brightness",  1.0, 16, 65534, 65535},
    {"brightness",  1.0, 16, 65535, 65535},

    {"brightness",  0.0, 16,     0,     0},
    {"brightness",  0.0, 16,     1,     1},
    {"brightness",  0.0, 16, 32767, 32767},
    {"brightness",  0.0, 16, 32768, 32768},
    {"brightness",  0.0, 16, 65534, 65534},
    {"brightness",  0.0, 16, 65535, 65535},

    {"brightness", -1.0, 16,     0, 0},
    {"brightness", -1.0, 16,     1, 0},
    {"brightness", -1.0, 16, 32767, 0},
    {"brightness", -1.0, 16, 32768, 32768-32767},
    {"brightness", -1.0, 16, 65534, 65534-32767},
    {"brightness", -1.0, 16, 65535, 65535-32767},

    {"contrast",    1.0,  8,   0,   0},
    {"contrast",    1.0,  8,   1,   0},
    {"contrast",    1.0,  8, 127,   0},
    {"contrast",    1.0,  8, 128, 255},
    {"contrast",    1.0,  8, 254, 255},
    {"contrast",    1.0,  8, 255, 255},

    {"contrast",    0.0,  8,   0,   0},
    {"contrast",    0.0,  8,   1,   1},
    {"contrast",    0.0,  8, 127, 127},
    {"contrast",    0.0,  8, 128, 128},
    {"contrast",    0.0,  8, 254, 254},
    {"contrast",    0.0,  8, 255, 255},

    {"contrast",   -1.0,  8,   0,  63},
    {"contrast",   -1.0,  8,   1,  64},
    {"contrast",   -1.0,  8, 127, 127},
    {"contrast",   -1.0,  8, 128, 127},
    {"contrast",   -1.0,  8, 254, 190},
    {"contrast",   -1.0,  8, 255, 191},

    {"contrast",    1.0, 16,     0,     0},
    {"contrast",    1.0, 16,     1,     0},
    {"contrast",    1.0, 16, 32767,     0},
    {"contrast",    1.0, 16, 32768, 65535},
    {"contrast",    1.0, 16, 65534, 65535},
    {"contrast",    1.0, 16, 65535, 65535},

    {"contrast",    0.0, 16,     0,     0},
    {"contrast",    0.0, 16,     1,     1},
    {"contrast",    0.0, 16, 32767, 32767},
    {"contrast",    0.0, 16, 32768, 32768},
    {"contrast",    0.0, 16, 65534, 65534},
    {"contrast",    0.0, 16, 65535, 65535},

    {"contrast",   -1.0, 16,     0, 16383},
    {"contrast",   -1.0, 16,     1, 16384},
    {"contrast",   -1.0, 16, 32767, 32767},
    {"contrast",   -1.0, 16, 32768, 32767},
    {"contrast",   -1.0, 16, 65534, 49150},
    {"contrast",   -1.0, 16, 65535, 49151},
  };

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_bc_lut, arg, ARRAY_END (arg)));

  return true;
}

#include "utsushi/test/runner.ipp"
