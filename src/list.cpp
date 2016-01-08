//  list.cpp -- available image acquisition devices
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

#include <exception>
#include <iostream>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/monitor.hpp>
#include <utsushi/run-time.hpp>

int
main (int argc, char *argv[])
{
  using boost::lambda::_1;

  try
    {
      using utsushi::_;

      utsushi::run_time rt (argc, argv, utsushi::i18n);

      if (rt.count ("help"))
        {
          std::cout << rt.help
            (CCB_("list available image acquisition devices"));
          return EXIT_SUCCESS;
        }
      if (rt.count ("version"))
        {
          std::cout << rt.version ();
          return EXIT_SUCCESS;
        }

      utsushi::monitor mon;

      std::for_each (mon.begin (), mon.end (),
                     std::cout
                     << boost::lambda::bind (&utsushi::scanner::info::udi, _1) << "\n");
    }
  catch (std::exception& e)
    {
      std::cerr << e.what () << "\n";
      return EXIT_FAILURE;
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }

  exit (EXIT_SUCCESS);
}
