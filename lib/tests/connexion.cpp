//  connexion.cpp -- unit tests for the utsushi::ipc::connexion API
//  Copyright (C) 2013, 2015  SEIKO EPSON CORPORATION
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

#include "utsushi/connexion.hpp"

struct connexion
  : public utsushi::ipc::connexion
{
  connexion (const std::string& type, const std::string& path)
    : utsushi::ipc::connexion (type, path)
  {}

  // Make protected members public for testing purposes
  using utsushi::ipc::connexion::pid_;
  using utsushi::ipc::connexion::port_;
  using utsushi::ipc::connexion::socket_;
};

BOOST_AUTO_TEST_CASE (process_lifetime)
{
  connexion cnx ("ipc-cnx", "path");

  BOOST_CHECK_NE (-1, cnx.pid_);
  BOOST_CHECK_NE (-1, cnx.port_);
  BOOST_CHECK_NE (-1, cnx.socket_);
}

BOOST_AUTO_TEST_CASE (simple_xfer)
{
  connexion cnx ("ipc-cnx", "path");

  utsushi::octet obuf[] = "hello";
  utsushi::octet ibuf[] = "hello";

  cnx.send (obuf, sizeof (obuf));
  cnx.recv (ibuf, sizeof (ibuf));

  BOOST_CHECK_EQUAL (std::string ("HELLO"), ibuf);
}

#include "utsushi/test/runner.ipp"
