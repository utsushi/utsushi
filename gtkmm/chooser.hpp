//  chooser.hpp -- for scanner device selection and maintenance actions
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

#ifndef gtkmm_chooser_hpp_
#define gtkmm_chooser_hpp_

#include <set>
#include <string>

#include <gtkmm/builder.h>

#include <utsushi/scanner.hpp>

#include "dropdown.hpp"

namespace utsushi {
namespace gtkmm {

class chooser : public dropdown
{
  typedef dropdown base;

  std::set<scanner::info> custom_;
  std::set<scanner::info> system_;

public:
  chooser (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder);

  sigc::signal<void, scanner::ptr>
  signal_device_changed ();

protected:
  void on_run ();
  void on_changed ();
  void on_custom (const std::string& udi);
  void on_system (const std::string& udi);

private:
  void create_device (const std::set<scanner::info>& devices,
                      const std::string& udi);

  void insert_device (type_id type, const scanner::info& device);
  void insert_custom (const scanner::info& device);
  void insert_system (const scanner::info& device);

  sigc::signal<void, scanner::ptr>
  signal_device_changed_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_chooser_hpp_ */
