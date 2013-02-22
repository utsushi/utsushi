//  chooser.cpp -- for scanner device selection and maintenance actions
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#include <utsushi/connexion.hpp>
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
    if (it->has_driver ())
      {
        if (it->is_configured ()) {
          custom_.insert (*it);
        } else {
          system_.insert (*it);
        }
      }
    ++it;
  }

  std::for_each (custom_.begin (), custom_.end (),
                 sigc::mem_fun (*this, &chooser::insert_custom));
  std::for_each (system_.begin (), system_.end (),
                 sigc::mem_fun (*this, &chooser::insert_system));
  insert_actions (builder, "chooser-actions");
  insert_separators ();

  show_all ();
}

sigc::signal<void, scanner::ptr>
chooser::signal_device_changed ()
{
  return signal_device_changed_;
}

void
chooser::on_system (const std::string& name)
{
  create_device (system_, name);
}

void
chooser::on_custom (const std::string& name)
{
  create_device (custom_, name);
}

void
chooser::create_device (const std::set<scanner::id>& devices,
                        const std::string& name)
{
  std::set<scanner::id>::const_iterator it = devices.begin ();
  while (devices.end () != it && name != it->name ()) {
    ++it;
  }
  if (devices.end () != it) {
    connexion::ptr cnx (connexion::create (it->iftype (), it->path ()));
    signal_device_changed_.emit (scanner::create (cnx, *it));
  }
}

void
chooser::insert_device (type_id type, const scanner::id& device)
{
  insert (type, device.name (), device.text ());
}

void
chooser::insert_custom (const scanner::id& device)
{
  insert_device (CUSTOM, device);
}

void
chooser::insert_system (const scanner::id& device)
{
  insert_device (SYSTEM, device);
}

}       // namespace gtkmm
}       // namespace utsushi
