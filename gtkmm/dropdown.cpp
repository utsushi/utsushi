//  dropdown.cpp -- menu with three sections
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

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include <gtkmm/messagedialog.h>
#include <gtkmm/uimanager.h>

#include <utsushi/format.hpp>
#include <utsushi/i18n.hpp>

#include "dropdown.hpp"

namespace utsushi {
namespace gtkmm {

using std::logic_error;

static const char *separator_ = "-----";

const dropdown::column_record *dropdown::cols_ = 0;

dropdown::dropdown (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder,
                    bool inhibit_callback)
  : base (ptr), inhibit_callback_(inhibit_callback)
{
  if (!cols_) {
    cols_ = new column_record ();
  }

  model_ = Gtk::ListStore::create (*cols_);
  set_model (model_);
  pack_start (cols_->name);

  set_row_separator_func (sigc::mem_fun (*this, &dropdown::is_separator));
}

void
dropdown::insert_actions (Glib::RefPtr<Gtk::Builder>& builder,
                          const Glib::ustring& path)
{
  Glib::RefPtr<Glib::Object> obj = builder->get_object ("uimanager");
  Glib::RefPtr<Gtk::UIManager> ui_manager
    = Glib::RefPtr<Gtk::UIManager>::cast_dynamic (obj);

  if (!ui_manager)
    BOOST_THROW_EXCEPTION
      (logic_error ("Dialog specification requires a 'uimanager'"));

  Glib::ListHandle< Glib::RefPtr<Gtk::ActionGroup> >::const_iterator
    it     = ui_manager->get_action_groups ().begin (),
    it_end = ui_manager->get_action_groups ().end ();

  while (it_end != it && path != (*it)->get_name ()) {
    ++ it;
  }
  if (it_end != it) {
    std::list< Glib::RefPtr<const Gtk::Action> > al = (*it)->get_actions ();
    std::list< Glib::RefPtr<const Gtk::Action> >::const_iterator jt;

    for (jt = al.begin (); al.end () != jt; ++jt) {
      insert (ACTION, (*jt)->property_label ().get_value ());
    }
  }
}

void
dropdown::insert_separators ()
{
  Gtk::TreeModel::iterator it = model_->children ().begin ();

  if (model_->children ().end () != it)
    {
      type_id type = (*it)[cols_->type];
      ++it;

      while (model_->children ().end () != it)
        {
          if (type != (*it)[cols_->type])
            {
              Gtk::TreeRow row = *(model_->insert (it));

              row[cols_->name] = separator_;
              type = (*it)[cols_->type];
            }
          ++it;
        }
    }
}

bool
dropdown::is_separator (const Glib::RefPtr<Gtk::TreeModel>& model,
                        const Gtk::TreeModel::iterator& it)
{
  return separator_ == (*it)[cols_->name];
}

void
dropdown::insert (type_id type,
                  const std::string& name, const std::string& text,
                  const std::string& udi)
{
  Gtk::TreeRow row = *(model_->append ());

  row[cols_->type] = type;
  row[cols_->name] = name;
  if (!text.empty ()) {
    row[cols_->text] = text;
  }
  if (!udi.empty ()) {
    row[cols_->udi] = udi;
  }
}

void
dropdown::on_changed ()
{
  const std::string& name = get_active ()->get_value (cols_->name);

  type_id type = get_active ()->get_value (cols_->type);
  if (cache_) cache_name_ = cache_->get_value (cols_->name);

  if (ACTION != type) {
    cache_ = get_active ();
    if (!inhibit_callback_) {
     if (CUSTOM == type) {
       on_custom (name);
     } else if (SYSTEM == type) {
       on_system (name);
     } else {
       // :FIXME: log error
     }
    }
    inhibit_callback_ = false;
  } else {
    on_action (name);
    inhibit_callback_ = true;
    if (cache_) {
      set_active (cache_);
    }
  }
}

void
dropdown::on_custom (const std::string& name)
{
  Gtk::MessageDialog tbd (CCB_("To be implemented."),
                          false, Gtk::MESSAGE_WARNING);
  tbd.set_secondary_text
    ((format (CCB_("Support for changing the active item has not been "
                   "implemented yet.  Should be changing from"
                   "\n\n\t<b>%1%</b>\n\nto\n\n\t<b>%2%</b>"))
      % cache_name_
      % name).str (),
     true);
  tbd.run ();
}

void
dropdown::on_system (const std::string& name)
{
  this->on_custom (name);
}

void
dropdown::on_action (const std::string& name)
{
  Gtk::MessageDialog msg (name, false, Gtk::MESSAGE_WARNING);
  msg.set_secondary_text
    ((format(CCB_("Support for management action functions has not been "
                  "implemented yet.  This action could manipulate, and "
                  "revert to,\n\n\t<b>%1%</b>"))
      % cache_name_).str (),
     true);
  msg.run ();
}

}       // namespace gtkmm
}       // namespace utsushi
