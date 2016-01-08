//  pump.hpp -- move image octets from a source to a sink
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

#ifndef utsushi_pump_hpp_
#define utsushi_pump_hpp_

#include "device.hpp"
#include "log.hpp"
#include "option.hpp"
#include "signal.hpp"
#include "stream.hpp"

namespace utsushi {

//! Move image octets from a source to a sink
class pump
  : public configurable
{
public:
  typedef shared_ptr< pump > ptr;

  pump (idevice::ptr idev);

  virtual ~pump ();

  virtual void start (odevice::ptr odev);
  virtual void start (stream::ptr str);

  void cancel ();

  typedef signal< void (log::priority, std::string) > notify_signal_type;
  typedef signal< void () > cancel_signal_type;

  connection connect (const notify_signal_type::slot_type& slot) const;
  connection connect_cancel (const cancel_signal_type::slot_type& slot) const;

private:
  class impl;
  impl *pimpl_;
};

}       // namespace utsushi

#endif  /* utsushi_pump_hpp_ */
