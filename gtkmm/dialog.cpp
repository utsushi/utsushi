//  dialog.cpp -- to acquire image data
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2015  Olaf Meeuwissen
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

#include <iostream>
#include <stdexcept>
#include <string>
#include <set>

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
#if HAVE_LIBJPEG
#include "../filters/jpeg.hpp"
#endif
#include "../filters/magick.hpp"
#include "../filters/padding.hpp"
#include "../filters/pdf.hpp"
#include "../filters/pnm.hpp"
#include "../filters/reorient.hpp"
#if HAVE_LIBTIFF
#include "../outputs/tiff.hpp"
#endif

#include "action-dialog.hpp"
#include "chooser.hpp"
#include "file-chooser.hpp"
#include "dialog.hpp"
#include "editor.hpp"
#include "presets.hpp"
#include "preview.hpp"

namespace fs = boost::filesystem;

namespace utsushi {
namespace gtkmm {

using std::logic_error;
using std::runtime_error;

#define SCANNING  SEC_N_("Scanning...")
#define CANCELING SEC_N_("Canceling...")

dialog::dialog (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder)
  : base (ptr), opts_ (new option::map), app_opts_ (new option::map)
  , maintenance_(nullptr)
  , maintenance_dialog_(nullptr)
  , progress_(nullptr)
  , scan_started_(false)
  , ignore_delete_event_(false)
  , revert_overscan_(false)
{
  Glib::RefPtr<Glib::Object> obj = builder->get_object ("uimanager");
  ui_manager_ = Glib::RefPtr<Gtk::UIManager>::cast_dynamic (obj);

  if (!ui_manager_)
    BOOST_THROW_EXCEPTION
      (logic_error ("Dialog specification requires a 'uimanager'"));

  //  set up custom child widgets

  preview *preview = 0;
  {
    builder->get_widget_derived ("scanner-list", chooser_);
    chooser_->unreference ();
    chooser_->signal_device_changed ()
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
    chooser_->signal_device_changed ()
      .connect (sigc::mem_fun (*preview, &preview::on_device_changed));
  }
  {
    builder->get_widget_derived ("editor-pane", editor_);
    editor_->unreference ();
    signal_options_changed ()
      .connect (sigc::mem_fun (*editor_, &editor::on_options_changed));
    editor_->signal_values_changed ()
      .connect (sigc::mem_fun (*preview, &preview::on_values_changed));
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

  Gtk::Button *quit = 0;
  if (builder->get_object ("quit-button")) {
    builder->get_widget ("quit-button", quit);
    if (quit) {
      Glib::RefPtr<Gtk::Action> action;
      action = ui_manager_->get_action ("/dialog/quit");
      if (action) {
        action->connect_proxy (*quit);
        action->signal_activate ()
          .connect (sigc::mem_fun (*this, &Gtk::Widget::hide));
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
      if (!quit) {              // overload cancel button to quit
        cancel_ = action->signal_activate ()
          .connect (sigc::mem_fun (*this, &Gtk::Widget::hide));
      } else {
        action->set_sensitive (false);
        cancel_ = action->signal_activate ()
          .connect (sigc::mem_fun (*this, &dialog::on_cancel));
      }
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

  if (builder->get_object ("progress-indicator")) {
    Glib::RefPtr< Pango::Layout > layout;
    int height, w, h;

    builder->get_widget ("progress-indicator", progress_);
    progress_->get_size_request (w, height);

    layout = progress_->create_pango_layout ("");
    layout->get_pixel_size (w, h);
    if (h > height) height = h;
    layout->set_text (SEC_(SCANNING));
    layout->get_pixel_size (w, h);
    if (h > height) height = h;
    layout->set_text (SEC_(CANCELING));
    layout->get_pixel_size (w, h);
    if (h > height) height = h;
    progress_->set_size_request (-1, height);

    progress_->set_text ("");
    progress_->set_fraction (0);
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

sigc::signal< void, option::map::ptr, const std::set<std::string>& >
dialog::signal_options_changed ()
{
  return signal_options_changed_;
}

void
dialog::set_sensitive (void)
{
  Glib::RefPtr<Gtk::Action> a;

  a = ui_manager_->get_action ("/dialog/scan");
  if (a) { a->set_sensitive (bool(idevice_)); }
}

void
dialog::rewire_dialog (bool scanning)
{
  // switch to/from a busy cursor
  Glib::RefPtr< Gdk::Window > window = get_window ();
  if (window) {
    if (scanning)
      window->set_cursor (Gdk::Cursor (Gdk::WATCH));
    else
      window->set_cursor ();
  }

  // stop and clear progress bar
  if (progress_) {
    if (scanning) {
      progress_->set_text (SEC_(SCANNING));
      progress_pulse_ = Glib::signal_timeout ()
        .connect (sigc::mem_fun (*this, &dialog::on_timeout), 50);
    }
    else {
      progress_pulse_.disconnect ();
      progress_->set_text ("");
      progress_->set_fraction (0);
    }
  }

  // toggle sensitivity of the editor pane and the preview, cancel,
  // scan and quit buttons as well as the device chooser
  chooser_->set_sensitive (!scanning);
  editor_->set_sensitive (!scanning);
  Glib::RefPtr<Gtk::Action> action;
  action = ui_manager_->get_action ("/preview/refresh");
  if (action) action->set_sensitive (!scanning);
  action = ui_manager_->get_action ("/dialog/scan");
  if (action) action->set_sensitive (!scanning);
  action = ui_manager_->get_action ("/dialog/quit");
  if (action) {                 // quit button
    action->set_sensitive (!scanning);
    action = ui_manager_->get_action ("/dialog/cancel");
    if (action) action->set_sensitive (scanning);
  }
  else {                        // overloaded cancel button
    action = ui_manager_->get_action ("/dialog/cancel");
    if (action) {
      cancel_.disconnect ();
      cancel_ = action->signal_activate ()
        .connect (sigc::mem_fun
                  (*this, (scanning
                           ? &dialog::on_cancel : &Gtk::Widget::hide)));
    }
  }
  ignore_delete_event_ = scanning;
}

bool
dialog::on_delete_event (GdkEventAny *event)
{
  // FIXME should cancel and quit the dialog cleanly
  return ignore_delete_event_;
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
dialog::on_scan (void)
{
  file_chooser dialog (*this, SEC_("Save As..."));

  fs::path default_name (std::string (SEC_("Untitled")) + ".pdf");
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
  else if (HAVE_MAGICK  && ".png"  == ext) fmt = "PNG";
  else if (HAVE_LIBJPEG && ".jpg"  == ext) fmt = "JPEG";
  else if (HAVE_LIBJPEG && ".jpeg" == ext) fmt = "JPEG";
  else if (".pdf"  == ext) fmt = "PDF";
  else if (HAVE_LIBTIFF && ".tif"  == ext) fmt = "TIFF";
  else if (HAVE_LIBTIFF && ".tiff" == ext) fmt = "TIFF";
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
#if HAVE_LIBTIFF
      /**/ if ("TIFF" == fmt)
        {
          odev = make_shared< _out_::tiff_odevice > (path);
        }
      else
#endif
      /**/ if ("PDF" == fmt
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
#if HAVE_LIBTIFF
      if ("TIFF" == fmt)
        {
          odev = make_shared< _out_::tiff_odevice > (gen);
        }
      else
#endif
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
    revert_overscan_ = false;
    if (HAVE_MAGICK_PP
        && opts_->count ("doc-locate/crop"))
      {
        toggle t = value ((*opts_)["doc-locate/crop"]);
        if (t)
          {
            if (opts_->count ("device/overscan"))
              {
                t = value ((*opts_)["device/overscan"]);
                if (!t)
                  {
                    (*opts_)["device/overscan"] = toggle (true);
                    revert_overscan_ = true;
                  }
              }
            autocrop = make_shared< _flt_::autocrop > ();
          }
      }

    if (autocrop)
      {
        (*autocrop->options ())["lo-threshold"] = value ((*opts_)["device/lo-threshold"]);
        (*autocrop->options ())["hi-threshold"] = value ((*opts_)["device/hi-threshold"]);
      }

    filter::ptr deskew;
    if (HAVE_MAGICK_PP
        && !autocrop && opts_->count ("doc-locate/deskew"))
      {
        toggle t = value ((*opts_)["doc-locate/deskew"]);

        if (opts_->count ("device/long-paper-mode")
            && (value (toggle (true))
                == (*opts_)["device/long-paper-mode"]))
            t = false;

        if (t)
          deskew = make_shared< _flt_::deskew > ();
      }

    if (deskew)
      {
        (*deskew->options ())["lo-threshold"] = value ((*opts_)["device/lo-threshold"]);
        (*deskew->options ())["hi-threshold"] = value ((*opts_)["device/hi-threshold"]);
      }

    if (HAVE_MAGICK_PP
        && opts_->count ("device/long-paper-mode"))
      {
        string s = value ((*opts_)["device/doc-source"]);
        toggle t = value ((*opts_)["device/long-paper-mode"]);
        if (s == "ADF" && t && opts_->count ("device/scan-area"))
          {
            t = ((((*opts_)["device/scan-area"]) == value ("Auto Detect"))
                 || (// because we may be emulating "Auto Detect" ourselves
                     opts_->count ("doc-locate/crop")
                     && (*opts_)["doc-locate/crop"] == value (toggle (true))));
            if (t && !autocrop)
              autocrop = make_shared< _flt_::autocrop > ();
            if (t)
              (*autocrop->options ())["trim"] = t;
          }
      }
    if (autocrop) force_extent = false;

    filter::ptr reorient;
    if (opts_->count ("magick/reorient/rotate"))
      {
        value angle = value ((*opts_)["magick/reorient/rotate"]);
        reorient = make_shared< _flt_::reorient > ();
        (*reorient->options ())["rotate"] = angle;
      }

    toggle resample = false;
    if (opts_->count ("device/enable-resampling"))
      resample = value ((*opts_)["device/enable-resampling"]);

    filter::ptr magick;
    bool bilevel = false;
    if (HAVE_MAGICK)
      {
        magick = make_shared< _flt_::magick > ();
      }

    if (magick)
      {
        string type = value ((*opts_)["magick/image-type"]);
        bilevel = (type == "Monochrome");
        if (bilevel)                // use software thresholding
          {
            type = "Grayscale";
          }
        try {
          (*opts_)["device/image-type"] = type;
        }
        catch (const std::out_of_range&){}

        if (reorient)
          {
            (*magick->options ())["auto-orient"] = toggle (true);
          }

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

        quantity threshold = value ((*opts_)["magick/threshold"]);
        quantity brightness = value ((*opts_)["magick/brightness"]);
        quantity contrast   = value ((*opts_)["magick/contrast"]);
        (*magick->options ())["threshold"] = threshold;
        (*magick->options ())["brightness"] = brightness;
        (*magick->options ())["contrast"]   = contrast;

        (*magick->options ())["image-format"] = fmt;
      }
    else
    {
      if (opts_->count ("device/image-type"))
        bilevel = ((*opts_)["device/image-type"] == "Monochrome");
    }

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
    if (magick)
      skip_blank = true;
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
#if HAVE_LIBJPEG
    else if (xfer_jpg == xfer_fmt)
      {
        str->push (make_shared< jpeg::decompressor> ());
      }
#endif
    else
      {
        log::alert
          ("unsupported transfer format: '%1%'") % xfer_fmt;

        BOOST_THROW_EXCEPTION
          (runtime_error
           ((format (SEC_("conversion from %1% to %2% is not supported"))
             % xfer_fmt
             % fmt)
            .str ()));
      }

    if (skip_blank)  str->push (blank_skip);
    str->push (make_shared< pnm > ());
    if (autocrop)    str->push (autocrop);
    if (deskew)      str->push (deskew);
    if (reorient)    str->push (reorient);
    if (magick)      str->push (magick);

    if ("PDF" == fmt)
      {
        if (bilevel) str->push (make_shared< g3fax > ());
        str->push (make_shared< pdf > (gen));
      }
  }

  // Make pump just before starting scan
  pump_ = make_shared< pump > (idevice_);

  pump_->signal_marker (pump::in)
    .connect (sigc::mem_fun (*this, &dialog::on_scan_update));
  pump_->signal_marker (pump::out)
    .connect (sigc::mem_fun (*this, &dialog::on_scan_update));
  pump_->signal_notify ()
    .connect (sigc::mem_fun (*this, &dialog::on_notify));

  {
    scan_started_ = false;
    rewire_dialog (true);
    str->push (odev);
    pump_->start (str);
  }
}

void
dialog::on_scan_update (traits::int_type c)
{
  if (traits::bos () == c)
    {
      scan_started_ = true;
    }
  if (traits::eos () == c || traits::eof () == c)
    {
      if (revert_overscan_)
        {
          (*opts_)["device/overscan"] = toggle (false);
          revert_overscan_ = false;
        }
      rewire_dialog (false);
      scan_started_ = false;
    }
}

void
dialog::on_cancel (void)
{
  pump_->cancel ();
  if (progress_) { progress_->set_text (SEC_(CANCELING)); }
  if (scan_started_) {
    // The (traits::eof() == c) branch of dialog::on_scan_update()
    // will call rewire_dialog() eventually.
  } else {
    rewire_dialog (false);
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
  std::set<std::string> option_blacklist;

  idevice_ = idev;

  opts_.reset (new option::map);
  opts_->add_option_map () ("application", app_opts_);
  opts_->add_option_map () ("device", idevice_->options ());
  _flt_::image_skip skip;
  opts_->add_option_map () ("blank-skip", skip.options ());
  option::map::ptr opts (make_shared< option::map > ());
  if (HAVE_MAGICK_PP
      && idevice_->options ()->count ("lo-threshold")
      && idevice_->options ()->count ("hi-threshold"))
    {
      if (idevice_->options ()->count ("scan-area"))
        {
          constraint::ptr c ((*idevice_->options ())["scan-area"]
                             .constraint ());
          if (value ("Auto Detect") != (*c) (value ("Auto Detect")))
            {
              dynamic_pointer_cast< store >
                (c)->alternative ("Auto Detect");
              opts->add_options ()
                ("crop", toggle ());
            }
        }

      if (!idevice_->options ()->count ("deskew"))
        {
          opts->add_options ()
            ("deskew", toggle (),
             attributes (tag::enhancement)(level::standard),
             SEC_N_("Deskew"));
        }
    }

  opts_->add_option_map () ("doc-locate", opts);

  filter::ptr magick;
  if (HAVE_MAGICK)
    {
      magick = make_shared< _flt_::magick > ();
    }
  if (magick)
    {
      option::map::ptr opts = magick->options ();
      opts->add_options ()
        ("image-type", (from< utsushi::store > ()
                        -> alternative (SEC_N_("Monochrome"))
                        -> alternative (SEC_N_("Grayscale"))
                        -> default_value (SEC_N_("Color"))),
         attributes (tag::general)(level::standard),
         SEC_N_("Image Type"))
        ;
      option_blacklist.insert ("device/image-type");

      option_blacklist.insert ("device/threshold");

      filter::ptr reorient;
      if (magick->options ()->count ("auto-orient"))
        {
          reorient = make_shared< _flt_::reorient > ();
          opts->add_option_map () ("reorient", reorient->options ());
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

  signal_options_changed_.emit (opts_, option_blacklist);


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

bool
dialog::on_timeout (void)
{
  if (progress_) {
    progress_->pulse ();
  }
  return true;
}

}       // namespace gtkmm
}       // namespace utsushi
