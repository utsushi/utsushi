//  scan-gtkmm.cpp -- graphical user interface based scan utility
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

#include <iostream>

#include <boost/program_options.hpp>

#include <gtkmm/builder.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/rc.h>

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/run-time.hpp>

#include "../gtkmm/dialog.hpp"

namespace bpo = boost::program_options;
using namespace utsushi;

void
catch_and_return_to_GUI_thread (void)
{
  try
    {
      throw;
    }
  catch (const std::exception& e)
    {
      Gtk::MessageDialog dialog (e.what (), false, Gtk::MESSAGE_ERROR);

      dialog.set_keep_above ();
      dialog.run ();
    }
}

int
main (int argc, char *argv[])
{
  try
    {
      run_time rt (argc, argv, i18n);

      bpo::variables_map vm;

      std::string gui_file = "gtkmm/dialog.glade";
      std::string rsc_file = "gtkmm/dialog.rc";

      bpo::options_description opts (CCB_("Utility options"));
      opts
        .add_options ()
        ("layout"  , bpo::value<std::string>(&gui_file),
         CCB_("use an alternative GUI layout definition file"))
        ("resource", bpo::value<std::string>(&rsc_file),
         CCB_("use an alternative GUI resource file"))
        ;

      if (rt.count ("help"))
        {
          std::cout << "\n"
                    << opts
                    << "\n";

          return EXIT_SUCCESS;
        }

      if (rt.count ("version"))
        {
          std::cout << rt.version ();
          return EXIT_SUCCESS;
        }

      bpo::store (bpo::parse_command_line (argc, argv, opts), vm);
      bpo::notify (vm);

      Gtk::Main kit (argc, argv);

      Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create ();
      try
        {
          run_time rt;

          Gtk::RC::add_default_file (rt.data_file (run_time::pkg, rsc_file));
          builder->add_from_file    (rt.data_file (run_time::pkg, gui_file));
        }
      catch (const Glib::Error& e)
        {
          std::cerr << e.what () << std::endl;
          return EXIT_FAILURE;
        }

      utsushi::gtkmm::dialog *window = 0;
      builder->get_widget_derived ("scanning-dialog", window);

      if (window)
        {
          Glib::add_exception_handler (&catch_and_return_to_GUI_thread);

          kit.run (*window);
          delete window;
        }
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
