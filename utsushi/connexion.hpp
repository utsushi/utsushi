//  connexion.hpp -- transport messages between software and device
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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

#include <sys/types.h>

#include "cstdint.hpp"
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
  virtual void send (const octet *message, streamsize size,
                     double timeout) = 0;
  virtual void recv (octet *message, streamsize size) = 0;
  virtual void recv (octet *message, streamsize size,
                     double timeout) = 0;

  static connexion::ptr create (const std::string& type,
                                const std::string& path,
                                const bool debug = false);
};

namespace ipc {

class header
{
public:
  header ();

  uint32_t token () const;
  uint32_t type () const;
  uint32_t error () const;
  int32_t size () const;

  void token (uint32_t token);
  void type (uint32_t type);
  void error (uint32_t error);
  void size (int32_t size);

  enum {                        // \todo get rid of "legacy" types
    OPEN = 4,
    CLOSE,
  };

private:
  uint32_t token_id_;
  uint32_t type_;
  uint32_t error_;
  int32_t size_;
};

class connexion
  : public utsushi::connexion
{
public:
  connexion (const std::string& type, const std::string& path);
  virtual ~connexion ();

  virtual void send (const octet *message, streamsize size);
  virtual void send (const octet *message, streamsize size, double timeout);
  virtual void recv (octet *message, streamsize size);
  virtual void recv (octet *message, streamsize size, double timeout);

protected:
  bool connect_();
  bool fork_();

  streamsize send_message_(const header& hdr, const octet *  payload);
  streamsize recv_message_(      header& hdr,       octet *& payload);
  streamsize send_message_(const void *data, streamsize size);
  streamsize recv_message_(      void *data, streamsize size);

  pid_t pid_;
  int   port_;
  int   socket_;

  std::string name_;

  uint32_t id_;

  static int default_timeout_;
};

}       // namespace ipc

template<>
class decorator< connexion >
  : public connexion
{
public:
  typedef shared_ptr< connexion > ptr;

  decorator (ptr instance);

  virtual void send (const octet *message, streamsize size);
  virtual void send (const octet *message, streamsize size, double timeout);
  virtual void recv (octet *message, streamsize size);
  virtual void recv (octet *message, streamsize size, double timeout);

  virtual option::map::ptr options ();

protected:
  typedef decorator base_;

  ptr instance_;
};

}       // namespace utsushi

#endif  /* utsushi_connexion_hpp_ */
