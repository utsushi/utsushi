//  ipc-cnx.cpp -- mock program for use by ipc::connexion unit tests
//  Copyright (C) 2013, 2015  SEIKO EPSON CORPORATION
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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utsushi/connexion.hpp"
#include "utsushi/log.hpp"
#include "utsushi/octet.hpp"

using namespace utsushi;

namespace {

struct packet
  : public ipc::header
{
  packet ()
    : payload (0)
  {}

  ~packet ()
  {
    delete [] payload;
  }

  void recv (int sock);
  void send (int sock);

  octet    *payload;
};

void
packet::recv (int sock)
{
  read (sock, reinterpret_cast< octet * > (this), sizeof (ipc::header));

  if (0 < this->size ())
    {
      delete [] payload;
      payload = new octet[this->size ()];
      read (sock, payload, this->size ());
    }
}

void
packet::send (int sock)
{
  write (sock, reinterpret_cast< octet * > (this), sizeof (ipc::header));

  if (0 < this->size ())
    write (sock, payload, this->size ());
}

void
hangup (int signum)
{
  exit (signum == SIGHUP ? EXIT_SUCCESS : EXIT_FAILURE);
}

}

int
main (int argc, char *argv[])
{
  errno = 0;
  int s = socket (AF_INET, SOCK_STREAM, 0);
  if (0 > s)
    {
      log::fatal ("socket: %1%") % strerror (errno);
      return EXIT_FAILURE;
    }

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons (0);
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  errno = 0;
  int rv = ::bind (s, reinterpret_cast< sockaddr * > (&addr), sizeof (addr));
  if (rv)
    {
      log::fatal ("bind: %1%") % strerror (errno);
      return EXIT_FAILURE;
    }

  socklen_t n = sizeof (addr);
  errno = 0;
  rv = getsockname (s, reinterpret_cast< sockaddr * > (&addr), &n);
  if (rv)
    {
      log::fatal ("getsockname: %1%") % strerror (errno);
      return EXIT_FAILURE;
    }

  std::cout << ntohs (addr.sin_port) << std::endl;

  errno = 0;
  rv = listen (s, 0);
  if (rv)
    {
      log::fatal ("listen: %1%") % strerror (errno);
      return EXIT_FAILURE;
    }

  errno = 0;
  int as = accept (s, reinterpret_cast< sockaddr * > (&addr), &n);
  if (0 > as)
    {
      log::fatal ("accept: %1%") % strerror (errno);
      return EXIT_FAILURE;
    }

  std::signal (SIGHUP, hangup);

  for (;;)
    {
      using utsushi::ipc::header;

      packet p;

      p.recv (as);

      /**/ if (!p.type ())
        {
          p.error (0);
          for (streamsize i = 0; i < p.size (); ++i)
            p.payload[i] = std::toupper (p.payload[i]);
          p.send (as);
        }
      else if (header::OPEN == p.type ())
        {
          p.error (0);
          p.size (0);
          p.send (as);
        }
      else if (header::CLOSE == p.type ())
        {
          p.error (0);
          p.size (0);
          p.send (as);
        }
      else
        {
          log::error ("unhandled packet type: %1%") % p.type ();
          p.error (~0);
          p.size (0);
          p.send (as);
        }
    }

  return EXIT_FAILURE;
}
