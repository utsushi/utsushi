//  scan.cpp -- with a suitable utility
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
#include <utility>

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/program_options.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/run-time.hpp>

static std::pair< std::string, std::string >
negating_prefix (const std::string& option)
{
  if ("--no-interface" == option)
    {
      return std::make_pair ("interface", std::string ("false"));
    }
  if ("--interface" == option)
    {
      return std::make_pair ("interface", std::string ("true"));
    }
  return std::make_pair (std::string (), std::string ());
}

int
main (int argc, char *argv[])
{
  namespace po = boost::program_options;
  namespace fs = boost::filesystem;

  using boost::lambda::constant;
  using boost::lambda::_1;
  using utsushi::_;

  const std::string cli_scan_utility ("scan-cli");
  const std::string gui_scan_utility ("scan-gtkmm");

  const std::string fallback_scan_utility (cli_scan_utility);

  try
    {
      using utsushi::_;

      utsushi::run_time rt (argc, argv, utsushi::i18n);

      bool interface (getenv ("DISPLAY"));

      po::variables_map cmd_vm;
      po::options_description cmd_opts (CCB_("Command options"));
      cmd_opts
        .add_options ()
        ("interface", (po::value< bool > (&interface)
                       -> default_value (interface)),
         CCB_("Start an interactive user interface\n"
              "The default behavior depends on the environment where one runs"
              " the command.  A scan utility suitable for non-interactive use"
              " can be selected with the '--no-interface' option."))
        ;

      if (rt.count ("help"))
        {
          std::cout << rt.help
            (CCB_("acquire images with a suitable utility"))
                    << "\n"
                    << cmd_opts;
        }
      if (rt.count ("version"))
        {
          // Never mind our own version
        }

      po::parsed_options opts (po::command_line_parser (rt.arguments ())
                               .options (cmd_opts)
                               .extra_parser (negating_prefix)
                               .allow_unregistered ()
                               .run ());
      po::store (opts, cmd_vm);
      po::notify (cmd_vm);

      std::vector< std::string > utility_opts
        (po::collect_unrecognized (opts.options, po::include_positional));

      std::string cmd (interface
                       ? rt.locate (gui_scan_utility)
                       : rt.locate (cli_scan_utility));

      if (cmd.empty ())
        {
          cmd = rt.locate (fallback_scan_utility);
          if (cmd.empty ())
            {
              // FIXME Now what!?
            }
        }

      if (rt.count ("help"))    cmd += " --help";
      if (rt.count ("version")) cmd += " --version";

      std::for_each (utility_opts.begin (), utility_opts.end (),
                     cmd += constant (" \"") + _1 + "\"");
      rt.execute (cmd);
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
