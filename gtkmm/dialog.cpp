//  dialog.cpp -- to acquire image data
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

#include <iostream>
#include <string>

#include <boost/throw_exception.hpp>

#include <gtkmm/action.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>

#include <utsushi/file.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/run-time.hpp>

#include "../filters/g3fax.hpp"
#include "../filters/padding.hpp"
#include "../filters/pdf.hpp"
#include "../filters/pnm.hpp"
#include "../outputs/tiff.hpp"

#include "chooser.hpp"
#include "dialog.hpp"
#include "editor.hpp"
#include "presets.hpp"
#include "preview.hpp"

namespace utsushi {
namespace gtkmm {

using std::logic_error;

dialog::dialog (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr), opts_ (new option::map), app_opts_ (new option::map),
    jpeg_()
{
  Glib::RefPtr<Glib::Object> obj = builder->get_object ("uimanager");
  ui_manager_ = Glib::RefPtr<Gtk::UIManager>::cast_dynamic (obj);

  if (!ui_manager_)
    BOOST_THROW_EXCEPTION
      (logic_error (_("Dialog specification requires a 'uimanager'")));

  //  set up custom child widgets

  chooser *device_list = 0;
  preview *preview = 0;
  {
    builder->get_widget_derived ("scanner-list", device_list);
    device_list->signal_device_changed ()
      .connect (sigc::mem_fun (*this, &dialog::on_device_changed));
  }
  if (builder->get_object ("presets-list")) {
    presets *widget = 0;
    builder->get_widget_derived ("presets-list", widget);
  }
  {
    builder->get_widget_derived ("preview-area", preview);
    device_list->signal_device_changed ()
      .connect (sigc::mem_fun (*preview, &preview::on_device_changed));
  }
  {
    builder->get_widget_derived ("editor-pane", editor_);
    signal_options_changed ()
      .connect (sigc::mem_fun (*editor_, &editor::on_options_changed));
  }

  if (!device_list->get_model ()->children ().empty ())
    device_list->set_active (0);

  //  customise self

  if (builder->get_object ("settings-expander")) {
    builder->get_widget ("dialog-pane", dialog_);
    builder->get_widget ("settings-expander", expand_);
    if (expand_) {
      Glib::RefPtr<Gtk::Action> action;
      action = ui_manager_->get_action ("/dialog/expand");
      if (action) {
        action->connect_proxy (*expand_);
        action->signal_activate ()
          .connect (sigc::mem_fun (*this, &dialog::on_detail_toggled));
      } else {
        expand_->hide ();
      }
    }
  }

  Gtk::Button *cancel = 0;
  builder->get_widget ("cancel-button", cancel);
  if (cancel) {
    Glib::RefPtr<Gtk::Action> action;
    action = ui_manager_->get_action ("/dialog/cancel");
    if (action) {
      action->connect_proxy (*cancel);
      cancel_ = action->signal_activate ()
        .connect (sigc::mem_fun (*this, &Gtk::Widget::hide));
    }
  }

  if (builder->get_object ("refresh-button")) {
    Gtk::Button *refresh = 0;
    builder->get_widget ("refresh-button", refresh);
    if (refresh) {
      Glib::RefPtr<Gtk::Action> action;
      action = ui_manager_->get_action ("/preview/refresh");
      if (action) {
        action->connect_proxy (*refresh);
        action->signal_activate ()
          .connect (sigc::mem_fun (*preview, &preview::on_refresh));
      }
    }
  }

  Gtk::Button *scan = 0;
  builder->get_widget ("scan-button", scan);
  if (scan) {
    Glib::RefPtr<Gtk::Action> action;
    action = ui_manager_->get_action ("/dialog/scan");
    if (action) {
      action->connect_proxy (*scan);
      action->signal_activate ()
        .connect (sigc::mem_fun (*this, &dialog::on_scan));
    }
  }

  if (builder->get_object ("help-button")) {
    Gtk::Button *about = 0;
    builder->get_widget ("help-button", about);
    if (about) {
      Glib::RefPtr<Gtk::Action> action;
      action = ui_manager_->get_action ("/dialog/help");
      if (action) {
        action->connect_proxy (*about);
        action->signal_activate ()
          .connect (sigc::mem_fun (*this, &dialog::on_about));
      }
    }
  }
  set_sensitive ();
}

sigc::signal< void, option::map::ptr >
dialog::signal_options_changed ()
{
  return signal_options_changed_;
}

void
dialog::set_sensitive (void)
{
  Glib::RefPtr<Gtk::Action> a;

  a = ui_manager_->get_action ("/dialog/scan");
  if (a) { a->set_sensitive (idevice_); }
}

void
dialog::on_detail_toggled (void)
{
  if (!expand_ || !editor_) return;

  if (expand_->get_active ()) {
    editor_->show ();
  } else {
    editor_->hide ();
    if (dialog_)
      resize (dialog_->get_width (), dialog_->get_height ());
  }
}

void
dialog::on_scan_update (traits::int_type c)
{
  if (traits::eos () == c || traits::eof () == c) {
    // enable scan button
    Glib::RefPtr<Gtk::Action> action;
    action = ui_manager_->get_action ("/dialog/scan");
    if (action) action->set_sensitive (true);
    // retarget cancel button
    action = ui_manager_->get_action ("/dialog/cancel");
    if (action) {
      cancel_.disconnect ();
      cancel_ = action->signal_activate ()
        .connect (sigc::mem_fun (*this, &Gtk::Widget::hide));
    }
  }
}

void
dialog::on_scan (void)
{
  Gtk::FileChooserDialog dialog (*this, _("Save As"),
                                 Gtk::FILE_CHOOSER_ACTION_SAVE);

  dialog.set_current_folder (".");
  // We do this explicitly ourselves later on
  dialog.set_do_overwrite_confirmation (false);

  dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

  {
    Gtk::FileFilter filter;
    filter.add_mime_type ("image/*");
    filter.set_name (_("Image Files"));
    dialog.add_filter (filter);
  }
  {
    Gtk::FileFilter filter;
    filter.add_pattern ("*");
    filter.set_name (_("All Files"));
    dialog.add_filter (filter);
  }

  dialog.show_all ();

  if (Gtk::RESPONSE_OK == dialog.run ()) {

    // Infer desired image format from file extension

    fs::path filename (std::string (dialog.get_filename ()));
    fs::path::string_type ext = filename.extension ().string ();

    if (ext.empty ()
        || !(   "pnm"  == ext.substr (1)
             || "jpg"  == ext.substr (1)
             || "jpeg" == ext.substr (1)
             || "pdf"  == ext.substr (1)
             || "tif"  == ext.substr (1)
             || "tiff" == ext.substr (1))
        ) {
      Gtk::MessageDialog tbd (_("Unsupported file format."),
                              false, Gtk::MESSAGE_WARNING);
      tbd.set_secondary_text
        ((format (_("Support for images in %1% format has not been "
                    "implemented yet.\n"
                    "Please use PNM, JPEG, PDF or TIFF for now."))
          % (ext.empty () ? "default" : ext.substr (1))).str (), true);
      tbd.run ();
      return;
    }

    // Check whether we need to use raw image data transfer

    bool use_raw_transfer = false;
    use_raw_transfer |= ((*opts_)["device/image-type"] == "Gray (1 bit)");
    use_raw_transfer |= ("pnm"  == ext.substr (1));
    use_raw_transfer |= ("tif"  == ext.substr (1));
    use_raw_transfer |= ("tiff" == ext.substr (1));

    if (use_raw_transfer)
      {
        (*opts_)["device/transfer-format"] = "RAW";
      }

    try
      {
        (*jpeg_.options ())["quality"]
          = value ((*opts_)["device/jpeg-quality"]);
      }
    catch (...)
      {}

    // Configure the filter chain

    ostream::ptr ostr (new ostream);

    /**/ if ("pnm"  == ext.substr (1))
      {
        ostr->push (ofilter::ptr (new _flt_::padding));
        ostr->push (ofilter::ptr (new _flt_::pnm));
      }
    else if (   "jpg"  == ext.substr (1)
             || "jpeg" == ext.substr (1))
      {
        if ((*opts_)["device/image-type"] == "Gray (1 bit)")
          BOOST_THROW_EXCEPTION
            (logic_error
             (_("JPEG does not support bi-level imagery")));

        if (use_raw_transfer)
          {
            ostr->push (ofilter::ptr (new _flt_::padding));
            ostr->push (ofilter::ptr (&jpeg_, null_deleter ()));
          }
        else
          {
            (*opts_)["device/transfer-format"] = "JPEG";
          }
      }
    else if ("pdf" == ext.substr (1))
      {
        if (use_raw_transfer)
          {
            ostr->push (ofilter::ptr (new _flt_::padding));
            if ((*opts_)["device/image-type"] == "Gray (1 bit)")
              {
                ostr->push (ofilter::ptr (new _flt_::g3fax));
              }
            else
              {
                ostr->push (ofilter::ptr (&jpeg_, null_deleter ()));
              }
          }
        else
          {
            (*opts_)["device/transfer-format"] = "JPEG";
          }
        ostr->push (ofilter::ptr (new _flt_::pdf));
      }
    else if (   "tif"  == ext.substr (1)
             || "tiff" == ext.substr (1))
      {
        ostr->push (ofilter::ptr (new _flt_::padding));
      }
    else
      BOOST_THROW_EXCEPTION
        (logic_error (_("unsupported file format")));

    // Create an output device

    odevice::ptr odev;

    /**/ if ("pdf"  == ext.substr (1))
      {
        odev = odevice::ptr (new file_odevice (filename));
      }
    else if (   "tif"  == ext.substr (1)
             || "tiff" == ext.substr (1))
      {
        odev = odevice::ptr (new _out_::tiff_odevice (filename));
      }
    else if (idevice_->is_single_image ())
      {
        odev = odevice::ptr (new file_odevice (filename));
      }

    if (odev)
      {
        if (fs::exists (filename))
          {
            Gtk::MessageDialog tbd
              ((format (_("A file named \"%1%\" already exists.  "
                          "Do you want to replace it?"))
                % filename.filename ().string ()).str (),
               false, Gtk::MESSAGE_QUESTION,
               Gtk::BUTTONS_OK_CANCEL);
            tbd.set_secondary_text
              ((format (_("The file already exists in \"%1%\".  "
                          "Replacing it will overwrite its contents."))
                % filename.parent_path ().string ()).str ());
            if (Gtk::RESPONSE_OK != tbd.run ()) return;
          }
      }
    else {                      // put every image in a separate file
      path_generator gen (!filename.parent_path ().empty ()
                          ? filename.parent_path () / filename.stem ()
                          : filename.stem (), ext);
      odev = odevice::ptr (new file_odevice (gen));

      Gtk::MessageDialog tbd (_("This may overwrite existing files!"),
                              false, Gtk::MESSAGE_WARNING,
                              Gtk::BUTTONS_OK_CANCEL);
      tbd.set_secondary_text
        ((format (_("Files matching \"%1%\" may be overwritten."))
          % gen ().string ()).str (), true);
      if (Gtk::RESPONSE_OK != tbd.run ()) return;
    }
    ostr->push (odev);

    // disable scan button
    Glib::RefPtr<Gtk::Action> action;
    action = ui_manager_->get_action ("/dialog/scan");
    if (action) action->set_sensitive (false);
    // retarget cancel button
    action = ui_manager_->get_action ("/dialog/cancel");
    if (action) {
      cancel_.disconnect ();
      cancel_ = action->signal_activate ()
        .connect (sigc::mem_fun (*pump_, &pump::cancel));
    }

    pump_->start (ostr);
  }
}

void
dialog::on_about (void)
{
  run_time rt;

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create ();
  builder->add_from_file (rt.data_file (run_time::pkg, "gtkmm/about.xml"));

  Gtk::Dialog *about;
  builder->get_widget ("about-dialog", about);

  about->run ();
  about->hide ();
}

void
dialog::on_device_changed (utsushi::idevice::ptr idev)
{
  idevice_ = idev;

  opts_.reset (new option::map);
  opts_->add_option_map () ("application", app_opts_);
  opts_->add_option_map () ("device", idevice_->options ());
  signal_options_changed_.emit (opts_);

  pump_ = pump::ptr (new pump (idev));

  pump_->signal_marker (pump::in)
    .connect (sigc::mem_fun (*this, &dialog::on_scan_update));
  pump_->signal_marker (pump::out)
    .connect (sigc::mem_fun (*this, &dialog::on_scan_update));
  pump_->signal_notify ()
    .connect (sigc::mem_fun (*this, &dialog::on_notify));

  set_sensitive ();
}

void
dialog::on_notify (log::priority level, std::string message)
{
  Gtk::MessageType gui_level;
  traits::int_type c = traits::eof(); // assume scan was cancelled

  switch (level)
    {
    case log::FATAL: gui_level = Gtk::MESSAGE_ERROR  ; break;
    case log::ALERT: gui_level = Gtk::MESSAGE_WARNING; break;
    case log::ERROR: gui_level = Gtk::MESSAGE_INFO   ; break;
    default:
      gui_level = Gtk::MESSAGE_OTHER;
      c = 0;                    // not an error -> not cancelled
    }
  Gtk::MessageDialog dialog (message, false, gui_level);

  dialog.set_keep_above ();
  dialog.run ();

  if (traits::eof () == c)
    on_scan_update (c);
}

}       // namespace gtkmm
}       // namespace utsushi
