//  monitor.cpp -- unit tests for the utsushi::monitor API
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

#include <cstdlib>
#include <fstream>

#include <boost/test/unit_test.hpp>

#include "utsushi/monitor.hpp"

#include "utsushi/test/environment.hpp"
#include "../run-time.ipp"

namespace {

using namespace utsushi;

struct run_time_fixture
{
  const char *program_name_;

  run_time_fixture ()
    : program_name_("monitor-unit-test-runner")
  {
    const char *argv[] = { program_name_ };

    run_time (1, argv);
  }

  ~run_time_fixture ()
  {
    delete run_time::impl::instance_;
    run_time::impl::instance_ = nullptr;
  }
};

BOOST_FIXTURE_TEST_SUITE (conf_file, run_time_fixture)

BOOST_AUTO_TEST_CASE (devices_configuration)
{
  run_time rt;

  monitor::container_type devices;
  std::ifstream ifs (rt.conf_file (run_time::pkg, "devices.conf").c_str ());

  devices = monitor::read (ifs);

  BOOST_CHECK_EQUAL (3, devices.size ());
}

BOOST_AUTO_TEST_SUITE_END (/* conf_file */)

}       // namespace

#include "utsushi/test/runner.ipp"
