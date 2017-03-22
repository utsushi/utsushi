//  chooser.cpp -- for scanner device selection and maintenance actions
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

#include <algorithm>

#include <boost/throw_exception.hpp>

#include <gdkmm/cursor.h>
#include <gdkmm/general.h>

#include <gtkmm/main.h>

#include <utsushi/format.hpp>
#include <utsushi/log.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/monitor.hpp>

#include "chooser.hpp"

namespace utsushi {
namespace gtkmm {

chooser::chooser (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr, builder, false)
{
  monitor mon;
  monitor::const_iterator it = mon.begin ();

  // FIXME list devices w/o driver but prevent their selection
  while (mon.end () != it) {
    if (it->is_driver_set ())
      {
        system_.insert (*it);
      }
    ++it;
  }

  std::for_each (custom_.begin (), custom_.end (),
                 sigc::mem_fun (*this, &chooser::insert_custom));
  std::for_each (system_.begin (), system_.end (),
                 sigc::mem_fun (*this, &chooser::insert_system));

  // FIXME: MESSAGE items should not be selectable
  /**/ if (0 == custom_.size () + system_.size ())
    {
      Gtk::TreeRow row = *(model_->prepend ());

      row[cols_->type] = MESSAGE;
      row[cols_->name] = SEC_("No devices found");
    }
  else if (2 <= custom_.size () + system_.size ())
    {
      Gtk::TreeRow row = *(model_->prepend ());

      row[cols_->type] = MESSAGE;
      row[cols_->name] = SEC_("Select a device");
    }

  insert_actions (builder, "chooser-actions");
  insert_separators ();

  show_all ();

  // Postpone device creation until the GUI has had a chance to show
  // itself.  This allows for feedback to the user during long waits
  // in the device creation process.

  Gtk::Main::signal_run ().connect (sigc::mem_fun (this, &chooser::on_run));
}

sigc::signal<void, scanner::ptr>
chooser::signal_device_changed ()
{
  return signal_device_changed_;
}

void
chooser::on_run ()
{
  set_active (0);
  cache_ = get_active ();
}

void
chooser::on_changed ()
{
  if (inhibit_callback_) return;

  const std::string& udi = get_active ()->get_value (cols_->udi);
  type_id type = get_active ()->get_value (cols_->type);

  if (cache_ && udi == cache_->get_value (cols_->udi)) return;

  /**/ if (ACTION  == type) dropdown::on_changed ();
  else if (CUSTOM  == type) on_custom (udi);
  else if (SYSTEM  == type) on_system (udi);
  else if (MESSAGE == type)
    {
      inhibit_callback_ = true;
      if (cache_) set_active (cache_);
      inhibit_callback_ = false;
    }
  else
    {
      // FIXME log unsupported type error
    }
}

void
chooser::on_system (const std::string& udi)
{
  create_device (system_, udi);
}

void
chooser::on_custom (const std::string& udi)
{
  create_device (custom_, udi);
}

void
chooser::create_device (const std::set<scanner::info>& devices,
                        const std::string& udi)
{
  std::set<scanner::info>::const_iterator it = devices.begin ();
  while (devices.end () != it && udi != it->udi ()) {
    ++it;
  }
  if (devices.end () != it) {

    Glib::RefPtr< Gdk::Window > window = get_window ();

    if (window)
      {
        window->set_cursor (Gdk::Cursor (Gdk::WATCH));
        Gdk::flush ();
      }

    scanner::ptr ptr;
    std::string  why;
    try
      {
        // FIXME This is a bit clunky but scanner creation may be time
        //       consuming and cannot be put in a separate thread if
        //       the scanner object is run via process separation.
        //       The child process would exit at thread end.

        while (Gtk::Main::events_pending ())
          Gtk::Main::iteration ();

        ptr = scanner::create (*it);
      }
    catch (const std::exception& e)
      {
        why = e.what ();
      }
    catch (...)
      {
        // FIXME set a why we failed to create a device
      }

    if (window)
      {
        window->set_cursor ();
      }

    if (ptr)
      {
        cache_ = get_active ();
        set_tooltip_text (it->udi ());
        signal_device_changed_.emit (ptr);
      }
    else
      {
        const std::string& name = get_active ()->get_value (cols_->name);
        const std::string& udi  = get_active ()->get_value (cols_->udi);

        inhibit_callback_ = true;
        if (cache_) set_active (cache_);
        inhibit_callback_ = false;

        BOOST_THROW_EXCEPTION
          (std::runtime_error
           ((format (SEC_("Cannot access %1%\n(%2%)\n%3%"))
             % name
             % udi
             % _(why)
             ).str ()));
      }
  }
}

void
chooser::insert_device (type_id type, const scanner::info& device)
{
  insert (type, device.name (), device.text (), device.udi ());
}

void
chooser::insert_custom (const scanner::info& device)
{
  insert_device (CUSTOM, device);
}

void
chooser::insert_system (const scanner::info& device)
{
  insert_device (SYSTEM, device);
}

}       // namespace gtkmm
}       // namespace utsushi
