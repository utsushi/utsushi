//  file-chooser.cpp -- select where and how to save scan results
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

#include "file-chooser.hpp"

#include <utsushi/format.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/regex.hpp>

#include <gtkmm/box.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/liststore.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>

#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>

#include <algorithm>
#include <vector>

namespace fs = boost::filesystem;

using boost::assign::list_of;

using std::count;
using std::vector;

namespace {

using utsushi::regex;

const std::string default_extension_ = ".pdf";
const std::string default_pattern_   = "-%i";

const regex filename_re ("(([^%]|%[^i])*?)"     // non-greedy
                         "(-*)"
                         "%0*([0-9]*)i"
                         "(([^%]|%[^i])*)");

bool
requests_single_file (const std::string& name)
{
  return (!regex_match (name, filename_re));
}

bool
supports_multi_image (const fs::path& name)
{
  fs::path ext (name.extension ());

  return (ext == ".pdf" || ext == ".tiff" || ext == ".tif");
}

struct model_columns
  : public Gtk::TreeModel::ColumnRecord
{
  typedef vector< std::string > extension_list_;

  model_columns ()
  {
    add (text);
    add (exts);
  }

  Gtk::TreeModelColumn< std::string >     text;
  Gtk::TreeModelColumn< extension_list_ > exts;
};

typedef model_columns::extension_list_ extension_list;

const model_columns *column (nullptr);

const bool use_markup = false;
const bool modal      = true;

}       // namespace

namespace utsushi {
namespace gtkmm {

file_chooser::file_chooser (Gtk::Window& parent, const std::string& title)
  : Gtk::Dialog (title, parent, modal)
  , impl_(Gtk::FILE_CHOOSER_ACTION_SAVE)
{
  common_ctor_logic_();
}

file_chooser::file_chooser (const std::string& title)
  : Gtk::Dialog (title, modal)
  , impl_(Gtk::FILE_CHOOSER_ACTION_SAVE)
{
  common_ctor_logic_();
}

file_chooser::~file_chooser ()
{
  if (watch_thread_)
    {
      cancel_watch_ = true;
      watch_thread_->join ();
      delete watch_thread_;
    }
}

bool
file_chooser::get_do_overwrite_confirmation () const
{
  return do_overwrite_confirmation_;
}

void
file_chooser::set_do_overwrite_confirmation (bool do_overwrite_confirmation)
{
  do_overwrite_confirmation_ = do_overwrite_confirmation;
}

std::string
file_chooser::get_current_name () const
{
  return fs::path (get_filename ()).filename ().string ();
}

void
file_chooser::set_current_name (const std::string& name)
{
  if (name == get_current_name ()) return;

  impl_.set_current_name (name);

  set_filename (get_filename ());
}

std::string
file_chooser::get_current_extension () const
{
  return fs::path (get_current_name ()).extension ().string ();
}

void
file_chooser::set_current_extension (const std::string& extension)
{
  if (extension == get_current_extension ()) return;

  set_current_name (fs::path (get_current_name ())
                    .replace_extension (extension).string ());
}

std::string
file_chooser::get_filename () const
{
  lock_guard< mutex > lock (filename_mutex_);
  return Glib::filename_to_utf8 (impl_.get_filename ());
}

bool
file_chooser::set_filename (const std::string& filename)
{
  lock_guard< mutex > lock (filename_mutex_);
  return impl_.set_filename (Glib::filename_from_utf8 (filename));
}

std::string
file_chooser::get_current_folder () const
{
  return impl_.get_current_folder ();
}

bool
file_chooser::set_current_folder (const std::string& filename)
{
  return impl_.set_current_folder (filename);
}

void
file_chooser::add_filter (const Gtk::FileFilter& filter)
{
  impl_.add_filter (filter);
}

void
file_chooser::remove_filter (const Gtk::FileFilter& filter)
{
  impl_.remove_filter (filter);
}

bool
file_chooser::get_single_image_mode () const
{
  return single_image_mode_;
}

void
file_chooser::set_single_image_mode (bool single_image_mode)
{
  if (single_image_mode == get_single_image_mode ()) return;

  single_image_mode_ = single_image_mode;

  if (single_image_mode_)
    single_file_.hide ();
  else
    single_file_.show ();
}

void
file_chooser::show_all ()
{
  impl_.show_all ();
}

void
file_chooser::on_response (int response_id)
{
  if (Gtk::RESPONSE_ACCEPT != response_id) return;

  if (get_current_extension ().empty ())
    set_current_extension (default_extension_);

  std::string fmt;
  {                             // check whether extension is known
    std::string ext (get_current_extension ());
    bool found = false;
    Gtk::TreeModel::Children children (file_type_.get_model ()->children ());

    for (Gtk::TreeModel::Children::const_iterator it = children.begin ();
         !found && children.end () != it;
         ++it)
      {
        Gtk::TreeModel::Row r = *it;
        extension_list      l = r[column->exts];

        found = count (l.begin (), l.end (), ext);

        if (found) fmt = r[column->text];
      }

    if (!found)
      {
        Gtk::MessageDialog tbd
          (*this, SEC_("Unsupported file format."),
           use_markup, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, modal);

        tbd.set_secondary_text
          ((format (SEC_("The '%1%' file extension is not associated with"
                         " a supported file format.  Please select a file"
                         " format or use one of the known file extensions."))
            % ext).str ());

        if (dynamic_cast< Gtk::Window * > (this))
          get_group ()->add_window (tbd);

        tbd.run ();
        signal_response ().emission_stop ();
        response (Gtk::RESPONSE_CANCEL);
        return;
      }
  }
  if (!single_image_mode_
      && requests_single_file (get_current_name ()))
    {                           // check whether single file is okay
      if (!supports_multi_image (get_current_name ()))
        {
          Gtk::MessageDialog tbd
            (*this, (format (SEC_("The %1% format does not support multiple"
                                  " images in a single file.")) % fmt).str (),
             use_markup, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, modal);

          tbd.set_secondary_text
            ((format (SEC_("Please save to PDF or TIFF if you want a single"
                           " file.  If you prefer the %1% image format, use"
                           " a filename such as 'Untitled-%%3i%2%'."))
              % fmt % get_current_extension ()).str ());

          if (dynamic_cast< Gtk::Window * > (this))
            get_group ()->add_window (tbd);

          tbd.run ();
          signal_response ().emission_stop ();
          response (Gtk::RESPONSE_CANCEL);
          return;
        }
    }

  if (!do_overwrite_confirmation_) return;

  format message;
  format details;

  if (requests_single_file (get_current_name ()))
    {
      if (!fs::exists (std::string (get_filename ()))) return;

      message = format (SEC_("The name \"%1%\" already exists.\n"
                             "OK to overwrite this name using the new"
                             " settings?"));
      details = format (SEC_("The file already exists in \"%1%\"."
                             "  Replacing it will overwrite its contents."));
    }
  else
    {
      // FIXME Add meaningful checking
      // if (no_possible_matches ) return;

      message = format (SEC_("Files matching \"%1%\" may already exist."
                             "  Do you want to replace them?"));
    //details = format (CCB_("These files already exist in \"%1%\"."
    //                       "  Replacing them may overwrite their "
    //                       "contents."));

      // FIXME show list of matching files in an expander with details
    }

  message % get_current_name ();
  if (0 < details.size ())
    details % get_current_folder ();

  Gtk::MessageDialog tbd (*this, message.str (), use_markup,
                          Gtk::MESSAGE_QUESTION,
                          Gtk::BUTTONS_NONE, modal);

  if (0 < details.size ())
    tbd.set_secondary_text (details.str ());
  tbd.add_button (Gtk::Stock::NO , Gtk::RESPONSE_CANCEL);
  tbd.add_button (Gtk::Stock::YES, Gtk::RESPONSE_ACCEPT);
  tbd.set_default_response (Gtk::RESPONSE_ACCEPT);

  if (dynamic_cast< Gtk::Window * > (this))
    get_group ()->add_window (tbd);

  if (Gtk::RESPONSE_ACCEPT != tbd.run ())
    {
      signal_response ().emission_stop ();
      response (Gtk::RESPONSE_CANCEL);
    }
}

void
file_chooser::on_file_type_changed ()
{
  Glib::RefPtr< Gtk::TreeSelection > s (file_type_.get_selection ());
  if (!s) return;

  Gtk::TreeModel::iterator it (s->get_selected ());
  if (!it) return;

  Gtk::TreeModel::Row r (*it);
  extension_list      l (r[column->exts]);

  if (l.empty ())
    {
      expander_.set_label (SEC_("File Type"));
    }
  else
    {
      expander_.set_label ((format (SEC_("File type: %1%"))
                            % r.get_value (column->text)).str ());

      if (!count (l.begin (), l.end (), get_current_extension ()))
        set_current_extension (l.front ());
    }

  if (!single_image_mode_)
    {
      single_file_.set_sensitive (supports_multi_image (get_current_name ()));
      if (!supports_multi_image (get_current_name ()))
        {
          if (!regex_match (get_current_name (), filename_re))
            {
              fs::path path (get_current_name ());
              fs::path stem (path.stem ());
              fs::path ext  (path.extension ());

              path = stem;
              path = path.native () + default_pattern_;
              path.replace_extension (ext);

              set_current_name (path.string ());
            }
        }
      single_file_.set_active (requests_single_file (get_current_name ()));
    }
}

void
file_chooser::on_single_file_toggled ()
{
  std::string name (get_current_name ());
  smatch m;

  if (regex_match (name, m, filename_re))
    {
      if (!single_file_.get_active ()) return;

      set_current_name (m.str (1) + m.str (5));
    }
  else
    {
      if (single_file_.get_active ()) return;

      fs::path path (get_current_name ());
      fs::path stem (path.stem ());
      fs::path ext  (path.extension ());

      path = stem;
      path = path.native () + default_pattern_;
      path.replace_extension (ext);

      set_current_name (path.string ());
    }
}

void
file_chooser::common_ctor_logic_()
{
  do_overwrite_confirmation_ = true;
  single_image_mode_         = false;

  if (!column) column = new model_columns;

  Glib::RefPtr< Gtk::ListStore > types;
  types = Gtk::ListStore::create (*column);
  {
    Gtk::TreeModel::Row r;
    extension_list      l;

    r = *(types->append ());
    r[column->text] = SEC_("By extension");
    r[column->exts] = l;

    r = *(types->append ());
    r[column->text] = CCB_("JPEG");
#if __cplusplus >= 201103L
    l = {".jpeg", ".jpg"};
#else
    l = list_of (".jpeg")(".jpg");
#endif
    r[column->exts] = l;

    r = *(types->append ());
    r[column->text] = CCB_("PDF");
#if __cplusplus >= 201103L
    l = {".pdf"};
#else
    l = list_of (".pdf");
#endif
    r[column->exts] = l;

    r = *(types->append ());
    r[column->text] = CCB_("PNG");
#if __cplusplus >= 201103L
    l = {".png"};
#else
    l = list_of (".png");
#endif
    r[column->exts] = l;

    r = *(types->append ());
    r[column->text] = CCB_("PNM");
#if __cplusplus >= 201103L
    l = {".pnm"};
#else
    l = list_of (".pnm");
#endif
    r[column->exts] = l;

    r = *(types->append ());
    r[column->text] = CCB_("TIFF");
#if __cplusplus >= 201103L
    l = {".tiff", ".tif"};
#else
    l = list_of (".tiff")(".tif");
#endif
    r[column->exts] = l;
  }

  file_type_.set_model (types);
  file_type_.set_headers_visible (false);
  file_type_.append_column ("", column->text);
  file_type_.set_rules_hint ();
  file_type_.get_selection ()->signal_changed ()
    .connect (sigc::mem_fun (*this, &file_chooser::on_file_type_changed));

  expander_.set_label (SEC_("File Type"));
  expander_.add (file_type_);
  expander_.set_expanded ();

  single_file_.set_label (SEC_("Save all images in a single file"));
  single_file_.signal_toggled ()
    .connect (sigc::mem_fun (*this, &file_chooser::on_single_file_toggled));

  Gtk::VBox *extras (Gtk::manage (new Gtk::VBox));
  extras->pack_start (expander_);
  extras->pack_start (single_file_);

  // Set up the dialog, using layout values from Gtk::FileChooserDialog
  set_has_separator (false);
  set_border_width (5);
  get_action_area ()->set_border_width (5);

  Gtk::Box *content_area = get_vbox ();
  content_area->set_spacing (2);
  content_area->pack_start (impl_  , Gtk::PACK_EXPAND_WIDGET);
  content_area->pack_start (*extras, Gtk::PACK_SHRINK);
  content_area->show_all ();
  {
    // FIXME determine the default width and height in a way similar
    //       to how Gtk::FileChooserDialog does.  Its implementation
    //       uses an internal function which takes font information
    //       and numbers of characters and lines into account.
    //       See file_chooser_widget_default_size_changed()
    int width = 800, height = 600;
    set_default_size (width, height);
  }

  add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button (Gtk::Stock::OK    , Gtk::RESPONSE_ACCEPT);

  // FIXME leave this to the "application" or tie it to file_type_
  {
    Gtk::FileFilter filter;
    filter.add_mime_type ("application/pdf");
    filter.add_mime_type ("image/*");
    filter.set_name (SEC_("PDFs and Image Files"));
    add_filter (filter);
  }
  {
    Gtk::FileFilter filter;
    filter.add_mime_type ("image/*");
    filter.set_name (SEC_("Image Files"));
    add_filter (filter);
  }
  {
    Gtk::FileFilter filter;
    filter.add_pattern ("*");
    filter.set_name (SEC_("All Files"));
    add_filter (filter);
  }

  gui_name_change_dispatch_
    .connect (sigc::mem_fun (*this, &file_chooser::signal_name_change_));
  signal_name_change ()
    .connect (sigc::mem_fun (*this, &file_chooser::on_name_change_));

  cancel_watch_ = false;
  watch_thread_ = new thread (&file_chooser::watch_, this);
}

void
file_chooser::watch_()
{
  // As long as the user cannot notice any discrepancy between the
  // file name, format and single file toggle, any value of t will
  // be just fine.  A value of 100 ms appears to be good enough.

  struct timespec t = { 0, 100000000 /* ns */ };

  while (!cancel_watch_ && 0 == nanosleep (&t, 0))
    {
      std::string name (get_current_name ());
      if (name == cached_name_) continue;

      cached_name_ = name;
      {
        lock_guard< mutex > lock (name_change_mutex_);
        name_change_queue_.push (cached_name_);
      }
      gui_name_change_dispatch_.emit ();
    }
}

file_chooser::name_change_signal_type
file_chooser::signal_name_change ()
{
  return gui_name_change_signal_;
}

void
file_chooser::signal_name_change_()
{
  std::string name;
  {
    lock_guard< mutex > lock (name_change_mutex_);

    if (name_change_queue_.empty ()) return;

    name = name_change_queue_.front ();
    name_change_queue_.pop ();
  }

  signal_name_change ().emit (name);
}

void
file_chooser::on_name_change_(const std::string& name)
{
  if (!single_image_mode_)
    {
      single_file_.set_sensitive (supports_multi_image (name));
      single_file_.set_active    (requests_single_file (name));
    }

  Glib::RefPtr< Gtk::TreeSelection > s (file_type_.get_selection ());
  if (!s) return;

  Gtk::TreeModel::iterator it (s->get_selected ());
  if (!it) return;

  Gtk::TreeModel::Row r (*it);
  extension_list      l (r[column->exts]);

  if (!count (l.begin (), l.end (), get_current_extension ()))
    {
      s->unselect (it);
      expander_.set_label (SEC_("File Type"));
    }

  // FIXME add filename sanity checking
  //       Any warnings should not interrupt the user's file
  //       name entry.  One idea would be to change the file
  //       entry widgets background color to indicate error.
}

}       // namespace gtkmm
}       // namespace utsushi
