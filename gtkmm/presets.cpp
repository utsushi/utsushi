//  presets.cpp -- for scanner device selection and maintenance actions
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
#include <list>

#include <utsushi/run-time.hpp>

#include "presets.hpp"

namespace utsushi {
namespace gtkmm {

//! \todo Get preset definitions from external file(s)
presets::presets (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr, builder)
{
  std::list< preset > presets;
  presets.push_back (preset ("Office Documents"));
  presets.push_back (preset ("Multi-page Duplex to PDF"));
  presets.push_back (preset ("Invoices"));
  presets.push_back (preset ("Newspaper Articles"));
  presets.push_back (preset ("Share Photos",
                             "Upload photos to Flickr account"));
  presets.push_back (preset ("Archive Photos"));
  presets.push_back (preset ("Mounted Positives"));
  presets.push_back (preset ("Negative Strips (35mm)"));

  std::for_each (presets.begin (), presets.end (),
                 sigc::mem_fun (*this, &presets::insert_custom));
  insert_actions (builder, "presets-actions");
  insert_separators ();

  if (is_sensitive ())
    set_active (0);
  show_all ();
}

void
presets::insert_preset (type_id type, const preset& preset)
{
  insert (type, preset.name (), preset.text ());
}

void
presets::insert_custom (const preset& preset)
{
  insert_preset (CUSTOM, preset);
}

void
presets::insert_system (const preset& preset)
{
  insert_preset (SYSTEM, preset);
}

}       // namespace gtkmm
}       // namespace utsushi
