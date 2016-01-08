//  sane-api.ipp -- callability unit test source code template
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

BOOST_AUTO_TEST_CASE (SANE_API_TEST (init))
{
  SANE_API_CALL (init) (NULL, NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (exit))
{
  SANE_API_CALL (exit) ();
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (get_devices))
{
  SANE_API_CALL (get_devices) (NULL, SANE_FALSE);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (open))
{
  SANE_API_CALL (open) (NULL, NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (close))
{
  SANE_API_CALL (close) (NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (get_option_descriptor))
{
  SANE_API_CALL (get_option_descriptor) (NULL, 0);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (control_option))
{
  SANE_API_CALL (control_option) (NULL, 0, SANE_ACTION_GET_VALUE, NULL, NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (get_parameters))
{
  SANE_API_CALL (get_parameters) (NULL, NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (start))
{
  SANE_API_CALL (start) (NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (read))
{
  SANE_API_CALL (read) (NULL, NULL, 0, NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (cancel))
{
  SANE_API_CALL (cancel) (NULL);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (set_io_mode))
{
  SANE_API_CALL (set_io_mode) (NULL, SANE_FALSE);
}

BOOST_AUTO_TEST_CASE (SANE_API_TEST (get_select_fd))
{
  SANE_API_CALL (get_select_fd) (NULL, NULL);
}
