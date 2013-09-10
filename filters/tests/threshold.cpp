//  threshold.cpp -- unit tests for the threshold filter implementation
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#include <boost/test/unit_test.hpp>

#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>

#include "../pnm.hpp"
#include "../threshold.hpp"

using namespace utsushi;
using _flt_::threshold;
using _flt_::pnm;

struct fixture
  : public threshold
{
  fixture () : name_("threshold.pnm") {}
  ~fixture () { remove (name_); }

  const fs::path name_;
};

BOOST_FIXTURE_TEST_CASE (gray_to_binary, fixture)
{
  context ctx (8, 2, context::GRAY8);
  istream istr;
  ostream ostr;
  shared_ptr<setmem_idevice::generator> gen
    = make_shared< const_generator > (0x7f);
  octet o[] = {'P', '4', ' ', '8', ' ', '2', '\n',
               octet(0xff), octet(0xff)};

  istr.push (make_shared< setmem_idevice > (gen, ctx, 1));
  ostr.push (make_shared< threshold > ());
  ostr.push (make_shared< pnm > ());
  ostr.push (make_shared< file_odevice > (name_));

  istr | ostr;

  fs::ifstream file;
  file.open (name_);
  octet *buf = new octet [sizeof (o)];
  file.read (buf, sizeof (o));
  file.close ();
  BOOST_CHECK_EQUAL_COLLECTIONS (o, o + sizeof (o),
                                 buf, buf + sizeof (o));
  delete [] buf;
}

#include "utsushi/test/runner.ipp"
