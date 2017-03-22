//  connexion.hpp -- sockets for IPC with the parent process
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

#ifndef drivers_esci_connexion_hpp_
#define drivers_esci_connexion_hpp_

#include <string>

#if __cplusplus >= 201103L
#include <cstdint>
#else
#include <stdint.h>
#endif

#include <unistd.h>

// WARNING: IPC request header structure, supported types and statuses
// are determined by the API that the parent process uses.  We are not
// free to do as we please with these.

// IPC request header
struct header
{
  uint32_t id;
  uint32_t type;
  uint32_t error;
  int32_t  size;
};

// Supported IPC request types
enum {
  TYPE_NATIVE = 0,
  TYPE_OPEN   = 4,
  TYPE_CLOSE  = 5,
};

// IPC request statuses
enum {
  STATUS_OK   = 0,
  STATUS_NG   = ~uint32_t (0),
};

class machine;

//! Socket connexion for IPC with the parent process
class connexion
{
public:
  connexion ();
  ~connexion ();

  void accept ();
  bool eof () const;

  int port () const;
  int error () const;

public:
  ssize_t read (header& hdr, std::string& payload);
private:
  ssize_t read (header& hdr);
  ssize_t read (void *buf, size_t size);
  ssize_t read (int fd, void *buf, size_t size);

public:
  ssize_t write (uint32_t id, uint32_t status,
                 const std::string& buf = std::string ());
private:
  ssize_t write (const header& hdr, const std::string& buf);
  ssize_t write (const void *buf, size_t size);
  ssize_t write (int fd, const void *buf, size_t size);

public:
  void dispatch (const header& hdr, const std::string& buf);
private:
  void handle_open (const header& hdr, const std::string& buf);
  void handle_close (const header& hdr, const std::string& buf);
  void handle_native (const header& hdr, const std::string& buf);

  uint32_t create_id () const;

  uint32_t id_;
  int sock_;
  int port_;
  int error_;

  machine *machine_;
};

#endif /* drivers_esci_connexion_hpp_ */
