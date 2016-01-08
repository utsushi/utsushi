//  version.cpp -- output command version information and exit
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

#include <cstdlib>

#include <exception>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/run-time.hpp>

int
main (int argc, char *argv[])
{
  namespace fs = boost::filesystem;

  const std::string default_command ("main");

  try
    {
      using utsushi::_;

      utsushi::run_time rt (argc, argv, utsushi::i18n);

      if (rt.count ("help"))
        {
          std::cout << rt.help
            (CCB_("display version information for a command"));
          return EXIT_SUCCESS;
        }
      if (rt.count ("version"))
        {
          std::cout << rt.version ();
          return EXIT_SUCCESS;
        }

      std::string cmd (rt.arguments ().empty ()
                       ? default_command
                       : rt.arguments ().front ());

      rt.execute (rt.locate (cmd) + " --version");
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
