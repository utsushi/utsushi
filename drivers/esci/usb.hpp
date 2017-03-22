//  usb.hpp -- device I/O API
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
//
//  License: BSL-1.0
//  Author : EPSON AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This file is distributed under the terms of the Boost Software
//  License, Version 1.0.
//
//  You ought to have received a copy of the Boost Software License
//  along along with this package.
//  If not, see <http://www.boost.org/LICENSE_1_0.txt>.

#ifndef drivers_esci_usb_hpp_
#define drivers_esci_usb_hpp_

#include <string>

class usb_handle
{
public:
  usb_handle (const std::string& udi);
  ~usb_handle ();
};

#endif /* drivers_esci_usb_hpp_ */
