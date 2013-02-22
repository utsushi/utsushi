//  connexion.hpp -- transport messages between software and device
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//  Copyright (C) 2008  Olaf Meeuwissen
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

#ifndef utsushi_connexion_hpp_
#define utsushi_connexion_hpp_

#include "memory.hpp"
#include "octet.hpp"
#include "option.hpp"

#include "pattern/decorator.hpp"

namespace utsushi {

class connexion
  : public configurable
{
public:
  typedef shared_ptr< connexion > ptr;

  virtual ~connexion () {}

  virtual void send (const octet *message, streamsize size) = 0;
  virtual void recv (      octet *message, streamsize size) = 0;

  static connexion::ptr create (const std::string& type,
                                const std::string& path);
};

template<>
class decorator< connexion >
  : public connexion
{
public:
  typedef shared_ptr< connexion > ptr;

  decorator (ptr instance);

  virtual void send (const octet *message, streamsize size);
  virtual void recv (      octet *message, streamsize size);

protected:
  typedef decorator base_;

  ptr instance_;
};

}       // namespace utsushi

#endif  /* utsushi_connexion_hpp_ */
