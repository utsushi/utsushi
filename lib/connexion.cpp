//  connexion.cpp -- transport messages between software and device
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//  Copyright (C) 2011  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
//  Author : Olaf Meeuwissen
//  Origin : FreeRISCI
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

#include "utsushi/connexion.hpp"
#include "connexions/usb.hpp"

namespace utsushi
{
  connexion::ptr
  connexion::create (const std::string& type, const std::string& path)
  {
    /**/ if ("usb" == type)
      {
        // This will be replaced by code that searches for a USB
        // connexion plugin and makes its factory available for use
        // here.  When done, the connexions/usb.hpp include can be
        // removed as well.  For now, any applications that want to
        // create a USB connexion will need to link with a suitable
        // USB plugin.

        return libcnx_usb_LTX_factory (type, path);
      }
    else
      {
        std::cerr << "unsupported interface type" << std::endl;
      }
    return ptr ();
  }

decorator<connexion>::decorator (ptr instance)
  : instance_(instance)
{}

void
decorator<connexion>::send (const octet *message, streamsize size)
{
  instance_->send (message, size);
}

void
decorator<connexion>::recv (octet *message, streamsize size)
{
  instance_->recv (message, size);
}

} // namespace utsushi
