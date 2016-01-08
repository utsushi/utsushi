//  main.cpp -- entry point to all applications and utilities
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
#include <sstream>
#include <vector>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/run-time.hpp>

int
main (int argc, char *argv[])
{
  namespace fs = boost::filesystem;
  namespace po = boost::program_options;

  using std::runtime_error;

  using boost::lambda::constant;
  using boost::lambda::_1;

  const std::string default_command ("scan");

  try
    {
      using utsushi::_;

      utsushi::run_time rt (argc, argv, utsushi::i18n);

      po::options_description cli_cmds (CCB_("Supported commands"));
      cli_cmds
        .add_options ()
        ("help"   , CCB_("display the help for a command and exit"))
        ("version", CCB_("output command version information and exit"))
        ("list"   , CCB_("list available image acquisition devices"))
        ("scan"   , CCB_("scan with a suitable utility"))
        ;

      if (rt.count ("help"))
        {
          std::cout << rt.help (CCB_("next generation image acquisition"));

          // Kludge to remove leading dashes from the commands
          std::stringstream ss;
          ss << cli_cmds;
          std::string cmd_help (ss.str ());
          std::string::size_type i = cmd_help.find ("  --");
          while (std::string::npos != i)
            {
              cmd_help.erase (i+2, 2);
              i = cmd_help.find ("  --", i);
            }

          std::cout << "\n"
                    << cmd_help;

          return EXIT_SUCCESS;
        }
      if (rt.count ("version"))
        {
          std::cout << rt.version ();
          return EXIT_SUCCESS;
        }

      std::string cmd (rt.command ().empty ()
                       ? default_command
                       : rt.command ());

      if (!cli_cmds.find_nothrow (cmd, false))
        {
          BOOST_THROW_EXCEPTION (runtime_error (cmd));
        }

      std::vector< std::string > args (rt.arguments ());

      if (("version" == cmd)
          || ("help" == cmd))
        {
          if (!args.empty ())
            {
              std::string tmp (args.front ());
              args.front () = "--" + cmd;
              cmd = tmp;
            }
        }
      cmd = rt.locate (cmd);

      std::for_each (args.begin (), args.end (),
                     cmd += constant (" \"") + _1 + "\"");
      rt.execute (cmd);
    }
  catch (const boost::exception& e)
    {
      std::cerr << boost::diagnostic_information (e);
      return EXIT_FAILURE;
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

  return EXIT_SUCCESS;
}
