//  backend.cpp -- unit tests for the SANE utsushi backend
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

#include <cstdlib>
#include <cstring>

#include <boost/test/unit_test.hpp>
#include <boost/preprocessor/cat.hpp>

#include <ltdl.h>

#include <utsushi/monitor.hpp>

#define BACKEND_DISABLE_NAME_ALIASING
#include "../backend.hpp"
#include "../guard.hpp"

#ifndef BOOST_FIXTURE_TEST_SUITE_END
#define BOOST_FIXTURE_TEST_SUITE_END BOOST_AUTO_TEST_SUITE_END
#endif

struct preload_symbols
{
  preload_symbols ()
  {
    //LTDL_SET_PRELOADED_SYMBOLS ();
  }
};

BOOST_GLOBAL_FIXTURE (preload_symbols);

BOOST_AUTO_TEST_SUITE (sane_api_callability)

#define SANE_API_TEST(function)  BOOST_PP_CAT(test_sane_,function)
#define SANE_API_CALL(function)  BOOST_PP_CAT(     sane_,function)
#include "sane-api.ipp"
#undef  SANE_API_CALL
#undef  SANE_API_TEST

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE (sane_shadow_api_callability)

#define SANE_API_TEST(function)                                         \
  BOOST_PP_CAT(test_sane_,BOOST_PP_CAT(BACKEND_NAME,BOOST_PP_CAT(_,function)))
#define SANE_API_CALL(function)                                         \
  BOOST_PP_CAT(     sane_,BOOST_PP_CAT(BACKEND_NAME,BOOST_PP_CAT(_,function)))
#include "sane-api.ipp"
#undef  SANE_API_CALL
#undef  SANE_API_TEST

BOOST_AUTO_TEST_SUITE_END()

/*! def_group  FIXTURES  Fixtures for SANE backend unit testing.
 *
 *  Writing unit tests for any SANE backend quickly becomes a bit of a
 *  bore because you need to initialise SANE and open a device before
 *  you can do anything useful.  And after the test, you have to clean
 *  up again.
 *
 *  The fixtures in this module are meant to make that easy.  The test
 *  cases and suites can just use a \c BOOST_FIXTURE_* macro and list
 *  the fixture they need.
 *
 *  @{
 */

//! Backend initialization and clean up
struct backend_fixture
{
  backend_fixture ()
  {
    sane_init (NULL, NULL);
  }

  ~backend_fixture ()
  {
    sane_exit ();
  }
};

//! Opening and closing the default handle
struct handle_fixture : backend_fixture
{
  //! \todo Make sure we pick up a mock device instance
  handle_fixture ()
    : handle_(NULL)
  {
    SANE_Status status = sane_open ("", &handle_);

    BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, status);
    BOOST_REQUIRE (handle_);
  }

  ~handle_fixture ()
  {
    sane_close (handle_);
  }

  SANE_Handle handle_;
};

//! Making sure a backend is \e not initialized
struct uninit_fixture
{
  uninit_fixture ()
    : handle_(NULL)
  {
    sane_exit ();
  }

  SANE_Handle handle_;
};

//! Making a handle not opened by the backend
struct bad_handle_fixture : backend_fixture
{
  bad_handle_fixture ()
    : handle_(new bool)
  {
    BOOST_REQUIRE (handle_);
  }

  SANE_Handle handle_;
};

/*! @} */

BOOST_FIXTURE_TEST_SUITE (uninit_backend_callability, uninit_fixture)

BOOST_AUTO_TEST_CASE (uninit_get_devices)
{
  const SANE_Device **list;

  BOOST_CHECK_EQUAL (failure_status_, sane_get_devices (&list, SANE_TRUE));
}

BOOST_AUTO_TEST_CASE (uninit_open)
{
  SANE_Handle handle;

  BOOST_CHECK_EQUAL (failure_status_, sane_open ("", &handle));
}

BOOST_AUTO_TEST_CASE (uninit_get_option_descriptor)
{
  BOOST_CHECK (!sane_get_option_descriptor (handle_, 0));
}

BOOST_AUTO_TEST_CASE (uninit_control_option)
{
  SANE_Int count = 0;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (uninit_get_parameters)
{
  SANE_Parameters params;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_get_parameters (handle_, &params));
}

BOOST_AUTO_TEST_CASE (uninit_start)
{
  BOOST_CHECK_EQUAL (failure_status_, sane_start (handle_));
}

BOOST_AUTO_TEST_CASE (uninit_read)
{
  SANE_Int  max_length = 1;
  SANE_Byte buffer[max_length];
  SANE_Int  length = -1;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_read (handle_, buffer, max_length, &length));
  BOOST_CHECK_EQUAL (0, length);
}

BOOST_AUTO_TEST_CASE (uninit_set_io_mode)
{
  BOOST_CHECK_EQUAL (failure_status_,
                     sane_set_io_mode (handle_, SANE_FALSE));
}

BOOST_AUTO_TEST_CASE (uninit_get_select_fd)
{
  SANE_Int fd = 0;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_get_select_fd (handle_, &fd));
}

BOOST_FIXTURE_TEST_SUITE_END () // uninit_backend_callability

BOOST_FIXTURE_TEST_CASE (get_devices, backend_fixture)
{
  const SANE_Device **list (NULL);
  SANE_Status status;

  status = sane_get_devices (&list, SANE_TRUE);

  BOOST_CHECK_EQUAL (SANE_STATUS_GOOD, status);
  BOOST_CHECK (list);

  int count = 0;

  while (*list)
    {
      BOOST_TEST_MESSAGE ((*list)->name);
      ++list;
      ++count;
    }
  BOOST_CHECK (0 <= count);
}

BOOST_FIXTURE_TEST_SUITE (option_count, handle_fixture)

BOOST_AUTO_TEST_CASE (option_count_descriptor)
{
  BOOST_CHECK (sane_get_option_descriptor (handle_, 0));
}

BOOST_AUTO_TEST_CASE (option_count_get_value)
{
  SANE_Int count = 0;
  SANE_Status status;

  status = sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE,
                                &count, NULL);

  BOOST_CHECK_EQUAL (SANE_STATUS_GOOD, status);
  BOOST_CHECK (0 < count);
}

BOOST_AUTO_TEST_CASE (option_count_check_read_only)
{
  SANE_Int count = 1;

  BOOST_CHECK (SANE_STATUS_GOOD !=
               sane_control_option (handle_, 0, SANE_ACTION_SET_VALUE,
                                    &count, NULL));
}

BOOST_AUTO_TEST_CASE (option_count_try_change_value)
{
  SANE_Int count = 0;
  SANE_Int value = 0;

  // FIXME?  check return value on all sane_control_option calls?

  sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE, &count, NULL);

  value = count - 1;            // try hard changing the value
  sane_control_option (handle_, 0, SANE_ACTION_SET_VALUE, &value, NULL);
  sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE, &count, NULL);

  BOOST_CHECK (count != value);
}

BOOST_AUTO_TEST_CASE (option_count_try_set_default)
{
  SANE_Int count = 1;

  BOOST_CHECK (SANE_STATUS_GOOD !=
               sane_control_option (handle_, 0, SANE_ACTION_SET_AUTO,
                                    &count, NULL));
}

BOOST_FIXTURE_TEST_SUITE_END () // option_count

BOOST_FIXTURE_TEST_SUITE (null_pointer_checking, handle_fixture)

BOOST_AUTO_TEST_CASE (null_list_get_devices)
{
  BOOST_CHECK_EQUAL (invalid_status_, sane_get_devices (NULL, SANE_TRUE));
}

BOOST_AUTO_TEST_CASE (null_device_open)
{
  SANE_Handle handle = NULL;

  BOOST_CHECK_EQUAL (SANE_STATUS_GOOD, sane_open (NULL, &handle));
}

BOOST_AUTO_TEST_CASE (null_handle_open)
{
  BOOST_CHECK_EQUAL (invalid_status_, sane_open ("", NULL));
}

BOOST_AUTO_TEST_CASE (null_handle_get_option_descriptor)
{
  BOOST_CHECK (!sane_get_option_descriptor (NULL, 0));
}

BOOST_AUTO_TEST_CASE (null_handle_control_option)
{
  SANE_Int count = 0;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_control_option (NULL, 0, SANE_ACTION_GET_VALUE,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (null_handle_get_parameters)
{
  SANE_Parameters params;

  BOOST_CHECK_EQUAL (failure_status_, sane_get_parameters (NULL, &params));
}

BOOST_AUTO_TEST_CASE (null_handle_start)
{
  BOOST_CHECK_EQUAL (failure_status_, sane_start (NULL));
}

BOOST_AUTO_TEST_CASE (null_handle_read)
{
  SANE_Int  max_length = 1;
  SANE_Byte buffer[max_length];
  SANE_Int  length = -1;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_read (NULL, buffer, max_length, &length));
  BOOST_CHECK_EQUAL (0, length);
}

BOOST_AUTO_TEST_CASE (null_handle_set_io_mode)
{
  BOOST_CHECK_EQUAL (failure_status_, sane_set_io_mode (NULL, SANE_FALSE));
}

BOOST_AUTO_TEST_CASE (null_handle_get_select_fd)
{
  SANE_Int fd;

  BOOST_CHECK_EQUAL (failure_status_, sane_get_select_fd (NULL, &fd));
}

BOOST_AUTO_TEST_CASE (null_value_get_option)
{
  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE,
                                          NULL, NULL));
}

BOOST_AUTO_TEST_CASE (null_value_set_option)
{
  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_control_option (handle_, 0, SANE_ACTION_SET_VALUE,
                                          NULL, NULL));
}

BOOST_AUTO_TEST_CASE (null_params_get_parameters)
{
  BOOST_CHECK_EQUAL (invalid_status_, sane_get_parameters (handle_, NULL));
}

BOOST_AUTO_TEST_CASE (null_buffer_read)
{
  SANE_Int    max_length =  1;
  SANE_Int    length     = -1;

  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_read (handle_, NULL, max_length, &length));
  BOOST_CHECK_EQUAL (0, length);
}

BOOST_AUTO_TEST_CASE (null_length_read)
{
  SANE_Int    max_length = 1;
  SANE_Byte   buffer[max_length];

  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_read (handle_, buffer, max_length, NULL));
}

BOOST_AUTO_TEST_CASE (null_fd_get_select_fd)
{
  BOOST_CHECK_EQUAL (invalid_status_, sane_get_select_fd (handle_, NULL));
}

BOOST_FIXTURE_TEST_SUITE_END () // null_pointer_checking

BOOST_FIXTURE_TEST_SUITE (bad_handle_checking, bad_handle_fixture)

BOOST_AUTO_TEST_CASE (bad_handle_get_option_descriptor)
{
  BOOST_CHECK (!sane_get_option_descriptor (handle_, 0));
}

BOOST_AUTO_TEST_CASE (bad_handle_get_option)
{
  SANE_Int count = 0;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (bad_handle_set_option)
{
  SANE_Int count = 0;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_control_option (handle_, 0, SANE_ACTION_SET_VALUE,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (bad_handle_set_default)
{
  SANE_Int count = 0;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_control_option (handle_, 0, SANE_ACTION_SET_AUTO,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (bad_handle_get_parameters)
{
  SANE_Parameters params;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_get_parameters (handle_, &params));
}

BOOST_AUTO_TEST_CASE (bad_handle_start)
{
  BOOST_CHECK_EQUAL (failure_status_, sane_start (handle_));
}

BOOST_AUTO_TEST_CASE (bad_handle_read)
{
  SANE_Int  max_length = 1;
  SANE_Byte buffer[max_length];
  SANE_Int  length = -1;

  BOOST_CHECK_EQUAL (failure_status_,
                     sane_read (handle_, buffer, max_length, &length));
  BOOST_CHECK_EQUAL (0, length);
}

BOOST_AUTO_TEST_CASE (bad_handle_set_io_mode)
{
  BOOST_CHECK_EQUAL (failure_status_,
                     sane_set_io_mode (handle_, SANE_FALSE));
}

BOOST_AUTO_TEST_CASE (bad_handle_get_select_fd)
{
  SANE_Int fd = 0;

  BOOST_CHECK_EQUAL (failure_status_, sane_get_select_fd (handle_, &fd));
}

BOOST_FIXTURE_TEST_SUITE_END () // bad_handle_checking

BOOST_FIXTURE_TEST_SUITE (option_bounds_checking, handle_fixture)

BOOST_AUTO_TEST_CASE (bounds_option_desc_index_negative)
{
  BOOST_CHECK (!sane_get_option_descriptor (handle_, -1));
}

BOOST_AUTO_TEST_CASE (bounds_option_desc_index_overflow)
{
  SANE_Int count = 0;
  SANE_Status status;

  status = sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE,
                                &count, NULL);

  BOOST_CHECK_EQUAL (SANE_STATUS_GOOD, status);
  BOOST_CHECK (!sane_get_option_descriptor (handle_, count));
}

BOOST_AUTO_TEST_CASE (bounds_control_option_index_negative)
{
  SANE_Int count = 0;

  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_control_option (handle_, -1, SANE_ACTION_GET_VALUE,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (bounds_control_option_index_overflow)
{
  SANE_Int count = 0;
  SANE_Status status;

  status = sane_control_option (handle_, 0, SANE_ACTION_GET_VALUE,
                                &count, NULL);

  BOOST_CHECK_EQUAL (SANE_STATUS_GOOD, status);
  BOOST_CHECK_EQUAL (SANE_STATUS_INVAL,
                     sane_control_option (handle_, count,
                                          SANE_ACTION_GET_VALUE,
                                          &count, NULL));
}

BOOST_AUTO_TEST_CASE (bounds_read_max_length_zero)
{
  SANE_Byte buffer[1];
  SANE_Int  length = -1;

  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_read (handle_, buffer, 0, &length));
  BOOST_CHECK_EQUAL (0, length);
}

BOOST_AUTO_TEST_CASE (bounds_read_max_length_negative)
{
  SANE_Byte buffer[1];
  SANE_Int  length = -1;

  BOOST_CHECK_EQUAL (invalid_status_,
                     sane_read (handle_, buffer, -1, &length));
  BOOST_CHECK_EQUAL (0, length);
}

BOOST_FIXTURE_TEST_SUITE_END () // option_bounds_checking

BOOST_FIXTURE_TEST_SUITE (api_compliance, handle_fixture)

BOOST_AUTO_TEST_CASE (api_compliance_set_io_mode)
{
  SANE_Bool non_blocking = SANE_FALSE;

  BOOST_CHECK_EQUAL   (SANE_STATUS_INVAL,
                       sane_set_io_mode (handle_, non_blocking));
  BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_start (handle_));
  BOOST_CHECK_EQUAL   (SANE_STATUS_GOOD,
                       sane_set_io_mode (handle_, non_blocking));

  SANE_Status status = sane_set_io_mode (handle_, !non_blocking);
  BOOST_CHECK (SANE_STATUS_UNSUPPORTED == status
               || SANE_STATUS_GOOD     == status);

  sane_cancel (handle_);
  BOOST_CHECK_EQUAL   (SANE_STATUS_INVAL,
                       sane_set_io_mode (handle_, non_blocking));
}

BOOST_AUTO_TEST_CASE (api_compliance_get_select_fd)
{
  const SANE_Int fd_default = 0xdeadbeef;
  SANE_Int fd (fd_default);

  BOOST_CHECK_EQUAL   (SANE_STATUS_INVAL,
                       sane_get_select_fd (handle_, &fd));
  BOOST_CHECK_EQUAL   (fd_default, fd);
  BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_start (handle_));

  SANE_Status status = sane_get_select_fd (handle_, &fd);
  if (SANE_STATUS_GOOD == status)
    {
      BOOST_CHECK (fd_default != fd);
    }
  else
    {
      BOOST_CHECK_EQUAL (SANE_STATUS_UNSUPPORTED, status);
      BOOST_CHECK_EQUAL (fd_default, fd);
    }

  sane_cancel (handle_);
  BOOST_CHECK_EQUAL   (SANE_STATUS_INVAL,
                       sane_get_select_fd (handle_, &fd));
}

BOOST_FIXTURE_TEST_SUITE_END () // api_compliance

BOOST_FIXTURE_TEST_SUITE (scan_scenarios, handle_fixture)

BOOST_AUTO_TEST_CASE (default_scan_parameters)
{
  SANE_Parameters p;
  SANE_Status status;

  memset (&p, 0xff, sizeof (p)); // invalidate all fields

  status = sane_get_parameters (handle_, &p);

  BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, status);
  BOOST_CHECK_MESSAGE (   (SANE_FRAME_GRAY  == p.format)
                       || (SANE_FRAME_RGB   == p.format)
                       || (SANE_FRAME_RED   == p.format)
                       || (SANE_FRAME_GREEN == p.format)
                       || (SANE_FRAME_BLUE  == p.format)
                       , "invalid frame: " << p.format);
  BOOST_CHECK (-1 <= p.lines);
  BOOST_CHECK ( 0 <  p.depth);
  BOOST_CHECK ( 0 <= p.pixels_per_line);

  SANE_Int samples = (SANE_FRAME_RGB == p.format ? 3 : 1) * p.pixels_per_line;
  SANE_Int minimum = (1 == p.depth
                      ? samples / 8
                      : samples * ((p.depth + 7) / 8));

  BOOST_CHECK (minimum <= p.bytes_per_line);
}

BOOST_AUTO_TEST_CASE (default_scan)
{
  SANE_Parameters p;
  SANE_Status status = SANE_STATUS_GOOD;

  BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_start (handle_));
  BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_get_parameters (handle_, &p));
  BOOST_REQUIRE (0 < p.pixels_per_line);

  SANE_Int  max_length = p.pixels_per_line;
  SANE_Byte buffer[max_length];
  SANE_Int  length = 0;
  SANE_Int  bytes_read = 0;

  while (SANE_STATUS_GOOD == status)
    {
      length = -1;
      status = sane_read (handle_, buffer, max_length, &length);

      if (SANE_STATUS_GOOD == status)
        {
          BOOST_WARN (0 < length);
          bytes_read += length;
        }
      else
        {
          BOOST_CHECK_EQUAL (0, length);
        }
    }

  if (SANE_STATUS_EOF == status)
    {
      if (-1 < p.lines)
        {
          BOOST_CHECK_EQUAL (p.lines * p.bytes_per_line, bytes_read);
        }
      else
        {
          SANE_Int lines_read = bytes_read / p.bytes_per_line;
          BOOST_CHECK_EQUAL (lines_read * p.bytes_per_line, bytes_read);
        }
    }
  else
    {
      BOOST_FAIL ("failed to read a complete image");
    }

  sane_cancel (handle_);
}

BOOST_AUTO_TEST_CASE (triple_start)
{
  BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_start (handle_));
  BOOST_CHECK_EQUAL   (SANE_STATUS_DEVICE_BUSY, sane_start (handle_));
  BOOST_CHECK_EQUAL   (SANE_STATUS_DEVICE_BUSY, sane_start (handle_));
}

BOOST_AUTO_TEST_CASE (triple_scan)
{
  for (int i = 0; i < 3; ++i)
    {
      SANE_Parameters p;
      SANE_Status status = SANE_STATUS_GOOD;

      BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_start (handle_));
      BOOST_REQUIRE_EQUAL (SANE_STATUS_GOOD, sane_get_parameters (handle_, &p));
      BOOST_REQUIRE (0 < p.pixels_per_line);

      SANE_Int  max_length = p.pixels_per_line;
      SANE_Byte buffer[max_length];
      SANE_Int  length = 0;
      SANE_Int  bytes_read = 0;

      while (SANE_STATUS_GOOD == status)
        {
          length = -1;
          status = sane_read (handle_, buffer, max_length, &length);

          if (SANE_STATUS_GOOD == status)
            {
              BOOST_WARN (0 < length);
              bytes_read += length;
            }
          else
            {
              BOOST_CHECK_EQUAL (0, length);
            }
        }

      if (SANE_STATUS_EOF == status)
        {
          if (-1 < p.lines)
            {
              BOOST_CHECK_EQUAL (p.lines * p.bytes_per_line, bytes_read);
            }
          else
            {
              SANE_Int lines_read = bytes_read / p.bytes_per_line;
              BOOST_CHECK_EQUAL (lines_read * p.bytes_per_line, bytes_read);
            }
        }
      else
        {
          BOOST_FAIL ("failed to read a complete image");
        }

      sane_cancel (handle_);
    }
}

BOOST_FIXTURE_TEST_SUITE_END () // scan_scenarios

// Prevent duplicate definition of init_test_runner()

#ifndef BOOST_PARAM_TEST_CASE
#define BOOST_PARAM_TEST_CASE
#endif

static void
drop_test_suite (const std::string& name)
{
  namespace but = boost::unit_test;

  but::master_test_suite_t& master (but::framework::master_test_suite ());
  but::test_case_counter tcc;
  but::test_unit_id tuid;

  tuid = master.get (name);

  but::traverse_test_tree (tuid, tcc);

  master.remove (tuid);
  BOOST_MESSAGE ("Disabled \"" << name << "\" test suite for lack of "
                 "a mock device (" << tcc.p_count << " test cases)");
}

bool
init_test_runner ()
{
  namespace but = boost::unit_test;

  but::master_test_suite_t& master (but::framework::master_test_suite ());

  BOOST_MESSAGE ("Initializing \"" << master.p_name << "\" test suite");

  utsushi::monitor mon;    // to discover devices

  if (mon.begin () == mon.end ()
      || "mock" != mon.begin ()->driver ())
    {
      drop_test_suite ("option_count");
      drop_test_suite ("null_pointer_checking");
      drop_test_suite ("option_bounds_checking");
      drop_test_suite ("api_compliance");
      drop_test_suite ("scan_scenarios");
    }
  return true;
}

#include "utsushi/test/runner.ipp"
