//  connexion.cpp -- transport messages between software and device
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2011  Olaf Meeuwissen
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>

#include <boost/filesystem.hpp>

#include "utsushi/connexion.hpp"
#include "utsushi/format.hpp"
#include "utsushi/log.hpp"
#include "utsushi/run-time.hpp"
#include "utsushi/thread.hpp"
#include "connexions/usb.hpp"
#include "connexions/hexdump.hpp"

namespace fs = boost::filesystem;

using std::runtime_error;
const double seconds = 1.0;

namespace utsushi
{
  connexion::ptr
  connexion::create (const std::string& type, const std::string& path, const bool debug)
  {
    ptr cnx;
   /**/ if ("usb" == type)
      {
        // This will be replaced by code that searches for a USB
        // connexion plugin and makes its factory available for use
        // here.  When done, the connexions/usb.hpp include can be
        // removed as well.  For now, any applications that want to
        // create a USB connexion will need to link with a suitable
        // USB plugin.

        libcnx_usb_LTX_factory (cnx, type, path);
      }
    else if (!type.empty ())
      {
        cnx = make_shared< ipc::connexion > (type, path);
      }

    if (debug)
      {
        // same as usb. see comment above.
        libcnx_hexdump_LTX_factory (cnx);
      }

    if (!cnx)
      {
        log::fatal ("unsupported connexion type: '%1%'") % type;
      }
    return cnx;
}

namespace ipc {

header::header ()
  : token_id_(0)
  , type_(0)
  , error_(0)
  , size_(0)
{}

uint32_t
header::token () const
{
  return ntohl (token_id_);
}

uint32_t
header::type () const
{
  return ntohl (type_);
}

uint32_t
header::error () const
{
  return ntohl (error_);
}

int32_t
header::size () const
{
  return ntohl (size_);
}

void
header::token (uint32_t token)
{
  token_id_ = htonl (token);
}

void
header::type (uint32_t type)
{
  type_ = htonl (type);
}

void
header::error (uint32_t error)
{
  error_ = htonl (error);
}

void
header::size (int32_t size)
{
  size_ = htonl (size);
}

bool delay_elapsed (double t_sec)
{
  struct timespec t;
  t.tv_sec  =  t_sec;
  t.tv_nsec = (t_sec - t.tv_sec) * 1000000000;
  return 0 == nanosleep (&t, 0);
}

//! Conveniently change a socket's timeout settings
void
set_timeout (int socket, double t_sec)
{
  if (0 > socket) return;

  struct timeval t;
  t.tv_sec  =  t_sec;
  t.tv_usec = (t_sec - t.tv_sec) * 1000000;

  errno = 0;
  if (0 > setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof (t)))
    {
      log::alert ("socket option: %1%") % strerror (errno);
    }
  errno = 0;
  if (0 > setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof (t)))
    {
      log::alert ("socket option: %1%") % strerror (errno);
    }
}

void
kill_(pid_t pid_, int port_, int socket_, std::string name_);

//! Signal blocking versions of I/O system calls
/*! Applications may register signal handlers with \c SA_RESTART, so
 *  we have to make sure that our I/O with the child process doesn't
 *  repeat itself when a signal handler returns.  This is achieved by
 *  blocking signals for the duration of the I/O system call.
 */
namespace {

streamsize write (int fd, const void *buf, streamsize count);
streamsize read  (int fd,       void *buf, streamsize count);

}       // namespace

int connexion::default_timeout_ = 30 * seconds;

/*! \todo Make retry_count and delay_time configurable
 *  \todo Allow for searching in multiple locations
 */
connexion::connexion (const std::string& type, const std::string& path)
  : pid_(-1)
  , port_(-1)
  , socket_(-1)
  , id_(0)
{
  run_time rt;

  /*! \todo Work out search path logic.
   *
   *  By default, installed services ought to live in PKGLIBEXECDIR.
   *  During development they can be anywhere in the file system as
   *  most services will probably be developed as out-of-tree, third
   *  party modules.  For unit testing purposes one may also have a
   *  module that is in-tree and lives in the current directory.
   *
   *  For now, we are not interested in supporting services all over
   *  the place yet would like to be able to run in place.
   */
  if (!rt.running_in_place ())  // installed version
    {
      name_ = (fs::path (PKGLIBEXECDIR) / type).string ();
    }
  else                          // development, run-in-place setup
    {
      /*! \todo We may need to cater to several services installed in
       *        different locations during a single run so search path
       *        support is warranted.
       */
      const char *path = getenv (PACKAGE_ENV_VAR_PREFIX "LIBEXECDIR");

      if (!path) path = ".";

      name_ = (fs::path (path) / type).string ();
    }

  if (name_.empty ())
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ((format ("%1%: not found") % type).str ()));
    }

  // Not sure if this is really necessary.  The access(2) manual page
  // does not encourage the use of the function as there is a window
  // between the check of a file and the file's use.

  if (0 != access (name_.c_str (), F_OK | X_OK))
    {
      fs::path path (fs::path (PKGLIBEXECDIR)
                     .remove_filename () // == PACKAGE_TARNAME
                     .remove_filename () // host-system triplet?
                     );
      if (   path.filename () == "lib"
          || path.filename () == "lib64"
          || path.filename () == "libexec")
        {
          path /= PACKAGE_TARNAME;
          name_ = (path / type).string ();
        }

      if (0 != access (name_.c_str (), F_OK | X_OK))
        BOOST_THROW_EXCEPTION
          (runtime_error ((format ("%1%: not executable") % name_).str ()));
    }

  if (!fork_())
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ((format ("%1%: cannot fork") % name_).str ()));
    }

  int tries_left = 5;
  while (!connect_()
         && 0 < --tries_left
         && delay_elapsed (1.0 * seconds));

  format fmt ("%1%: %2%");
  std::string msg ("cannot connect");

  if (tries_left)
    {
      header hdr;
      hdr.type (header::OPEN);
      hdr.size (path.length ());

      streamsize n = send_message_(hdr, path.c_str ());

      if (n == hdr.size ())
        {
          header hdr;
          hdr.token (id_);
          octet *placeholder = nullptr;

          n = recv_message_(hdr, placeholder);
          if (!hdr.error () && 0 <= n)
            {
              id_ = hdr.token ();
              log::brief ("opened ipc::connexion to: %1%") % path;
              set_timeout (socket_, default_timeout_);
              return;
            }
          msg = "error receiving";
        }
      else
        {
          msg = "error sending";
        }
    }

  thread (kill_, pid_, port_, socket_, name_).detach ();

  BOOST_THROW_EXCEPTION
    (runtime_error ((fmt % path % msg).str ()));
}

connexion::~connexion ()
{
  header hdr;
  hdr.token (id_);
  hdr.type (header::CLOSE);

  streamsize n = send_message_(hdr, nullptr);
  if (0 > n)
    {
      log::brief ("%1%: failure closing connexion") % name_;
    }
  thread (kill_, pid_, port_, socket_, name_).detach ();
}

void
connexion::send (const octet *message, streamsize size)
{
  return send (message, size, default_timeout_);
}

void
connexion::send (const octet *message, streamsize size, double timeout)
{
  header hdr;
  hdr.token (id_);
  hdr.size (size);
  set_timeout (socket_, timeout);
  send_message_(hdr, message);
}

void
connexion::recv (octet *message, streamsize size)
{
  return recv (message, size, default_timeout_);
}

void
connexion::recv (octet *message, streamsize size, double timeout)
{
  header hdr;
  hdr.token (id_);
  octet *reply = nullptr;

  set_timeout (socket_, timeout);
  recv_message_(hdr, reply);

  if (!hdr.error () && size == hdr.size ())
    {
      if (0 < hdr.size () && reply)
        traits::copy (message, reply, hdr.size ());
    }
  delete [] reply;
}

/*! \todo Make send and receive timeouts configurable
 *  \todo Make usable with IPv6?  IPv6 can handle IPv4 connections
 *        transparently according to ipv6(7).
 */
bool
connexion::connect_()
{
  errno = 0;
  socket_ = socket (AF_INET, SOCK_STREAM, 0);
  if (0 > socket_)
    {
      log::error ("socket: %1%") % strerror (errno);
      return false;
    }

  set_timeout (socket_, 3 * seconds);

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons (port_);
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  if (0 != connect (socket_, (struct sockaddr *) &addr, sizeof (addr)))
    {
      log::error ("connect: %1%") % strerror (errno);
      return false;
    }

  return true;
}

bool
connexion::fork_()
{
  int pipe_fd[2];

  if (-1 == pipe (pipe_fd))
    {
      log::fatal ("pipe: %1%") % strerror (errno);
      return false;
    }

  pid_ = fork ();
  if (0 == pid_)
    {
      // replace child process with a plugin program

      signal (SIGTERM, SIG_IGN);         // dealt with by parent
      signal (SIGINT , SIG_IGN);

      close (pipe_fd[0]);               // unused read end
      if (0 <= dup2 (pipe_fd[1], STDOUT_FILENO))
        {
          log::brief ("%1%[%2%]: starting") % name_ % getpid ();
          if (-1 == execl (name_.c_str (), name_.c_str (), NULL))
            {
              log::fatal ("%1%[%2%]: %3%") % name_ % getpid () % strerror (errno);
            }
        }
      else
        {
          log::error ("%1%[%2%]: %3%") % name_ % getpid () %  strerror (errno);
        }

      // notify the parent process that we're done here

      write (pipe_fd[1], "-1\n", strlen ("-1\n"));
      fsync (pipe_fd[1]);

      close (pipe_fd[1]);
      exit (EXIT_FAILURE);
    }

  bool s = true;
  if (0 > pid_)
    {
      log::fatal ("fork: %1%") % strerror (errno);
      s = false;
    }
  else
    {
      // Check whether child process has (unexpectedly) exited.  We
      // don't want to have zombies in our closet ;-)

      pid_t w = waitpid (pid_, NULL, WNOHANG);
      if (-1 == w)
        {
          log::alert ("waitpid: %1%") % strerror (errno);
        }
      if (0 != w)
        {
          log::brief ("%1%[%2%]: exited prematurely") % name_ % pid_;
          s = false;
        }
      else
        {
          FILE *fp = fdopen (pipe_fd[0], "rb");
          if (fp)
            {
              if (1 != fscanf (fp, "%d", &(port_)))
                {
                  log::error ("fscanf: %1%") % strerror (errno);
                }
              fclose (fp);
            }
          else
            {
              log::fatal ("fdopen: %1%") % strerror (errno);
            }
        }
    }
  close (pipe_fd[0]);
  close (pipe_fd[1]);

  if (0 > port_)
    s = false;

  return s;
}

void
kill_(pid_t pid_, int port_, int socket_, std::string name_)
{
  log::brief ("terminating %1% (port %2%)") % name_ % port_;

  if (0 <= socket_)
    {
      if (0 != close (socket_))
        {
          log::alert ("close: %1%") % strerror (errno);
        }
    }

  if (1 >= pid_)                // nothing left to do
    return;

  if (0 != kill (pid_, SIGHUP))
    {
      log::alert ("kill: %1%") % strerror (errno);
    }

  int status = 0;

  if (pid_ == waitpid (pid_, &status, 0))
    {
      format fmt ("%1%[%2%]: %3% %4%");

      /**/ if (WIFEXITED (status))
        {
          log::trace (fmt)
            % name_ % pid_ % "exited with status"
            % WEXITSTATUS (status)
            ;
        }
      else if (WIFSIGNALED (status))
        {
          log::trace (fmt)
            % name_ % pid_ % "killed by"
            % strsignal (WTERMSIG (status))
            ;
        }
      else if (WIFSTOPPED (status))
        {
          log::brief (fmt)
            % name_ % pid_ % "stopped by"
            % strsignal (WSTOPSIG (status))
            ;
        }
      else if (WIFCONTINUED (status))
        {
          log::brief (fmt)
            % name_ % pid_ % "continued by"
            % strsignal (SIGCONT)
            ;
        }
      else
        {
          log::alert (fmt)
            % name_ % pid_ % "terminated with status"
            % status
            ;
        }
    }
  else
    {
      log::error ("waitpid: %1%") % strerror (errno);
    }
}

streamsize
connexion::send_message_(const header& hdr, const octet *payload)
{
  streamsize n = 0;

  n = send_message_(&hdr, sizeof (hdr));
  if (0 >= n) return -1;

  if (!hdr.size ()) return 0;
  if (!payload) return -1;

  n = send_message_(payload, hdr.size ());

  return n;
}

streamsize
connexion::recv_message_(header& hdr, octet *& payload)
{
  streamsize n = recv_message_(&hdr, sizeof (hdr));

  if (0 > n) return n;

  if (!hdr.size ()) return 0;

  octet *buf = new octet[hdr.size ()];
  n = recv_message_(buf, hdr.size ());
  payload = buf;

  return n;
}

streamsize
connexion::send_message_(const void *data, streamsize size)
{
  if (!size) return -1;

  streamsize n = 0;
  streamsize t = 1;

  const octet *p = reinterpret_cast< const octet * > (data);

  while (n < size && t > 0)
    {
      t = write (socket_, p + n, size - n);
      if (0 > t)
        {
          return -1;
        }
      else
        {
          n += t;
        }
    }

  return n;
}

streamsize
connexion::recv_message_(void *data, streamsize size)
{
  if (!size) return -1;

  streamsize n = 0;
  streamsize t = 1;

  octet *p = reinterpret_cast< octet * > (data);

  while (n < size && t > 0)
    {
      t = read (socket_, p + n, size - n);
      if (0 > t)
        {
          return -1;
        }
      else
        {
          n += t;
        }
    }

  return n;
}

namespace {

streamsize
write (int fd, const void *buf, streamsize count)
{
  sigset_t current, blocked;
  sigemptyset (&blocked);
  sigaddset (&blocked, SIGTERM);
  sigaddset (&blocked, SIGINT );
  sigprocmask (SIG_BLOCK, &blocked, &current);

  errno = 0;
  streamsize rv = ::write (fd, buf, count);
  if (0 > rv)
    log::error ("write failed: %1%") % strerror (errno);

  sigprocmask (SIG_SETMASK, &current, NULL);

  return rv;
}

streamsize
read (int fd, void *buf, streamsize count)
{
  sigset_t current, blocked;
  sigemptyset (&blocked);
  sigaddset (&blocked, SIGTERM);
  sigaddset (&blocked, SIGINT );
  sigprocmask (SIG_BLOCK, &blocked, &current);

  errno = 0;
  streamsize rv = ::read (fd, buf, count);
  if (0 > rv)
    log::error ("read failed: %1%") % strerror (errno);

  sigprocmask (SIG_SETMASK, &current, NULL);

  return rv;
}

}       // namespace
}       // namespace ipc

decorator<connexion>::decorator (ptr instance)
  : instance_(instance)
{}

void
decorator<connexion>::send (const octet *message, streamsize size)
{
  instance_->send (message, size);
}

void
decorator<connexion>::send (const octet *message, streamsize size,
                            double timeout)
{
  instance_->send (message, size, timeout);
}

void
decorator<connexion>::recv (octet *message, streamsize size)
{
  instance_->recv (message, size);
}

void
decorator<connexion>::recv (octet *message, streamsize size,
                            double timeout)
{
  instance_->recv (message, size, timeout);
}

option::map::ptr
decorator<connexion>::options ()
{
  return instance_->options ();
}

} // namespace utsushi
