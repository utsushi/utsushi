//  action-dialog.hpp -- controls to trigger device maintenance
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef gtkmm_action_dialog_hpp_
#define gtkmm_action_dialog_hpp_

#include <utsushi/option.hpp>
#include <utsushi/thread.hpp>

#include <gtkmm/dialog.h>

namespace utsushi {
namespace gtkmm {

class action_dialog
  : public Gtk::Dialog
{
public:
  action_dialog (option::map::ptr actions, Gtk::Widget *trigger,
                 bool use_spinner = false);
  ~action_dialog ();

  void on_action (Gtk::Button *button, std::string key,
                  std::string message);
  void on_maintenance (void);

private:
  option::map::ptr actions_;
  Gtk::Widget     *trigger_;
  Gtk::ButtonBox  *buttons_;
  thread          *process_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_action_dialog_hpp_ */
