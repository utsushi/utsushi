//  pnm.cpp -- unit tests for the PNM filter implementation
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
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

#include <string>

#include <boost/test/unit_test.hpp>

#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>

#include "../pnm.hpp"

using namespace utsushi;
using _flt_::pnm;

struct fixture
{
  const fs::path name_;

  fixture () : name_("pnm.out") {}
  ~fixture () { remove (name_); }
};

BOOST_FIXTURE_TEST_CASE (triple_image, fixture)
{
  context ctx (100, 100);
  istream istr;
  ostream ostr;

  istr.push (idevice::ptr (new rawmem_idevice (ctx, 3)));
  ostr.push (ofilter::ptr (new pnm));
  ostr.push (odevice::ptr (new file_odevice (name_)));

  istr | ostr;

  std::string header = "P5 100 100 255\n";
  BOOST_CHECK_EQUAL (3 * (ctx.octets_per_image () + header.length ()),
                     file_size (name_));
}

#include "utsushi/test/runner.ipp"
