//  setter.cc -- unit tests for ESC/I setter commands
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

#include <algorithm>

#include <boost/test/unit_test.hpp>

#include "../setter.hpp"
#include "../set-dither-pattern.hpp"
#include "../set-scan-parameters.hpp"

using namespace utsushi::_drv_::esci;
using utsushi::traits;

//  Define some setter wrapper classes for testing purposes so that we
//  can get at the base class' protected parts.
//  The setter template base is instantiated with an arbitrary set of
//  template parameters (that just so happen to be different from all
//  the known ESC/I protocol setter commands).

static const int setter_data_size = 19;
#define template_parameters  0, 0, setter_data_size

struct test_setter : public setter< template_parameters >
{
  using setter< template_parameters >::cmd_;
  using setter< template_parameters >::dat_;
};

struct test_dither_pattern : public set_dither_pattern
{
  using set_dither_pattern::cmd_;
  using set_dither_pattern::dat_;
  using set_dither_pattern::dat_size;
};

struct test_scan_parameters : public set_scan_parameters
{
  using set_scan_parameters::cmd_;
  using set_scan_parameters::dat_;
};

BOOST_AUTO_TEST_CASE (setter_deep_copy)
{
  test_setter cmd1;

  traits::assign (cmd1.dat_, setter_data_size, 5);

  test_setter cmd2 (cmd1);

  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.cmd_, cmd1.cmd_ + 2,
                                 cmd2.cmd_, cmd2.cmd_ + 2);
  BOOST_CHECK (cmd1.dat_ != cmd2.dat_);
  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.dat_, cmd1.dat_ + setter_data_size,
                                 cmd2.dat_, cmd2.dat_ + setter_data_size);

  traits::assign (cmd2.dat_, setter_data_size, ~5);

  BOOST_CHECK (!std::equal (cmd1.dat_, cmd1.dat_ + setter_data_size,
                            cmd2.dat_));
}

BOOST_AUTO_TEST_CASE (setter_assignment)
{
  test_setter cmd1;
  test_setter cmd2;

  traits::assign (cmd1.dat_, setter_data_size, 5);

  cmd1 = cmd2;

  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.cmd_, cmd1.cmd_ + 2,
                                 cmd2.cmd_, cmd2.cmd_ + 2);
  BOOST_CHECK (cmd1.dat_ != cmd2.dat_);
  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.dat_, cmd1.dat_ + setter_data_size,
                                 cmd2.dat_, cmd2.dat_ + setter_data_size);

  traits::assign (cmd2.dat_, setter_data_size, ~5);

  BOOST_CHECK (!std::equal (cmd1.dat_, cmd1.dat_ + setter_data_size,
                            cmd2.dat_));
}

BOOST_AUTO_TEST_CASE (dither_deep_copy)
{
  test_dither_pattern cmd1;

  cmd1 (set_dither_pattern::CUSTOM_A);

  test_dither_pattern cmd2 (cmd1);

  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.cmd_, cmd1.cmd_ + 2,
                                 cmd2.cmd_, cmd2.cmd_ + 2);
  BOOST_CHECK (cmd1.dat_ != cmd2.dat_);
  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.dat_, cmd1.dat_ + cmd1.dat_size (),
                                 cmd2.dat_, cmd2.dat_ + cmd2.dat_size ());

  cmd2 (set_dither_pattern::CUSTOM_B);

  BOOST_CHECK (!std::equal (cmd1.dat_, cmd1.dat_ + cmd1.dat_size (),
                            cmd2.dat_));
}

BOOST_AUTO_TEST_CASE (dither_assignment)
{
  test_dither_pattern cmd1;
  test_dither_pattern cmd2;

  cmd1 (set_dither_pattern::CUSTOM_A);

  cmd2 = cmd1;

  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.cmd_, cmd1.cmd_ + 2,
                                 cmd2.cmd_, cmd2.cmd_ + 2);
  BOOST_CHECK (cmd1.dat_ != cmd2.dat_);
  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.dat_, cmd1.dat_ + cmd1.dat_size (),
                                 cmd2.dat_, cmd2.dat_ + cmd2.dat_size ());

  cmd2 (set_dither_pattern::CUSTOM_B);

  BOOST_CHECK (!std::equal (cmd1.dat_, cmd1.dat_ + cmd1.dat_size (),
                            cmd2.dat_));
}

BOOST_AUTO_TEST_CASE (dither_self_assignment)
{
  test_dither_pattern cmd;

  cmd (set_dither_pattern::CUSTOM_A);

  byte pay_load[cmd.dat_size ()];
  traits::copy (pay_load, cmd.dat_, cmd.dat_size ());

  cmd = cmd;

  BOOST_CHECK (cmd.dat_);
  BOOST_CHECK_EQUAL_COLLECTIONS (pay_load, pay_load + cmd.dat_size (),
                                 cmd.dat_, cmd.dat_ + cmd.dat_size ());
}

BOOST_AUTO_TEST_CASE (scan_parameters_deep_copy)
{
  test_scan_parameters cmd1;

  cmd1.color_mode (16);
  cmd1.line_count (50);

  test_scan_parameters cmd2 (cmd1);

  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.cmd_, cmd1.cmd_ + 2,
                                 cmd2.cmd_, cmd2.cmd_ + 2);
  BOOST_CHECK_EQUAL (cmd1.color_mode (), cmd2.color_mode ());
  BOOST_CHECK_EQUAL (cmd1.line_count (), cmd2.line_count ());

  cmd2.line_count (75);

  BOOST_CHECK_EQUAL (cmd1.color_mode (), cmd2.color_mode ());
  BOOST_CHECK (cmd1.line_count () != cmd2.line_count ());
}

BOOST_AUTO_TEST_CASE (scan_parameters_assignment)
{
  test_scan_parameters cmd1;
  test_scan_parameters cmd2;

  cmd1.color_mode (16);
  cmd1.line_count (50);

  cmd2 = cmd1;

  BOOST_CHECK_EQUAL_COLLECTIONS (cmd1.cmd_, cmd1.cmd_ + 2,
                                 cmd2.cmd_, cmd2.cmd_ + 2);
  BOOST_CHECK_EQUAL (cmd1.color_mode (), cmd2.color_mode ());
  BOOST_CHECK_EQUAL (cmd1.line_count (), cmd2.line_count ());

  cmd2.line_count (75);

  BOOST_CHECK_EQUAL (cmd1.color_mode (), cmd2.color_mode ());
  BOOST_CHECK (cmd1.line_count () != cmd2.line_count ());
}

#include "utsushi/test/runner.ipp"
