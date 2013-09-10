//  dialog.hpp -- to acquire image data
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#ifndef gtkmm_dialog_hpp_
#define gtkmm_dialog_hpp_

#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/uimanager.h>
#include <sigc++/connection.h>

#include <utsushi/device.hpp>

#include "../filters/jpeg.hpp"

#include "pump.hpp"

namespace utsushi {
namespace gtkmm {

class editor;

class dialog : public Gtk::Dialog
{
  typedef Gtk::Dialog base;

  Glib::RefPtr<Gtk::UIManager> ui_manager_;

  Gtk::Widget *dialog_;
  editor      *editor_;

  Gtk::ToggleButton *expand_;
  sigc::connection   cancel_;

  idevice::ptr idevice_;
  pump::ptr pump_;

  option::map::ptr opts_;
  option::map::ptr app_opts_;

  _flt_::jpeg::compressor jpeg_;

public:
  dialog (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder);

  sigc::signal< void, option::map::ptr >
  signal_options_changed ();

protected:
  void set_sensitive (void);

  void on_detail_toggled (void);
  void on_scan (void);
  void on_scan_update (traits::int_type c);
  void on_about (void);

  void on_device_changed (idevice::ptr idev);
  void on_notify (log::priority level, std::string message);

  sigc::signal< void, option::map::ptr >
  signal_options_changed_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_dialog_hpp_ */
