//  value.cpp -- unit tests for the sane::value API
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

#include <boost/test/unit_test.hpp>

#include "../value.hpp"

BOOST_AUTO_TEST_CASE (integer_from_utsushi_quantity)
{
  utsushi::quantity uq (1);
  sane::value sv (uq);

  BOOST_CHECK_EQUAL (SANE_TYPE_INT, sv.type ());

  SANE_Word v;

  sv >> (void *) &v;

  BOOST_CHECK_EQUAL (1, v);
}

BOOST_AUTO_TEST_CASE (integer_from_utsushi_value)
{
  utsushi::value uv (1);
  sane::value sv (uv);

  BOOST_CHECK_EQUAL (SANE_TYPE_INT, sv.type ());

  SANE_Word v;

  sv >> (void *) &v;

  BOOST_CHECK_EQUAL (1, v);
}

BOOST_AUTO_TEST_CASE (fixed_from_utsushi_quantity)
{
  utsushi::quantity uq (1.);
  sane::value sv (uq);

  BOOST_CHECK_EQUAL (SANE_TYPE_FIXED, sv.type ());

  SANE_Word v;

  sv >> (void *) &v;

  BOOST_CHECK_EQUAL (1, SANE_UNFIX (v));
}

BOOST_AUTO_TEST_CASE (fixed_from_utsushi_value)
{
  utsushi::value uv (1.);
  sane::value sv (uv);

  BOOST_CHECK_EQUAL (SANE_TYPE_FIXED, sv.type ());

  SANE_Word v;

  sv >> (void *) &v;

  BOOST_CHECK_EQUAL (1, SANE_UNFIX (v));
}

#include "utsushi/test/runner.ipp"
