//  dropdown.hpp -- menu with three sections
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

#ifndef gtkmm_dropdown_hpp_
#define gtkmm_dropdown_hpp_

#include <gtkmm/builder.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

namespace utsushi {
namespace gtkmm {

class dropdown : public Gtk::ComboBox
{
  typedef Gtk::ComboBox base;

public:
  dropdown (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder,
            bool inhibit_callback = true);

protected:

  typedef enum {
    CUSTOM,
    SYSTEM,
    ACTION,
    MESSAGE,
  } type_id;

  void insert (type_id type,
               const std::string& name,
               const std::string& text = std::string (),
               const std::string& udi  = std::string ());

  template <typename iterT>
  void insert (type_id type, iterT first, iterT last)
  {
    while (first != last) {
      insert (type, *first);
      ++first;
    }
  }

  void insert_actions (Glib::RefPtr<Gtk::Builder>& builder,
                       const Glib::ustring& path);
  void insert_separators ();

  virtual void on_changed ();
  virtual void on_custom (const std::string& name);
  virtual void on_system (const std::string& name);
  virtual void on_action (const std::string& name);

private:
  bool is_separator (const Glib::RefPtr<Gtk::TreeModel>& model,
                     const Gtk::TreeModel::iterator& it);

protected:
  Glib::RefPtr<Gtk::ListStore> model_;
  Gtk::TreeModel::iterator     cache_;

  bool          inhibit_callback_;
  Glib::ustring cache_name_;

  // FIXME UDI does not belong here, that's a chooser responsibility
  struct column_record : public Gtk::TreeModel::ColumnRecord
  {
    Gtk::TreeModelColumn<type_id>       type;
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> text;
    Gtk::TreeModelColumn<Glib::ustring> udi;

    column_record ()
    {
      add (type);
      add (name);
      add (text);
      add (udi);
    }
  };

  static const column_record *cols_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_dropdown_hpp_ */
