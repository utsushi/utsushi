//  value.cpp -- unit tests for the utsushi::value API
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

#include "utsushi/value.hpp"

using namespace utsushi;

/*  While every bounded type may have a suitable default value, we can
 *  not assume a default bounded type.  As a logical consequence, that
 *  means there really should be no default `value` constructor.
 *  However, the operator[] API for "value_maps" requires one (for the
 *  same API in the `group` class, we can absorb that in the `visitor`
 *  implementation).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE (no_usable_default_value, T, value::types)
{
  T t;
  value v;

  BOOST_CHECK_THROW (t = v, boost::bad_get);
}

#include "utsushi/test/runner.ipp"
