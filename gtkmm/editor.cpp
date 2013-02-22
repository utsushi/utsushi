//  editor.cpp -- scanning dialog's option value editor
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
#include <cmath>
#include <list>

#include <boost/assert.hpp>
#include <boost/foreach.hpp>

#include <gtkmm/adjustment.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/togglebutton.h>

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/option.hpp>
#include <utsushi/range.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/store.hpp>

#include "editor.hpp"

namespace utsushi {
namespace gtkmm {

static void
on_toggled (editor *ed, std::string key, Gtk::CheckButton *w)
{
  ed->set (key, toggle (w->get_active ()));
}

static void
on_changed_popup (editor *ed, std::string key, Gtk::ComboBoxText *w)
{
  ed->set (key, ed->untranslate (key, std::string (w->get_active_text ())));
}

static void
on_changed_range (editor *ed, std::string key, Gtk::Adjustment *w)
{
  // Account for numeric imprecision when spinning.  This makes the
  // somewhat puzzling -0.00 display values go away.

  if (std::abs (w->get_value ()) < 1e-10)
    w->set_value (0);

  ed->set (key, w->get_value ());
}

static void
on_changed_entry (editor *ed, std::string key, Gtk::Entry *w)
{
  ed->set (key, ed->untranslate (key, std::string (w->get_text ())));
}

class option_visitor
  : public value::visitor< Gtk::Widget * >
{
public:
  option_visitor (editor& ed,
                  editor::widget_map& controls,
                  Glib::RefPtr< Gtk::SizeGroup > hgroup,
                  Glib::RefPtr< Gtk::SizeGroup > vgroup,
                  const option& opt)
    : ed_(&ed)
    , controls_(controls)
    , hgroup_(hgroup)
    , vgroup_(vgroup)
    , opt_(opt)
  {}

  //! \todo Move common specialization code into member functions
  template< typename T >
  result_type operator() (const T& t) const
  {
    return result_type ();
  }

protected:
  editor *ed_;
  editor::widget_map& controls_;
  Glib::RefPtr< Gtk::SizeGroup > hgroup_;
  Glib::RefPtr< Gtk::SizeGroup > vgroup_;
  const option& opt_;
};

template<>
option_visitor::result_type
option_visitor::operator() (const quantity& q) const
{
  Gtk::HBox *widget (0);
  Gtk::Widget *ctrl (0);

  /**/ if (dynamic_cast< range * > (opt_.constraint ().get ()))
    {
      range rc (opt_.constraint< range > ());

      Gtk::Adjustment *range = new Gtk::Adjustment
        (q.amount< double > (),
         rc.lower().amount< double > (),
         rc.upper().amount< double > (),
         (q.is_integral () ?  1 : 0.1),
         (q.is_integral () ? 10 : 1.0));

      range->signal_value_changed ()
        .connect (sigc::bind< editor *, std::string, Gtk::Adjustment * >
                  (sigc::ptr_fun (on_changed_range), ed_, opt_.key (), range));

      // :FIXME: allow for different types
      //  GTK+ has at least scrollbar (H/V), scale (H/V) and spinbutton as
      //  suitable GUI elements for this.  Going with spinbutton for now
      Gtk::SpinButton *spinner = new Gtk::SpinButton
        (*range, 0.0, q.is_integral () ? 0 : 2);
      spinner->set_alignment (Gtk::ALIGN_RIGHT);

      ctrl = spinner;
    }
  else if (dynamic_cast< store * > (opt_.constraint ().get ()))
    {
      //! \todo Add support for quantity stores
    }
  else if (!opt_.constraint ())
    {
      //! \todo Add support for free format quantities
    }

  if (ctrl)
    {
      Gtk::Label *label = new Gtk::Label (_(opt_.name ()));
      label->set_alignment (Gtk::ALIGN_RIGHT);

      widget = new Gtk::HBox (true);
      widget->pack_start (*(Gtk::manage (label)));
      widget->pack_start (*(Gtk::manage (ctrl)));

      vgroup_->add_widget (*ctrl);

      controls_[opt_.key ()] = ctrl;
    }

  return widget;
}

template<>
option_visitor::result_type
option_visitor::operator() (const string& s) const
{
  Gtk::HBox *widget (0);
  Gtk::Widget *ctrl (0);

  /**/ if (dynamic_cast< store * > (opt_.constraint ().get ()))
    {
      store sc = opt_.constraint<store> ();

      Gtk::ComboBoxText *popup = new Gtk::ComboBoxText ();

      store::const_iterator it;
      for (it = sc.begin (); sc.end () != it; ++it)
        {
          string choice = value (*it);
          popup->append_text (_(choice));
        }
      popup->set_active_text (_(s));

      popup->signal_changed ()
        .connect (sigc::bind< editor *, std::string, Gtk::ComboBoxText * >
                  (sigc::ptr_fun (on_changed_popup), ed_, opt_.key (), popup));

      ctrl = popup;
    }
  else if (!opt_.constraint())
    {
      Gtk::Entry *entry = new Gtk::Entry ();

      entry->set_text (std::string (s));

      entry->signal_changed ()
        .connect (sigc::bind< editor *, std::string, Gtk::Entry * >
                  (sigc::ptr_fun (on_changed_entry), ed_, opt_.key (), entry));

      ctrl = entry;
    }

  if (ctrl)
    {
      Gtk::Label *label = new Gtk::Label (_(opt_.name ()));
      label->set_alignment (Gtk::ALIGN_RIGHT);

      widget = new Gtk::HBox (true);
      widget->pack_start (*(Gtk::manage (label)));
      widget->pack_start (*(Gtk::manage (ctrl)));

      vgroup_->add_widget (*ctrl);

      controls_[opt_.key ()] = ctrl;
    }

  return widget;
}

template<>
option_visitor::result_type
option_visitor::operator() (const toggle& t) const
{
  Gtk::CheckButton *check = new Gtk::CheckButton (_(opt_.name ()));
  check->set_active (t);
  check->signal_toggled ()
    .connect (sigc::bind< editor *, std::string, Gtk::CheckButton * >
              (sigc::ptr_fun (on_toggled), ed_, opt_.key (), check));

  Gtk::HBox *widget = new Gtk::HBox (true);
  widget->pack_start (*(Gtk::manage (new Gtk::Label ())));
  widget->pack_start (*(Gtk::manage (check)));
  vgroup_->add_widget (*widget);

  controls_[opt_.key ()] = check;

  return widget;
}

class resetter
  : public value::visitor<>
{
public:
  resetter (Gtk::Widget *w, const option& opt)
    : widget_(w)
    , opt_(opt)
  {}

  template< typename T >
  result_type operator() (const T& t) const;

  using value::visitor<>::operator();

protected:
  Gtk::Widget *widget_;
  const option& opt_;
};

template<>
resetter::result_type
resetter::operator() (const quantity& q) const
{
  /**/ if (dynamic_cast< range * > (opt_.constraint ().get ()))
    {
      Gtk::SpinButton *spinner = static_cast< Gtk::SpinButton * > (widget_);
      spinner->set_value (q.amount< double > ());
    }
  else if (dynamic_cast< store * > (opt_.constraint ().get ()))
    {
      //! \todo Add support for quantity stores
    }
  else if (!opt_.constraint ())
    {
      //! \todo Add support for free format quantities
    }
}

template<>
resetter::result_type
resetter::operator() (const string& s) const
{
  /**/ if (dynamic_cast< store * > (opt_.constraint ().get ()))
    {
      Gtk::ComboBoxText *popup = static_cast< Gtk::ComboBoxText * > (widget_);
      popup->set_active_text (_(s));
    }
  else if (!opt_.constraint ())
    {
      Gtk::Entry *entry = static_cast< Gtk::Entry * > (widget_);
      entry->set_text (std::string (s));
    }
}

template<>
resetter::result_type
resetter::operator() (const toggle& t) const
{
  Gtk::CheckButton *check = static_cast< Gtk::CheckButton * > (widget_);

  check->set_active (t);
}

editor::editor (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr), matrix_(0), editor_(0)
{
  builder->get_widget ("toggle-zone", matrix_);
  builder->get_widget ("editor-zone", editor_);

  hgroup_ = Gtk::SizeGroup::create (Gtk::SIZE_GROUP_HORIZONTAL);
  vgroup_ = Gtk::SizeGroup::create (Gtk::SIZE_GROUP_VERTICAL);

  int toggles = tags::count () + 1;     // for tag-less options
  // FIXME temporarily killing tag::application
  --toggles;

  int cols = matrix_->property_n_columns ().get_value ();
  int rows = (toggles + cols - 1) / cols;
  matrix_->resize (rows, cols);

  tags::const_iterator it = tags::begin ();
  int i;
  for (i = 0; tags::end () != it; ++i, ++it)
    {
      if (tag::application == *it) {
        --i;
        continue;               // FIXME
        app_key_  = *it;
        app_name_ = format (it->name ());
        app_desc_ = format (it->text ());
      }

      Gtk::ToggleButton *toggle = 0;

      toggle = new Gtk::ToggleButton (_(it->name ()));
      toggle->signal_toggled ()
        .connect (sigc::mem_fun (*this, &editor::on_toggled));
      if (it->text ())
        toggle->set_tooltip_text (_(it->text ()));

      toggles_[*it] = toggle;

      matrix_->attach (*toggle,
                       i % cols, i % cols + 1,
                       i / cols, i / cols + 1);
    }

  {                             // add a toggle for tag-less options
    Gtk::ToggleButton *toggle = new Gtk::ToggleButton (_("Other"));
    toggle->signal_toggled ()
      .connect (sigc::mem_fun (*this, &editor::on_toggled));
    toggles_["~"] = toggle;
    matrix_->attach (*toggle,
                     i % cols, i % cols + 1,
                     i / cols, i / cols + 1);
  }

  if (app_key_)
    {
      set_application_name (_("Application"));
      toggles_[app_key_]->set_sensitive (false);
    }

  matrix_->show_all ();
}

void
editor::add_widget (option& opt)
{
  value val (opt);
  option_visitor v (*this, this->controls_, hgroup_, vgroup_, opt);

  Gtk::Widget *widget = val.apply (v);

  if (widget)
    {
      widget = Gtk::manage (widget);
      widget->show_all ();
      widget->set_name (opt.key ());
      editor_->pack_start (*widget, Gtk::PACK_SHRINK);

      editors_.push_back (std::make_pair (opt.key (), widget));
    }
  else
    {
      log::error ("cannot create controller for %1%") % opt.key ();
    }
}

void
editor::set_application_name (const std::string& name)
{
  if (!app_key_) return;

  Gtk::Button *toggle (toggles_[app_key_]);

  try {
    toggle->set_label        ((app_name_ % name).str ());
    toggle->set_tooltip_text ((app_desc_ % name).str ());
  }
  catch (boost::io::format_error& e) {
    log::error (e.what ());
  }
}

void
editor::on_options_changed (option::map::ptr om)
{
  log::brief ("update the set of controllers");

  editors_.clear ();
  group_.clear ();

  delete editor_;               // :FIXME: should only kill all kids
  editor_ = new Gtk::VBox;
  editor_->show ();
  pack_start (*editor_);

  opts_ = om;

  //  For the time being, we will use a group rather than tag oriented
  //  display.  An option is part of a group if it has a matching tag.
  //  However, options cannot belong to multiple groups.  The tag with
  //  highest priority determines which group an option belongs to.

  std::set< key > seen;
  std::set< key >::size_type count;
  tags::const_iterator it (tags::begin ());
  for (; tags::end () != it; ++it)
    {
      if (tag::application == *it) continue; // FIXME
      count = seen.size ();

      option::map::iterator om_it (opts_->begin ());
      for (; opts_->end () != om_it; ++om_it)
        {
          if (!seen.count (om_it->key ())
              && om_it->is_at (level::standard)
              && om_it->tags ().count (*it))
            {
              add_widget (*om_it);
              seen.insert (om_it->key ());
              group_[om_it->key ()] = *it;
            }
        }
      toggles_[*it]->set_sensitive (count != seen.size ());

      if (tag::geometry == *it)
        {
          // FIXME Fast hack to flip top-left and bottom-right order
          BOOST_ASSERT (seen.size () - count == 4);
          editor_->reorder_child (*editors_[count + 2].second, count + 0);
          editor_->reorder_child (*editors_[count + 3].second, count + 1);
        }
    }
  count = seen.size ();

  //  Pick up options without any tags

  option::map::iterator om_it (opts_->begin ());
  for (; opts_->end () != om_it; ++om_it)
    {
      if (!seen.count (om_it->key ())
          && om_it->is_at (level::standard))
        {
          add_widget (*om_it);
          seen.insert (om_it->key ());
          group_[om_it->key ()] = "~";
        }
    }
  toggles_["~"]->set_sensitive (count != seen.size ());

  // FIXME hack to get the ADF-only options desensitized
  (*opts_)["device/doc-source"] = "ADF";
  (*opts_)["device/doc-source"] = "Flatbed";

  on_toggled ();

  {                             // show certain options by default
    Gtk::ToggleButton *toggle;

    if (toggles_.count (tag::application))
      {
        toggle = toggles_[tag::application];
        if (toggle)
          toggle->set_active (toggle->get_sensitive ());
      }

    toggle = toggles_[tag::general];
    if (toggle)
      toggle->set_active (toggle->get_sensitive ());

    // For as long as we do not have area selection support via
    // preview area

    toggle = toggles_[tag::geometry];
    if (toggle)
      toggle->set_active (toggle->get_sensitive ());
  }
}

void
editor::set (const std::string& key, const value& v)
{
  if (!opts_->count (key)) return;

  option opt ((*opts_)[key]);

  if (v == opt) return;

  try
    {
      opt = v;
    }
  catch (const constraint::violation& e)
    {
      Gtk::MessageDialog message (_("Restoring previous value"),
                                  false, Gtk::MESSAGE_WARNING);
      message.set_secondary_text
        (_("The selected combination of values is not supported."));
      message.run ();

      resetter r (controls_[key], opt);
      value (opt).apply (r);
    }

  for_each (editors_.begin (), editors_.end (),
            sigc::mem_fun (*this, &editor::update_appearance));
}

string
editor::untranslate (const key& k, const string& s)
{
  store *sp = dynamic_cast< store * > ((*opts_)[k].constraint ().get ());

  if (!sp) return s;

  store::const_iterator it;
  for (it = sp->begin (); sp->end () != it; ++it)
    {
      string candidate = value (*it);
      if (s == _(candidate)) return candidate;
    }

  log::error ("no translation matching '%1%'") % s;

  return s;
}

void
editor::on_toggled ()
{
  log::brief ("update controller visibility");

  for_each (editors_.begin (), editors_.end (),
            sigc::mem_fun (*this, &editor::update_appearance));
}

//!  Set toggle sensitivity based on presence of \a tags.
void
editor::set_toggles_sensitive (const tag_set& tags)
{
  toggle_map::iterator it;

  for (it = toggles_.begin (); toggles_.end () != it; ++it)
    {
      bool found = (tags.end () != tags.find (it->first));
      it->second->set_sensitive (found);
    }
}

void
editor::update_appearance (keyed_list::value_type& v)
{
  key          k = v.first;
  Gtk::Widget *w = v.second;

  if (!opts_->count (k))
    {
      w->set_sensitive (false);
      if (toggles_.at (group_[k])->get_active ())
        w->show ();
      else
        w->hide ();
      return;
    }

  option opt ((*opts_)[k]);

  w->set_sensitive (!opt.is_read_only ());

  if (opt.is_active () && active_toggle_(opt.tags ()))
    w->show ();
  else
    w->hide ();
}

bool
editor::active_toggle_(const std::set< key >& tags) const
{
  Gtk::ToggleButton *toggle (toggles_.at ("~"));

  if (tags.empty ())
    return (toggle && toggle->get_active ());

  BOOST_FOREACH (utsushi::key k, tags)
    {
      toggle_map::const_iterator it (toggles_.find (k));

      if (toggles_.end () != it)
        {
          Gtk::ToggleButton *toggle (it->second);

          if (toggle && toggle->get_active ()) return true;
        }
    }
  return false;
}

}       // namespace gtkmm
}       // namespace utsushi
