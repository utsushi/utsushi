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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "connexion.hpp"
#include "machine.hpp"

#include <boost/static_assert.hpp>

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define require assert
#define promise assert

#if __cplusplus < 201103L
#define nullptr 0
#endif

BOOST_STATIC_ASSERT (16 == sizeof (header));

// TODO implement proper cancellation support
extern sig_atomic_t cancel_requested;

connexion::connexion ()
  : id_(0)
  , sock_(-1)
  , port_(-1)
  , error_(0)
  , machine_(nullptr)
{
  sock_ = socket (AF_INET, SOCK_STREAM, 0);
  if (0 > sock_)
    throw std::runtime_error
      (std::string ("socket: ") + strerror (errno));

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  addr.sin_port        = htons (0);

  if (0 > bind (sock_, reinterpret_cast< sockaddr * > (&addr), sizeof (addr)))
    throw std::runtime_error
      (std::string ("bind: ") + strerror (errno));

  socklen_t len = sizeof (addr);
  if (0 > getsockname (sock_, reinterpret_cast< sockaddr * > (&addr), &len))
    throw std::runtime_error
      (std::string ("getsockname: ") + strerror (errno));
  port_ = ntohs (addr.sin_port);

  if (0 > listen (sock_, 0))
    throw std::runtime_error
      (std::string ("listen: ") + strerror (errno));
}

connexion::~connexion ()
{
  delete machine_;
  if (0 > close (sock_))
    std::clog << "close: " << strerror (errno) << std::endl;
}

void
connexion::accept ()
{
  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  addr.sin_port        = htons (port_);

  socklen_t len = sizeof (addr);
  int fd = ::accept (sock_, reinterpret_cast< sockaddr * > (&addr), &len);
  if (0 > fd)
    throw std::runtime_error
      (std::string ("accept: ") + strerror (errno));

  if (0 > close (sock_))
    std::clog << "close: " << strerror (errno) << std::endl;
  sock_ = fd;
}

bool
connexion::eof () const
{
  return (0 > port_ || cancel_requested);
}

int
connexion::port () const
{
  return port_;
}

int
connexion::error () const
{
  return error_;
}

ssize_t
connexion::read (header& hdr, std::string& payload)
{
  ssize_t rv = read (hdr);
  if (0 > rv) return rv;

  assert (rv == sizeof (hdr));

  payload.resize (hdr.size);
  if (0 == hdr.size) return rv;

  return read (const_cast< char * > (payload.data ()), hdr.size);
}

ssize_t
connexion::read (header& hdr)
{
  header h;
  ssize_t rv = read (&h, sizeof (h));
  if (0 > rv) return rv;

  assert (rv == sizeof (hdr));

  hdr.id    = ntohl (h.id);
  hdr.type  = ntohl (h.type);
  hdr.error = ntohl (h.error);
  hdr.size  = ntohl (h.size);

  return rv;
}

ssize_t
connexion::read (void *buf, size_t size)
{
  require (buf && (0 < size));

  size_t n = size;
  char  *p = reinterpret_cast< char * > (buf);

  while (0 < n)
    {
      ssize_t t = read (sock_, p, n);

      if (0 >  t) return t;
      if (0 == t) return EOF;

      n -= t;
      p += t;
    }

  promise (0 == n);
  return size - n;
}

ssize_t
connexion::read (int fd, void *buf, size_t size)
{
  assert (fd == sock_);

#ifdef HAVE_SIGPROCMASK
  sigset_t current, blocked;
  sigemptyset (&blocked);
  sigaddset (&blocked, SIGTERM);
  sigaddset (&blocked, SIGINT );
  sigprocmask (SIG_BLOCK, &blocked, &current);
#endif

  error_ = 0;
  ssize_t rv = ::read (fd, buf, size);
  if (0 > rv) error_ = errno;

#ifdef HAVE_SIGPROCMASK
  sigprocmask (SIG_SETMASK, &current, NULL);
#endif

  return rv;
}

ssize_t
connexion::write (uint32_t id, uint32_t status,
                  const std::string& payload)
{
  header hdr;

  hdr.id    = htonl (id);
  hdr.type  = htonl (0);
  hdr.error = htonl (status);
  hdr.size  = htonl (payload.size ());

  return write (hdr, payload);
}

ssize_t
connexion::write (const header& hdr, const std::string& payload)
{
  ssize_t rv = write (&hdr, sizeof (hdr));

  if (0 >  rv) return rv;
  if (0 == rv) return EOF;

  if (0 == hdr.size) return 0;

  return write (payload.data (), payload.size ());
}

ssize_t
connexion::write (const void *buf, size_t size)
{
  require (buf && (0 < size));

  size_t      n = size;
  const char *p = reinterpret_cast< const char * > (buf);

  while (0 < n)
   {
     ssize_t t = write (sock_, p, n);

     if (0 > t) return t;

     n -= t;
     p += t;
   }

  promise (0 == n);
  return size - n;

}

ssize_t
connexion::write (int fd, const void *buf, size_t size)
{
  assert (fd == sock_);

#ifdef HAVE_SIGPROCMASK
  sigset_t current, blocked;
  sigemptyset (&blocked);
  sigaddset (&blocked, SIGTERM);
  sigaddset (&blocked, SIGINT );
  sigprocmask (SIG_BLOCK, &blocked, &current);
#endif

  error_ = 0;
  ssize_t rv = ::write (fd, buf, size);
  if (0 > rv) error_ = errno;

#ifdef HAVE_SIGPROCMASK
  sigprocmask (SIG_SETMASK, &current, NULL);
#endif

  return rv;
}

void
connexion::dispatch (const header& hdr, const std::string& buf)
{
  if (hdr.id != id_ && TYPE_OPEN != hdr.type)
    {
      std::clog << "dispatch: ignoring request with unknown ID\n";
      return;
    }

  assert (std::string::size_type (hdr.size) == buf.size ());

  switch (hdr.type)
    {
    case TYPE_OPEN  : { handle_open   (hdr, buf); break; }
    case TYPE_CLOSE : { handle_close  (hdr, buf); break; }
    case TYPE_NATIVE: { handle_native (hdr, buf); break; }
    default:
      {
        std::clog << "dispatch: ignoring unknown request type: "
                  << hdr.type << "\n";
        write (hdr.id, STATUS_NG);
      }
    }
}

void
connexion::handle_open (const header& hdr, const std::string& udi)
{
  require (TYPE_OPEN == hdr.type);

  assert (!id_ || id_ == hdr.id);

  if (!id_)
    {
      try
        {
          machine_ = new machine (udi);
          id_ = create_id ();
          write (id_, STATUS_OK);

          promise (id_ && machine_);
        }
      catch (...)
        {
          write (hdr.id, STATUS_NG);
          promise (!id_ && !machine_);
        }
    }
  else
    {
      std::clog << "spurious open request\n";
      write (id_, STATUS_NG);
    }
}

void
connexion::handle_close (const header& hdr, const std::string& buf)
{
  require (TYPE_CLOSE == hdr.type);

  assert (id_ && id_ == hdr.id);

  if (id_)
    {
      delete machine_;
      machine_ = nullptr;
      write (id_, STATUS_OK);
      id_ = 0;
      port_ = -1;

      promise (!id_ && !machine_);
    }
  else
    {
      std::clog << "spurious close request\n";
      write (hdr.id, STATUS_NG);
    }
}

void
connexion::handle_native (const header& hdr, const std::string& buf)
{
  require (TYPE_NATIVE == hdr.type);
  require (machine_);

  assert (id_ && id_ == hdr.id);

  if (id_)
    {
      machine_->process (buf);
      while (!machine_->eof ())
        write (id_, STATUS_OK, machine_->respond ());
    }
  else
    {
      std::clog << "martian native request\n";
      write (hdr.id, STATUS_NG);
    }
}

uint32_t
connexion::create_id () const
{
  require (0 < port_);
  return ((uint32_t) getpid () << 16) | (0x0000FFFF & port_);
}
