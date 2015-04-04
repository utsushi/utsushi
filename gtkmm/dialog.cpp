//  dialog.cpp -- to acquire image data
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>

#include <gdkmm/cursor.h>
#include <gdkmm/general.h>

#include <gtkmm/action.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>

#include <utsushi/file.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/store.hpp>

#include "../filters/autocrop.hpp"
#include "../filters/deskew.hpp"
#include "../filters/g3fax.hpp"
#include "../filters/image-skip.hpp"
#include "../filters/jpeg.hpp"
#include "../filters/magick.hpp"
#include "../filters/padding.hpp"
#include "../filters/pdf.hpp"
#include "../filters/pnm.hpp"
#include "../outputs/tiff.hpp"

#include "action-dialog.hpp"
#include "chooser.hpp"
#include "file-chooser.hpp"
#include "dialog.hpp"
#include "editor.hpp"
#include "presets.hpp"
#include "preview.hpp"

#define nullptr 0

namespace fs = boost::filesystem;

namespace utsushi {
namespace gtkmm {

using std::logic_error;
using std::runtime_error;

dialog::dialog (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr), opts_ (new option::map), app_opts_ (new option::map)
  , maintenance_(nullptr)
  , maintenance_dialog_(nullptr)
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
    device_list->unreference ();
    device_list->signal_device_changed ()
      .connect (sigc::mem_fun (*this, &dialog::on_device_changed));
  }
  if (builder->get_object ("presets-list")) {
    presets *widget = 0;
    builder->get_widget_derived ("presets-list", widget);
    widget->unreference ();
  }
  {
    builder->get_widget_derived ("preview-area", preview);
    preview->unreference ();
    device_list->signal_device_changed ()
      .connect (sigc::mem_fun (*preview, &preview::on_device_changed));
  }
  {
    builder->get_widget_derived ("editor-pane", editor_);
    editor_->unreference ();
    signal_options_changed ()
      .connect (sigc::mem_fun (*editor_, &editor::on_options_changed));
  }

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

  if (builder->get_object ("maintenance-button")) {
    builder->get_widget ("maintenance-button", maintenance_);
    if (maintenance_) {
      Glib::RefPtr<Gtk::Action> action;
      action = ui_manager_->get_action ("/dialog/maintenance");
      if (action) {
        action->connect_proxy (*maintenance_);
        action->set_sensitive (false);
        // action->signal_activate()d in dialog::on_device_changed()
      }
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

dialog::~dialog ()
{
  if (maintenance_dialog_)
    {
      maintenance_trigger_.disconnect ();
      delete maintenance_dialog_;
    }
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

    Glib::RefPtr< Gdk::Window > window = get_window ();
    if (window)
      {
        window->set_cursor ();
      }
  }
}

void
dialog::on_scan (void)
{
  file_chooser dialog (*this, _("Save As..."));

  fs::path default_name (std::string (_("Untitled")) + ".pdf");
  fs::path default_path (fs::current_path () / default_name);

  dialog.set_current_name (default_name.string ());
  dialog.set_filename (default_path.string ());
  dialog.set_single_image_mode (idevice_->is_single_image ());
  dialog.set_do_overwrite_confirmation ();

  dialog.show_all ();

  if (Gtk::RESPONSE_ACCEPT != dialog.run ()) return;

  std::string path (dialog.get_filename ());

  // Infer image format and check support

  std::string ext = fs::path (path).extension ().string ();
  std::string fmt;

  /**/ if (".pnm"  == ext) fmt = "PNM";
  else if (".png"  == ext) fmt = "PNG";
  else if (".jpg"  == ext) fmt = "JPEG";
  else if (".jpeg" == ext) fmt = "JPEG";
  else if (".pdf"  == ext) fmt = "PDF";
  else if (".tif"  == ext) fmt = "TIFF";
  else if (".tiff" == ext) fmt = "TIFF";
  else
    {
      BOOST_THROW_EXCEPTION
        (logic_error ("unsupported file format"));
    }

  // Create an output device

  path_generator gen (path);
  odevice::ptr odev;

  if (!gen)                     // single file
    {
      /**/ if ("TIFF" == fmt)
        {
          odev = make_shared< _out_::tiff_odevice > (path);
        }
      else if ("PDF" == fmt
               || idevice_->is_single_image ())
        {
          odev = make_shared< file_odevice > (path);
        }
      else
        {
          BOOST_THROW_EXCEPTION
            (logic_error ("single file not supported"));
        }
    }
  else                          // file per image
    {
      if ("TIFF" == fmt)
        {
          odev = make_shared< _out_::tiff_odevice > (gen);
        }
      else
        {
          odev = make_shared< file_odevice > (gen);
        }
    }

  stream::ptr str = make_shared< stream > ();

  {
    // Configure the filter chain

    using namespace _flt_;

    const std::string xfer_raw = "image/x-raster";
    const std::string xfer_jpg = "image/jpeg";
    std::string xfer_fmt = idevice_->get_context ().content_type ();

    bool bilevel = ((*opts_)["device/image-type"] == "Gray (1 bit)");

    toggle force_extent = true;
    quantity width  = -1.0;
    quantity height = -1.0;
    try
      {
        force_extent = value ((*opts_)["device/force-extent"]);
        width   = value ((*opts_)["device/br-x"]);
        width  -= value ((*opts_)["device/tl-x"]);
        height  = value ((*opts_)["device/br-y"]);
        height -= value ((*opts_)["device/tl-y"]);
      }
    catch (const std::out_of_range&)
      {
        force_extent = false;
        width  = -1.0;
        height = -1.0;
      }
    if (force_extent) force_extent = (width > 0 || height > 0);

    filter::ptr autocrop;
    if (opts_->count ("magick/automatic-scan-area"))
      {
        toggle t = value ((*opts_)["magick/automatic-scan-area"]);
        if (t)
          autocrop = make_shared< _flt_::autocrop > ();
      }
    if (autocrop) force_extent = false;

    if (autocrop)
      {
        (*autocrop->options ())["lo-threshold"] = value ((*opts_)["device/lo-threshold"]);
        (*autocrop->options ())["hi-threshold"] = value ((*opts_)["device/hi-threshold"]);
      }

    filter::ptr deskew;
    if (!autocrop && opts_->count ("magick/deskew"))
      {
        toggle t = value ((*opts_)["magick/deskew"]);
        if (t)
          deskew = make_shared< _flt_::deskew > ();
      }

    if (deskew)
      {
        (*deskew->options ())["lo-threshold"] = value ((*opts_)["device/lo-threshold"]);
        (*deskew->options ())["hi-threshold"] = value ((*opts_)["device/hi-threshold"]);
      }

    toggle resample = false;
    if (opts_->count ("device/enable-resampling"))
      resample = value ((*opts_)["device/enable-resampling"]);

    filter::ptr magick (make_shared< _flt_::magick > ());

    toggle bound = true;
    quantity res_x  = -1.0;
    quantity res_y  = -1.0;

    std::string sw (resample ? "device/sw-" : "device/");
    if (opts_->count (sw + "resolution-x"))
      {
        res_x = value ((*opts_)[sw + "resolution-x"]);
        res_y = value ((*opts_)[sw + "resolution-y"]);
      }
    if (opts_->count (sw + "resolution-bind"))
      bound = value ((*opts_)[sw + "resolution-bind"]);

    if (bound)
      {
        res_x = value ((*opts_)[sw + "resolution"]);
        res_y = value ((*opts_)[sw + "resolution"]);
      }

    (*magick->options ())["resolution-x"] = res_x;
    (*magick->options ())["resolution-y"] = res_y;
    (*magick->options ())["force-extent"] = force_extent;
    (*magick->options ())["width"]  = width;
    (*magick->options ())["height"] = height;

    (*magick->options ())["bilevel"] = toggle (bilevel);

    quantity thr = value ((*opts_)["device/threshold"]);
    thr *= 100.0;
    thr /= (dynamic_pointer_cast< range >
            ((*opts_)["device/threshold"].constraint ()))->upper ();
    (*magick->options ())["threshold"] = thr;

    (*magick->options ())["image-format"] = fmt;

    {
      toggle sw_color_correction = false;
      if (opts_->count ("device/sw-color-correction"))
        {
          sw_color_correction = value ((*opts_)["device/sw-color-correction"]);
          for (int i = 1; sw_color_correction && i <= 9; ++i)
            {
              key k ("cct-" + boost::lexical_cast< std::string > (i));
              (*magick->options ())[k] = value ((*opts_)["device"/k]);
            }
        }
      (*magick->options ())["color-correction"] = sw_color_correction;
    }

    toggle skip_blank = !bilevel; // \todo fix filter limitation
    quantity skip_thresh = -1.0;
    filter::ptr blank_skip (make_shared< image_skip > ());
    try
      {
        (*blank_skip->options ())["blank-threshold"]
          = value ((*opts_)["blank-skip/blank-threshold"]);
        skip_thresh = value ((*blank_skip->options ())["blank-threshold"]);
      }
    catch (const std::out_of_range&)
      {
        skip_blank = false;
        log::error ("Disabling blank skip functionality");
      }
    // Don't even try skipping of completely white images.  We are
    // extremely unlikely to encounter any of those.
    skip_blank = (skip_blank
                  && (quantity (0.) < skip_thresh));

    /**/ if (xfer_raw == xfer_fmt)
      {
        str->push (make_shared< padding > ());
      }
    else if (xfer_jpg == xfer_fmt)
      {
        str->push (make_shared< jpeg::decompressor> ());
      }
    else
      {
        log::alert
          ("unsupported transfer format: '%1%'") % xfer_fmt;

        BOOST_THROW_EXCEPTION
          (runtime_error
           ((format (_("conversion from %1% to %2% is not supported"))
             % xfer_fmt
             % fmt)
            .str ()));
      }

    if (skip_blank) str->push (blank_skip);
    str->push (make_shared< pnm > ());
    if (autocrop)   str->push (autocrop);
    if (deskew)     str->push (deskew);
    str->push (magick);

    if ("PDF" == fmt)
      {
        if (bilevel) str->push (make_shared< g3fax > ());
        str->push (make_shared< pdf > (gen));
      }
  }
  {
    str->push (odev);

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

    Glib::RefPtr< Gdk::Window > window = get_window ();
    if (window)
      {
        window->set_cursor (Gdk::Cursor (Gdk::WATCH));
        Gdk::flush ();
      }

    pump_->start (str);
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
dialog::on_device_changed (utsushi::scanner::ptr idev)
{
  idevice_ = idev;

  opts_.reset (new option::map);
  opts_->add_option_map () ("application", app_opts_);
  opts_->add_option_map () ("device", idevice_->options ());
  _flt_::image_skip skip;
  opts_->add_option_map () ("blank-skip", skip.options ());
  if (   idevice_->options ()->count ("lo-threshold")
      && idevice_->options ()->count ("hi-threshold"))
    {
      option::map::ptr opts (make_shared< option::map > ());
      if (HAVE_MAGICK_PP)
        {
          opts->add_options ()
            ("deskew", toggle (),
             attributes (tag::enhancement)(level::standard),
             N_("Deskew"));
        }

      if (HAVE_MAGICK_PP
          && idevice_->options ()->count ("scan-area"))
        {
          constraint::ptr c ((*idevice_->options ())["scan-area"]
                             .constraint ());
          if (value ("Automatic") != (*c) (value ("Automatic")))
            {
              dynamic_pointer_cast< store >
                (c)->alternative ("Automatic");
              opts->add_options ()
                ("automatic-scan-area", toggle ());
            }
        }

      opts_->add_option_map () ("magick", opts);
    }

  Glib::RefPtr<Gtk::Action> action;
  action = ui_manager_->get_action ("/dialog/maintenance");
  if (action)
    {
      if (maintenance_dialog_)
        {
          maintenance_trigger_.disconnect ();
          delete maintenance_dialog_;
        }
      maintenance_dialog_  = new action_dialog (idevice_->actions (),
                                                maintenance_);
      maintenance_trigger_ = action->signal_activate ()
        .connect (sigc::mem_fun (*maintenance_dialog_,
                                 &action_dialog::on_maintenance));
      action->set_sensitive (!idevice_->actions ()->empty ());
    }

  signal_options_changed_.emit (opts_);

  pump_ = make_shared< pump > (idev);

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
