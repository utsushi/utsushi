//  file-chooser.hpp -- select where and how to save scan results
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

#ifndef gtkmm_file_chooser_hpp_
#define gtkmm_file_chooser_hpp_

#include <gtkmm/dialog.h>

#include <gtkmm/checkbutton.h>
#include <gtkmm/expander.h>
#include <gtkmm/filechooserwidget.h>
#include <gtkmm/treeview.h>
#include <glibmm/dispatcher.h>

#include <utsushi/mutex.hpp>
#include <utsushi/thread.hpp>
#include <csignal>
#include <queue>
#include <string>

namespace utsushi {
namespace gtkmm {

class file_chooser
  : public Gtk::Dialog
{
public:
  file_chooser (Gtk::Window& parent, const std::string& title);
  file_chooser (const std::string& title);

  ~file_chooser ();

  bool get_do_overwrite_confirmation () const;
  void set_do_overwrite_confirmation (bool do_overwrite_confirmation = true);

  std::string get_current_name () const;
  void        set_current_name (const std::string& name);
  std::string get_current_extension () const;
  void        set_current_extension (const std::string& extension);

  std::string get_filename () const;
  bool        set_filename (const std::string& filename);
  std::string get_current_folder () const;
  bool        set_current_folder (const std::string& filename);

  void add_filter    (const Gtk::FileFilter& filter);
  void remove_filter (const Gtk::FileFilter& filter);

  bool get_single_image_mode () const;
  void set_single_image_mode (bool single_image_mode = true);

  void show_all ();

protected:
  virtual void on_response (int response_id);

  virtual void on_file_type_changed ();
  virtual void on_single_file_toggled ();

private:
  void common_ctor_logic_();

  bool do_overwrite_confirmation_;
  bool single_image_mode_;

  Gtk::Expander    expander_;
  Gtk::TreeView    file_type_;
  Gtk::CheckButton single_file_;

  Gtk::FileChooserWidget impl_;

  // The Gtk::FileChooserWidget does not provide signals for filename
  // changes (nor does it expose the Gtk::Entry filename widget).  As
  // a result, keeping file_type_, single_file_ and filename views in
  // a mutually consistent state becomes not trivial.
  // We use a separate thread that watches the dialog's current name.
  // In case of change, it triggers an event in the GUI thread.

  void watch_();

  sig_atomic_t  cancel_watch_;
  thread       *watch_thread_;
  std::string   cached_name_;
  mutable mutex filename_mutex_;

  // Infra-structure to transfer signals safely from the watch_()
  // thread to the GUI thread.  This uses a Glib::Dispatcher in a
  // fashion similar to what gtkmm::pump does.

  typedef sigc::signal< void, const std::string& > name_change_signal_type;
  name_change_signal_type signal_name_change ();

  void signal_name_change_();
  void on_name_change_(const std::string& name);

  Glib::Dispatcher          gui_name_change_dispatch_;
  name_change_signal_type   gui_name_change_signal_;
  std::queue< std::string > name_change_queue_;
  mutex                     name_change_mutex_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_file_chooser_hpp_ */
