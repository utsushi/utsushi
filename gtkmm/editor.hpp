//  editor.hpp -- scanning dialog's option value editor
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

#ifndef gtkmm_editor_hpp_
#define gtkmm_editor_hpp_

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>
#include <sigc++/connection.h>

#include <utsushi/format.hpp>
#include <utsushi/option.hpp>
#include <utsushi/scanner.hpp>

namespace utsushi {
namespace gtkmm {

class editor : public Gtk::VBox
{
public:
  typedef std::map< key, Gtk::Widget * >    widget_map;
  typedef std::map< key, sigc::connection > signal_map;

private:
  typedef Gtk::VBox base;

  typedef std::map< key, Gtk::ToggleButton * > toggle_map;
  typedef std::pair< key, Gtk::Widget * > keyed_widget;
  typedef std::vector< keyed_widget >     keyed_list;
  typedef std::set< key > tag_set;

  Gtk::Table  *matrix_;
  Gtk::VBox   *editor_;

  Glib::RefPtr< Gtk::SizeGroup > hgroup_, vgroup_;

  toggle_map toggles_;
  keyed_list editors_;
  widget_map controls_;
  signal_map connects_;

  // Connect each option key to a group key
  std::map< key, key > group_;

  option::map::ptr opts_;

  key    app_key_;
  format app_name_;
  format app_desc_;

public:
  editor (BaseObjectType *ptr, Glib::RefPtr< Gtk::Builder >& builder);

  void add_widget (option& opt);

  void set_application_name (const std::string& name);
  void on_options_changed (option::map::ptr om,
                           const std::set<std::string>& option_blacklist);

  void set (const std::string& key, const value& v);

  //! Retrieve first string that matches in translation
  /*! Option constraint checking uses \e untranslated string values.
   *  The controls_, however, uses translated strings if these are
   *  available.  These cannot be assigned directly as they would
   *  needs satisfy any contraint.  This function returns the first
   *  untranslated string value or \a s if there is none.
   *
   *  \todo  Make the individual controls_ responsible for this.
   */
  string untranslate (const key &k, const string &s);

  sigc::signal<void, option::map::ptr>
  signal_values_changed ();

protected:
  bool block_on_toggled_;
  void on_toggled ();
  void set_toggles_sensitive (const tag_set& tags);

  //! Adjust the way an option editor widget is displayed
  void update_appearance (keyed_list::value_type& v);

  //! Whether any of the \a tags matches an active toggle
  bool active_toggle_(const std::set< key >& tags) const;

private:
  sigc::signal<void, option::map::ptr>
  signal_values_changed_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_editor_hpp_ */
