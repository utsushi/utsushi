//  pump.hpp -- move image octets from a source to a sink
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

#ifndef gtkmm_pump_hpp_
#define gtkmm_pump_hpp_

#include <queue>

#include <glibmm/dispatcher.h>

#include <utsushi/pump.hpp>
#include <utsushi/mutex.hpp>

namespace utsushi {
namespace gtkmm {

//! \copybrief utsushi::pump
/*! This subclass adds the plumbing needed to transport signals from
 *  the image data acquisition thread to the GUI thread.
 */
class pump
  : public utsushi::pump
{
public:
  typedef shared_ptr< pump > ptr;

  pump (idevice::ptr idev);

  ~pump ();

  void start (odevice::ptr odev);
  void start (stream::ptr str);

  typedef sigc::signal< void, traits::int_type >           marker_signal_type;
  typedef sigc::signal< void, streamsize, streamsize >     update_signal_type;
  typedef sigc::signal< void, log::priority, std::string > notify_signal_type;

  enum io_direction {
    in,
    out,
  };

  // GUI thread signal accessors
  marker_signal_type signal_marker (io_direction d);
  update_signal_type signal_update (io_direction d);
  notify_signal_type signal_notify ();

protected:
  // GUI Thread Signal Emitters
  void signal_marker_(io_direction d);
  void signal_update_(io_direction d);
  void signal_notify_();

  // Acquisition Thread Slots
  void on_marker_(io_direction d, traits::int_type c);
  void on_update_(io_direction d, streamsize current, streamsize total);
  void on_notify_(log::priority level, std::string message);

  // Internal Connection Management
  template< typename IO >
  void connect_(io_direction d, typename device< IO >::ptr dev_ptr);
  void disconnect_(io_direction d);

  device< input >::ptr idev_ptr_;

  // Internal Connections
  sigc::connection gui_marker_connection_[2];
  connection       acq_marker_connection_[2];
  sigc::connection gui_update_connection_[2];
  connection       acq_update_connection_[2];
  sigc::connection gui_notify_connection_;
  connection       acq_notify_connection_;

  // Signal Dispatchers
  Glib::Dispatcher gui_marker_dispatch_[2];
  Glib::Dispatcher gui_update_dispatch_[2];
  Glib::Dispatcher gui_notify_dispatch_;

  // GUI Thread Signals
  marker_signal_type gui_marker_signal_[2];
  update_signal_type gui_update_signal_[2];
  notify_signal_type gui_notify_signal_;

  // Signal Data Queues
  std::queue< traits::int_type >                        marker_queue_[2];
  std::queue< std::pair< streamsize, streamsize > >     update_queue_[2];
  std::queue< std::pair< log::priority, std::string > > notify_queue_;

  // Data Queue Access Synchronization
  mutex marker_mutex_[2];
  mutex update_mutex_[2];
  mutex notify_mutex_;
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_pump_hpp_ */
