//  monitor.hpp -- available scanner devices
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#ifndef utsushi_monitor_hpp_
#define utsushi_monitor_hpp_

#include <istream>
#include <set>

#include "option.hpp"
#include "scanner.hpp"

namespace utsushi {

//! Check up on the available scanner devices
/*! Most @PACKAGE_NAME@ commands eventually want to do something with
 *  one or more scanner devices.  To establish initial contact with a
 *  scanner device, they can turn to the monitor.  This singleton is
 *  in charge of finding available devices, noticing when new devices
 *  become available and when devices go away.
 */
class monitor
  : public configurable
{
  typedef scanner::info key_type;

public:
  typedef std::set< key_type > container_type;
  typedef container_type::size_type size_type;
  typedef container_type::const_iterator const_iterator;

  monitor ();

  const_iterator begin () const;
  const_iterator end () const;

  //! Check whether scanner devices are available
  bool empty () const;

  //! See how many scanner devices are available
  size_type size () const;

  //! Determine how many scanner devices can possibly be stored
  size_type max_size () const;

  //! Locate a specific scanner device
  const_iterator find (const scanner::info& info) const;

  //! Find out whether a certain scanner device is available
  size_type count (const scanner::info& info) const;

  static container_type read (std::istream& istr);

  class impl;
};

}       // namespace utsushi

#endif  /* utsushi_monitor_hpp_ */
