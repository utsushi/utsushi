//  pump.cpp -- move image data from a source to a sink
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#include <utsushi/functional.hpp>

#include "pump.hpp"

namespace utsushi {
namespace gtkmm {

pump::pump (idevice::ptr idev)
  : utsushi::pump (idev)
  , idev_ptr_(idev)
{
  connect_< input > (in, idev_ptr_);
}

pump::~pump ()
{
  acq_marker_connection_[in].disconnect ();
  gui_marker_connection_[in].disconnect ();
  acq_update_connection_[in].disconnect ();
  gui_update_connection_[in].disconnect ();

  acq_marker_connection_[out].disconnect ();
  gui_marker_connection_[out].disconnect ();
  acq_update_connection_[out].disconnect ();
  gui_update_connection_[out].disconnect ();

  acq_notify_connection_.disconnect ();
  gui_notify_connection_.disconnect ();
}

void
pump::start (odevice::ptr odev)
{
  if (odev) connect_< output > (out, odev);
  utsushi::pump::start (odev);
}

void
pump::start (stream::ptr str)
{
  if (str) connect_< output > (out, str->get_device ());
  utsushi::pump::start (str);
}

pump::marker_signal_type
pump::signal_marker (io_direction d)
{
  return gui_marker_signal_[d];
}

pump::update_signal_type
pump::signal_update (io_direction d)
{
  return gui_update_signal_[d];
}

pump::notify_signal_type
pump::signal_notify ()
{
  return gui_notify_signal_;
}

void
pump::signal_marker_(io_direction d)
{
  traits::int_type marker;
  {
    lock_guard< mutex > lock (marker_mutex_[d]);

    if (marker_queue_[d].empty ()) return;

    marker = marker_queue_[d].front ();
    marker_queue_[d].pop ();
  }

  gui_marker_signal_[d].emit (marker);

  if (   traits::eof () == marker
      || traits::eos () == marker)
    {
      disconnect_(d);
    }
}

void
pump::signal_update_(io_direction d)
{
  std::pair< streamsize, streamsize> update;
  {
    lock_guard< mutex > lock (update_mutex_[d]);

    if (update_queue_[d].empty ()) return;

    update = update_queue_[d].front ();
    update_queue_[d].pop ();
  }

  gui_update_signal_[d].emit (update.first, update.second);
}

void
pump::signal_notify_()
{
  std::pair< log::priority, std::string > notify;
  {
    lock_guard< mutex > lock (notify_mutex_);

    if (notify_queue_.empty ()) return;

    notify = notify_queue_.front ();
    notify_queue_.pop ();
  }

  gui_notify_signal_.emit (notify.first, notify.second);
}

template< typename IO >
void
pump::connect_(io_direction d, typename device< IO >::ptr dev_ptr)
{
  acq_marker_connection_[d] =
    dev_ptr->connect_marker (bind (&pump::on_marker_, this, d, _1));
  gui_marker_connection_[d] = gui_marker_dispatch_[d]
    .connect (sigc::bind (sigc::mem_fun (*this, &pump::signal_marker_), d));

  acq_update_connection_[d] =
    dev_ptr->connect_update (bind (&pump::on_update_, this, d, _1, _2));
  gui_update_connection_[d] = gui_update_dispatch_[d]
    .connect (sigc::bind (sigc::mem_fun (*this, &pump::signal_update_), d));

  if (in == d)
    {
      acq_notify_connection_ =
        this->connect (bind (&pump::on_notify_, this, _1, _2));
      gui_notify_connection_ = gui_notify_dispatch_
        .connect (sigc::mem_fun (*this, &pump::signal_notify_));
    }
}

void
pump::disconnect_(io_direction d)
{
  if (in == d) return;

  acq_marker_connection_[d].disconnect ();
  gui_marker_connection_[d].disconnect ();

  acq_update_connection_[d].disconnect ();
  gui_update_connection_[d].disconnect ();
}

void
pump::on_marker_(io_direction d, traits::int_type c)
{
  {
    lock_guard< mutex > lock (marker_mutex_[d]);
    marker_queue_[d].push (c);
  }

  gui_marker_dispatch_[d].emit ();
}

void
pump::on_update_(io_direction d, streamsize current, streamsize total)
{
  {
    lock_guard< mutex > lock (update_mutex_[d]);
    update_queue_[d].push (std::make_pair (current, total));
  }

  gui_update_dispatch_[d].emit ();
}

void
pump::on_notify_(log::priority level, std::string message)
{
  {
    lock_guard< mutex > lock (notify_mutex_);
    notify_queue_.push (std::make_pair (level, message));
  }

  gui_notify_dispatch_.emit ();
}

}       // namespace gtkmm
}       // namespace utsushi
