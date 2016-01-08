//  action-dialog.cpp -- controls to trigger device maintenance
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "action-dialog.hpp"

#include <utsushi/exception.hpp>
#include <utsushi/functional.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/memory.hpp>
#include <utsushi/thread.hpp>

#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/spinner.h>
#include <gtkmm/stock.h>
#include <gtkmm/widget.h>

#include <string>

namespace {

void set_insensitive (Gtk::Widget& w, Gtk::Widget *button)
{
  w.set_sensitive (&w == button);
}

void set_sensitive (Gtk::Widget& w)
{
  w.set_sensitive (true);
}

// Tweak message area label widget properties to prevent highlighting
// of message(s) and attempt to display the whole message, even if it
// is rather long-winded.
void set_properties (Gtk::Widget& w)
{
  try
    {
      Gtk::Label& label (dynamic_cast< Gtk::Label& > (w));

      label.set_line_wrap ();
      label.set_selectable (false);
    }
  catch (const std::bad_cast&)
    {
      try
        {
          Gtk::Container& c (dynamic_cast< Gtk::Container& > (w));

          c.foreach (&::set_properties);
        }
      catch (const std::bad_cast&) {}
    }
}

}       // namespace

namespace utsushi {
namespace gtkmm {

struct action_runner
{
  action_runner (option::map::ptr p, std::string key)
    : om (p), k (key)
  {}

  void operator() ()
  {
    try
      {
        rc = make_shared< result_code > ((*om)[k].run ());
      }
    catch (...)
      {
        ep = make_shared< exception_ptr > (current_exception ());
      }
  }

  option::map::ptr om;
  std::string k;
  shared_ptr< result_code >   rc;
  shared_ptr< exception_ptr > ep;
};

action_dialog::action_dialog (option::map::ptr actions, Gtk::Widget *trigger,
                              bool use_spinner)
  : Gtk::Dialog (SEC_("Maintenance"), true)
  , actions_(actions)
  , trigger_(trigger)
  , buttons_(new Gtk::HButtonBox ())
  , process_(nullptr)
{
  // window manager hints
  set_title (SEC_("Maintenance"));
  set_position (Gtk::WIN_POS_CENTER_ALWAYS);
  set_keep_above ();
  set_deletable (false);

  // use a spacier layout
  buttons_->set_layout (Gtk::BUTTONBOX_SPREAD);
  buttons_->set_spacing (10);
  buttons_->set_border_width (20);

  option::map::iterator it (actions_->begin ());

  for (; actions_->end () != it; ++it)
    {
      Gtk::Button *b = new Gtk::Button (_(it->name ()));

      if (use_spinner)
        {
          b->set_image (*(Gtk::manage (new Gtk::Spinner)));
          b->get_image ()->hide ();
        }
      b->signal_clicked ()
        .connect (sigc::bind< Gtk::Button *, std::string, std::string >
                  (sigc::mem_fun (*this, &action_dialog::on_action),
                   b, it->key (), it->text ()));
      buttons_->pack_end (*(Gtk::manage (b)));
    }

  get_vbox ()->pack_start (*(Gtk::manage (buttons_)));

  add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
}

action_dialog::~action_dialog ()
{
  if (process_)
    {
      process_->join ();
    }
  delete process_;
}

void
action_dialog::on_action (Gtk::Button *button, std::string key,
                          std::string message)
{
  Gtk::MessageDialog dlg (*this, message, false, Gtk::MESSAGE_OTHER);
  Gtk::Spinner *spinner =
    static_cast< Gtk::Spinner * > (button->get_image ());

  // window manager hints
  dlg.set_position (Gtk::WIN_POS_CENTER_ALWAYS);
  dlg.set_keep_above ();
  dlg.set_deletable (false);
  {                             // minimize run-time resizing of dlg
    // For an action_dialog with two to three buttons this ought to
    // work reasonably well.  If forces the dlg to have at least the
    // size of the action_dialog.
    int w, h;
    get_size (w, h);
    dlg.set_default_size (w, h);
  }

  dlg.set_modal (true);
  set_response_sensitive (Gtk::RESPONSE_OK, false);
  buttons_->foreach (sigc::bind (&::set_insensitive, button));

  if (spinner)
    {
      spinner->set_visible (true);
      spinner->show ();
      spinner->start ();
    }
  else
    {
      dlg.get_image ()->set_visible (false);
      // FIXME reserve icon area to avoid the message from moving
      dlg.get_action_area ()->set_sensitive (false);
      dlg.get_vbox ()->foreach (&::set_properties);
      dlg.show ();
    }

  action_runner ar (actions_, key);

  if (process_)
    {
      process_->join ();
    }
  delete process_;
  process_ = new thread (ref (ar));

  while (!ar.rc && !ar.ep)
    {
      while (Gtk::Main::events_pending ())
        Gtk::Main::iteration ();
    }

  /**/ if (ar.ep)
    {
      std::string msg;
      try
        {
          rethrow_exception (*ar.ep);
        }
      catch (const std::exception& e)
        {
          msg = e.what ();
        }
      catch (...)
        {
          msg = "unknown exception";
        }

      if (spinner)              // display a separate dialog
        {
          Gtk::MessageDialog dlg (*this, _(msg), false, Gtk::MESSAGE_ERROR);
          dlg.run ();
        }
      else                      // override info icon
        {
          Gtk::Image *img = new Gtk::Image (Gtk::Stock::DIALOG_ERROR,
                                            Gtk::ICON_SIZE_DIALOG);
          dlg.set_image (*Gtk::manage (img));
          dlg.set_message (_(msg));
        }
    }
  else if (ar.rc)
    {
      if (*ar.rc)               // something went wrong
        {
          if (spinner)          // display a separate dialog
            {
              Gtk::MessageDialog dlg (*this, ar.rc->message (), false,
                                      Gtk::MESSAGE_WARNING);
              dlg.run ();
            }
          else                  // override info icon
            {
              Gtk::Image *img = new Gtk::Image (Gtk::Stock::DIALOG_WARNING,
                                                Gtk::ICON_SIZE_DIALOG);
              dlg.set_image (*Gtk::manage (img));
            }
        }
      else
        {
          Gtk::Image *img = new Gtk::Image (Gtk::Stock::DIALOG_INFO,
                                            Gtk::ICON_SIZE_DIALOG);
          dlg.set_image (*Gtk::manage (img));
        }
      dlg.set_message (ar.rc->message ());
    }
  else
    {
      // BOOST_THROW_EXCEPTION (internal_error);
    }

  if (spinner)
    {
      spinner->stop ();
      spinner->set_visible (false);
      spinner->hide ();
    }
  else
    {
      dlg.get_image ()->set_visible (true);
      dlg.get_action_area ()->set_sensitive (true);
      dlg.get_widget_for_response (Gtk::RESPONSE_OK)->grab_focus ();
      dlg.run ();
    }

  buttons_->foreach (&::set_sensitive);
  set_response_sensitive (Gtk::RESPONSE_OK, true);
  get_widget_for_response (Gtk::RESPONSE_OK)->grab_focus ();
}

void
action_dialog::on_maintenance (void)
{
  if (trigger_) trigger_->set_sensitive (false);

  // Looks like Gtk::Box uses a stack internally.  First packed widget
  // ends up at the end of the vector.  Make sure it gets the focus.
  std::vector< Gtk::Widget * > v = buttons_->get_children ();
  if (v.size ()) v.back ()->grab_focus ();

  show_all ();
  run ();
  hide ();

  if (trigger_) trigger_->set_sensitive (true);
}

}       // namespace gtkmm
}       // namespace utsushi
