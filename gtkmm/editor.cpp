//  editor.cpp -- scanning dialog's option value editor
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2013  Olaf Meeuwissen
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
#include <cmath>
#include <list>
#include <set>
#include <sstream>

#include <boost/assert.hpp>
#include <boost/assign/list_of.hpp>
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

using std::stringstream;

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
on_changed_popup_q (editor *ed, std::string key, Gtk::ComboBoxText *w)
{
  stringstream ss;
  quantity q;

  ss << w->get_active_text ();
  ss >> q;

  ed->set (key, q);
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
                  editor::signal_map& connects,
                  Glib::RefPtr< Gtk::SizeGroup > hgroup,
                  Glib::RefPtr< Gtk::SizeGroup > vgroup,
                  const option& opt)
    : ed_(&ed)
    , controls_(controls)
    , connects_(connects)
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
  editor::signal_map& connects_;
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
  sigc::connection cnx;

  /**/ if (dynamic_cast< range * > (opt_.constraint ().get ()))
    {
      range rc (opt_.constraint< range > ());

      Gtk::Adjustment *range = new Gtk::Adjustment
        (q.amount< double > (),
         rc.lower().amount< double > (),
         rc.upper().amount< double > (),
         (q.is_integral () ?  1 : 0.1),
         (q.is_integral () ? 10 : 1.0));

      cnx = range->signal_value_changed ()
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
      Gtk::ComboBoxText *popup = new Gtk::ComboBoxText ();

      store sc (opt_.constraint< store > ());
      store::const_iterator it;

      for (it = sc.begin (); sc.end () != it; ++it)
        {
          stringstream ss;
          ss << *it;
          popup->append_text (ss.str ());
        }
      stringstream ss;
      ss << q;
      popup->set_active_text (ss.str ());

      BOOST_FOREACH (Gtk::CellRenderer *cr, popup->get_cells ())
        {
          // There's no overload yet for Gtk::AlignmentEnum :-(
          // cr->set_alignment (Gtk::ALIGN_END, Gtk::ALIGN_CENTER);
          cr->set_alignment (1.0, 0.5);
        }

      cnx = popup->signal_changed ()
        .connect (sigc::bind< editor *, std::string, Gtk::ComboBoxText * >
                  (sigc::ptr_fun (on_changed_popup_q), ed_, opt_.key (), popup));

      ctrl = popup;
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
      connects_[opt_.key ()] = cnx;
    }

  return widget;
}

template<>
option_visitor::result_type
option_visitor::operator() (const string& s) const
{
  Gtk::HBox *widget (0);
  Gtk::Widget *ctrl (0);
  sigc::connection cnx;

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

      cnx = popup->signal_changed ()
        .connect (sigc::bind< editor *, std::string, Gtk::ComboBoxText * >
                  (sigc::ptr_fun (on_changed_popup), ed_, opt_.key (), popup));

      ctrl = popup;
    }
  else if (!opt_.constraint())
    {
      Gtk::Entry *entry = new Gtk::Entry ();

      entry->set_text (std::string (s));

      cnx = entry->signal_changed ()
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
      connects_[opt_.key ()] = cnx;
    }

  return widget;
}

template<>
option_visitor::result_type
option_visitor::operator() (const toggle& t) const
{
  Gtk::CheckButton *check = new Gtk::CheckButton (_(opt_.name ()));
  check->set_active (t);
  sigc::connection cnx = check->signal_toggled ()
    .connect (sigc::bind< editor *, std::string, Gtk::CheckButton * >
              (sigc::ptr_fun (on_toggled), ed_, opt_.key (), check));

  Gtk::HBox *widget = new Gtk::HBox (true);
  widget->pack_start (*(Gtk::manage (new Gtk::Label ())));
  widget->pack_start (*(Gtk::manage (check)));
  vgroup_->add_widget (*widget);

  controls_[opt_.key ()] = check;
  connects_[opt_.key ()] = cnx;

  return widget;
}

class resetter
  : public value::visitor<>
{
public:
  resetter (Gtk::Widget *w, sigc::connection& cnx, const option& opt,
            bool update_constraint = true)
    : widget_(w)
    , cnx_(cnx)
    , opt_(opt)
    , update_constraint_(update_constraint)
  {}

  template< typename T >
  result_type operator() (const T& t) const;

  using value::visitor<>::operator();

protected:
  Gtk::Widget *widget_;
  sigc::connection& cnx_;
  const option& opt_;
  bool update_constraint_;
};

template<>
resetter::result_type
resetter::operator() (const quantity& q) const
{
  cnx_.block ();
  /**/ if (dynamic_cast< range * > (opt_.constraint ().get ()))
    {
      Gtk::SpinButton *spinner = static_cast< Gtk::SpinButton * > (widget_);

      if (update_constraint_)
        {
          range rc = opt_.constraint< range > ();
          spinner->set_range (rc.lower ().amount< double > (),
                              rc.upper ().amount< double > ());
          spinner->set_digits (q.is_integral () ? 0 : 2);
          spinner->set_increments (q.is_integral () ?  1 : 0.1,
                                   q.is_integral () ? 10 : 1.0);
        }
      spinner->set_value (q.amount< double > ());
    }
  else if (dynamic_cast< store * > (opt_.constraint ().get ()))
    {
      Gtk::ComboBoxText *popup = static_cast< Gtk::ComboBoxText * > (widget_);

      if (update_constraint_)
        {
          store sc = opt_.constraint< store > ();
          store::const_iterator it;

          popup->clear ();
          for (it = sc.begin (); it != sc.end (); ++it)
            {
              stringstream choice;
              choice << value (*it);
              popup->append_text (choice.str ());
            }
        }
      stringstream ss;
      ss << q;
      popup->set_active_text (ss.str ());
    }
  else if (!opt_.constraint ())
    {
      //! \todo Add support for free format quantities
    }
  cnx_.unblock ();
}

template<>
resetter::result_type
resetter::operator() (const string& s) const
{
  cnx_.block ();
  /**/ if (dynamic_cast< store * > (opt_.constraint ().get ()))
    {
      Gtk::ComboBoxText *popup = static_cast< Gtk::ComboBoxText * > (widget_);

      if (update_constraint_)
        {
          store sc = opt_.constraint< store > ();
          store::const_iterator it;

          popup->clear ();
          for (it = sc.begin (); it != sc.end (); ++it)
            {
              string choice = value (*it);
              popup->append_text (_(choice));
            }
        }
      popup->set_active_text (_(s));
    }
  else if (!opt_.constraint ())
    {
      Gtk::Entry *entry = static_cast< Gtk::Entry * > (widget_);
      entry->set_text (std::string (s));
    }
  cnx_.unblock ();
}

template<>
resetter::result_type
resetter::operator() (const toggle& t) const
{
  cnx_.block ();
  Gtk::CheckButton *check = static_cast< Gtk::CheckButton * > (widget_);

  check->set_active (t);
  cnx_.unblock ();
}

editor::editor (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr), matrix_(0), editor_(0)
  , block_on_toggled_(false)
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
    Gtk::ToggleButton *toggle = new Gtk::ToggleButton (SEC_("Other"));
    toggle->signal_toggled ()
      .connect (sigc::mem_fun (*this, &editor::on_toggled));
    toggles_["~"] = toggle;
    matrix_->attach (*toggle,
                     i % cols, i % cols + 1,
                     i / cols, i / cols + 1);
  }

  if (app_key_)
    {
      set_application_name (CCB_("Application"));
      toggles_[app_key_]->set_sensitive (false);
    }

  matrix_->show_all ();
}

void
editor::add_widget (option& opt)
{
  value val (opt);
  option_visitor v (*this, this->controls_, this->connects_,
                    hgroup_, vgroup_, opt);

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
editor::on_options_changed (option::map::ptr om,
                            const std::set<std::string>& option_blacklist)
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
              && om_it->tags ().count (*it)
              && !option_blacklist.count (om_it->key ()))
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
          && om_it->is_at (level::standard)
          && !option_blacklist.count (om_it->key ()))
        {
          add_widget (*om_it);
          seen.insert (om_it->key ());
          group_[om_it->key ()] = "~";
        }
    }
  toggles_["~"]->set_sensitive (count != seen.size ());

  // FIXME hack to get the other source only options desensitized
  //       Here's praying this doesn't trigger constraint::violations
  option source = (*opts_)["device/doc-source"];
  if (!source.constraint ()->is_singular ())
    {
      if (dynamic_cast< store * > (source.constraint ().get ()))
        {
          store s = source.constraint< store > ();

          value current = source;
          for (store::const_iterator it = s.begin (); it != s.end (); ++it)
            source = *it;

          source = current;
        }
    }

  // Rather than block the individual connections on each toggle, just
  // turn on_toggled() into a noop for the time being.  As all options
  // have just been replaced we need to call it explicitly anyway.

  block_on_toggled_ = true;
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
  block_on_toggled_ = false;

  on_toggled ();                // explicitly update the view
}

void
editor::set (const std::string& key, const value& v)
{
  if (!opts_->count (key)) return;

  option opt ((*opts_)[key]);
  value::map vm;

  if (key == "device/scan-area"
      && opts_->count ("doc-locate/crop"))
    {
      toggle t = (value ("Auto Detect") == v);

      vm[key] = (t ? value ("Maximum") : v);
      vm["doc-locate/crop"] = t;
      if (opts_->count ("device/overscan"))
        vm["device/overscan"] = t;
      if (opts_->count ("device/auto-kludge"))
        vm["device/auto-kludge"] = t;
    }
  else if (key == "magick/image-type"
           && opts_->count ("device/image-type"))
    {
      string type = v;
      if (type == "Monochrome")
        type = "Grayscale";
      vm[key] = v;
      try {
        vm["device/image-type"] = type;
      }
      catch (const std::out_of_range&){}
    }
  else
    {
      if (v == opt) return;
    }

  try
    {
      if (vm.empty ())
        opt = v;
      else
        opts_->assign (vm);
    }
  catch (const constraint::violation& e)
    {
      Gtk::MessageDialog message (SEC_("Restoring previous value"),
                                  false, Gtk::MESSAGE_WARNING);
      message.set_secondary_text
        (SEC_("The selected combination of values is not supported."));
      message.run ();

      resetter r (controls_[key], connects_[key], opt, false);
      value (opt).apply (r);
    }

  if (opts_->count ("device/long-paper-mode")
      && opts_->count ("doc-locate/deskew"))
    {
      toggle t1 = value ((*opts_)["device/long-paper-mode"]);
      (*opts_)["doc-locate/deskew"].active (!t1);
      toggle t2 = value ((*opts_)["doc-locate/deskew"]);
      (*opts_)["device/long-paper-mode"].active (!t2);
    }

  for_each (editors_.begin (), editors_.end (),
            sigc::mem_fun (*this, &editor::update_appearance));
  signal_values_changed_.emit (opts_);
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

sigc::signal<void, option::map::ptr>
editor::signal_values_changed ()
{
  return signal_values_changed_;
}

void
editor::on_toggled ()
{
  if (block_on_toggled_) return;

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

  // FIXME Keep the displayed value in sync with the effective value.
  //       This hack is needed until the options emit appropriate
  //       signals that allow us to do this in a suitable callback.
  if (k == "device/transfer-format")    // has device/mode dependency
    {
      resetter r (controls_[k], connects_[k], opt);
      value (opt).apply (r);
    }

  w->set_sensitive (!opt.is_read_only ());

  if (opt.is_active () && active_toggle_(opt.tags ()))
    w->show ();
  else
    w->hide ();

  if (k == "device/scan-area")  // has device/doc-source dependency
    {
      toggle automatic;
      if (opts_->count ("doc-locate/crop"))
        {
          automatic = value ((*opts_)["doc-locate/crop"]);
        }
      if (!automatic)
        {
          resetter r (controls_[k], connects_[k], opt);
          value (opt).apply (r);
        }
    }

  const std::set< key > coordinates = boost::assign::list_of
    ("device/tl-x")
    ("device/tl-y")
    ("device/br-x")
    ("device/br-y")
    ;
  if (coordinates.count (k))    // have device/scan-area as well as
                                // device/doc-source dependency
    {
      bool manual_width (false);
      if (opts_->count ("device/long-paper-mode")
          && (k == "device/tl-x" || k == "device/br-x"))
        {
          if (opts_->count ("device/doc-source")
              && (*opts_)["device/doc-source"] == string ("ADF"))
            {
              toggle t = value ((*opts_)["device/long-paper-mode"]);
              manual_width = t;
            }
        }

      if (opts_->count ("device/scan-area"))
        {
          string v = value ((*opts_)["device/scan-area"]);
          bool manual =
            (string ("Manual") == untranslate ("device/scan-area", v));
          manual_width &=
            (string ("Auto Detect") == untranslate ("device/scan-area", v)
             || (// because we may be emulating "Auto Detect" ourselves
                 opts_->count ("doc-locate/crop")
                 && (*opts_)["doc-locate/crop"] == value (toggle (true)))
             );

          keyed_list::iterator jt = editors_.begin ();
          while (editors_.end () != jt && k != jt->first) ++jt;
          if (editors_.end () != jt)
            jt->second->set_sensitive (manual || manual_width);

          resetter r (controls_[k], connects_[k], opt);
          value (opt).apply (r);
        }
    }

  // FIXME should be taken care of by the driver but it is checking
  //       resolution violations too late
  if (k == "device/long-paper-mode")
    {
      if (opts_->count ("device/doc-source"))
        {
          string v = value ((*opts_)["device/doc-source"]);
          bool active =
            (string ("ADF") == untranslate ("device/doc-source", v));

          w->set_sensitive (active);
        }
    }
  // FIXME should be taken care of by the driver but it is checking
  //       resolution violations too late
  if (k == "device/duplex")
    {
      if (opts_->count ("device/doc-source"))
        {
          string v = value ((*opts_)["device/doc-source"]);
          bool active =
            (string ("ADF") == untranslate ("device/doc-source", v));

          w->set_sensitive (active);
        }
    }
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
