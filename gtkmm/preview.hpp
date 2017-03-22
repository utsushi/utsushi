//  preview.hpp -- display/control preview images before final acquisition
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef gtkmm_preview_hpp_
#define gtkmm_preview_hpp_

#include <gdkmm/pixbuf.h>
#include <gdkmm/pixbufloader.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/uimanager.h>

#include <utsushi/context.hpp>
#include <utsushi/device.hpp>
#include <utsushi/option.hpp>
#include <utsushi/scanner.hpp>
#include <utsushi/stream.hpp>

namespace utsushi {
namespace gtkmm {

class preview : public Gtk::HBox, public odevice
{
  typedef Gtk::HBox base;

  double zoom_, step_;
  double zoom_min_, zoom_max_;

  Gdk::InterpType interp_;

  Glib::RefPtr<Gdk::PixbufLoader> loader_;
  Glib::RefPtr<Gdk::Pixbuf>       pixbuf_;
  Glib::RefPtr<Gdk::Pixbuf>       scaled_pixbuf_;
  Gtk::Image                      image_;
  Gtk::EventBox                   event_box_;
  Gtk::ScrolledWindow            *window_;
  Glib::RefPtr<Gtk::UIManager>    ui_;

  idevice::ptr  idevice_;
  odevice::ptr  odevice_;
  stream::ptr   stream_;

  option::map::ptr opts_;
  option::map::ptr ui_control_;

public:
  preview (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder);

  virtual streamsize write (const octet *data, streamsize n);

  virtual void boi (const context& ctx);
  virtual void eoi (const context& ctx);

  void on_device_changed (scanner::ptr s);
  void on_values_changed (option::map::ptr om);
  void on_refresh ();

protected:
  void set_sensitive ();

  void scale ();

  double get_zoom_factor (double width, double height);

  // signal handlers
  void on_area_prepared ();
  void on_area_updated (int x, int y, int width, int height);

  void on_zoom_in  ();
  void on_zoom_out ();
  void on_zoom_100 ();
  void on_zoom_fit ();

  bool on_expose_event (GdkEventExpose *event);
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* !defined (gtkmm_preview_hh) */
