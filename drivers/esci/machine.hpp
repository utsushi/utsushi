//  machine.hpp -- ESC/I handshake aware state machine
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

#ifndef drivers_esci_machine_hpp_
#define drivers_esci_machine_hpp_

#include <string>

//! An ESC/I handshake aware state machine
class machine
{
public:
  machine (const std::string& udi);
  ~machine ();

  bool eof () const;

  void        process (const std::string& data);
  std::string respond ();

  class implementation;

private:
  implementation *pimpl;
};

#endif /* drivers_esci_machine_hpp_ */
