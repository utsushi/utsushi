//  scanner.cpp -- unit tests for the utsushi::scanner API
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#include "utsushi/scanner.hpp"

using namespace utsushi;

BOOST_AUTO_TEST_CASE (udi_validation)
{
  BOOST_CHECK (!scanner::info::is_valid (""));

  BOOST_CHECK (!scanner::info::is_valid (":"));
  BOOST_CHECK (!scanner::info::is_valid ("drv:"));
  BOOST_CHECK (!scanner::info::is_valid (":cnx"));
  BOOST_CHECK (!scanner::info::is_valid ("drv:cnx"));

  BOOST_CHECK (!scanner::info::is_valid ("::"));
  BOOST_CHECK ( scanner::info::is_valid ("drv::"));
  BOOST_CHECK ( scanner::info::is_valid (":cnx:"));
  BOOST_CHECK (!scanner::info::is_valid ("::path"));

  BOOST_CHECK ( scanner::info::is_valid ("drv:cnx:"));
  BOOST_CHECK ( scanner::info::is_valid ("drv::path"));
  BOOST_CHECK ( scanner::info::is_valid (":cnx:path"));
  BOOST_CHECK ( scanner::info::is_valid ("drv:cnx:path"));

  BOOST_CHECK ( scanner::info::is_valid ("drv-net::"));
  BOOST_CHECK (!scanner::info::is_valid ("drv_net::"));
  BOOST_CHECK ( scanner::info::is_valid (":cnx-net:"));
  BOOST_CHECK (!scanner::info::is_valid (":cnx_net:"));
  BOOST_CHECK ( scanner::info::is_valid ("drv-net:cnx-net:path_net"));

  // Linux USB device below /sys
  BOOST_CHECK_NO_THROW
    (scanner::info ("drv:cnx:/sys/devices/pci0000:00/0000:00:1a.2/usb7/7-1"));
  // IPv4 numeric address with port number
  BOOST_CHECK_NO_THROW
    (scanner::info ("drv:ipv4://192.168.0.0:1865"));
  // IPv6 with leading zeroes replaced by a double colon
  BOOST_CHECK_NO_THROW
    (scanner::info ("drv:ipv6://::1"));
}

BOOST_AUTO_TEST_CASE (simple_splitting)
{
  scanner::info info ("drv:cnx:path?query#fragment");

  BOOST_CHECK_EQUAL ("cnx", info.connexion ());
  BOOST_CHECK_EQUAL ("drv", info.driver ());
  BOOST_CHECK_EQUAL ("path", info.path ());
  BOOST_CHECK_EQUAL ("query", info.query ());
  BOOST_CHECK_EQUAL ("fragment", info.fragment ());
}

BOOST_AUTO_TEST_CASE (no_such_splitting)
{
  scanner::info drv ("drv::path#fragment");
  scanner::info cnx (":cnx:?query");

  BOOST_CHECK_EQUAL ("", drv.connexion ());
  BOOST_CHECK_EQUAL ("", drv.query ());
  BOOST_CHECK_EQUAL ("", cnx.driver ());
  BOOST_CHECK_EQUAL ("", cnx.path ());
  BOOST_CHECK_EQUAL ("", cnx.fragment ());
}

BOOST_AUTO_TEST_CASE (driver_splicing)
{
  scanner::info info (":cnx:path");

  BOOST_CHECK (!info.is_driver_set ());

  std::string path = info.path ();
  BOOST_CHECK_EQUAL ("path", path);

  info.driver ("drv");
  BOOST_CHECK_EQUAL ("drv", info.driver ());
  BOOST_CHECK_EQUAL (path, info.path());
}

BOOST_AUTO_TEST_CASE (connexion_splicing)
{
  scanner::info info ("drv::path");

  BOOST_CHECK (info.connexion ().empty ());

  std::string path = info.path ();
  BOOST_CHECK_EQUAL ("path", path);

  info.connexion ("cnx");
  BOOST_CHECK_EQUAL ("cnx", info.connexion ());
  BOOST_CHECK_EQUAL (path, info.path());
}

BOOST_AUTO_TEST_CASE (local_device)
{
  scanner::info local  ("drv:usb:04b8:0123");   // vendor/product ID
  scanner::info remote ("drv:ipv4://192.168.0.0:1865");

  BOOST_CHECK ( local .is_local ());
  BOOST_CHECK (!remote.is_local ());
}

#include "utsushi/test/runner.ipp"
