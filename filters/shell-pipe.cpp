//  shell-pipe.cpp -- outsource filtering to a command-line utility
//  Copyright (C) 2014  SEIKO EPSON CORPORATION
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "shell-pipe.hpp"

#include <utsushi/log.hpp>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>
#include <stdexcept>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHELL "/bin/sh"
#define nullptr NULL

namespace {

using namespace utsushi;

inline
void
close_(int& fd)
{
  if (-1 == fd) return;

  if (0 > close (fd))
    log::error (strerror (errno));

  fd = -1;
}

inline
void
log_io_(const std::string& cmd, pid_t pid, int ec)
{
  if (EINTR == ec || EAGAIN == ec || EWOULDBLOCK == ec)
    log::debug ("%1% (pid: %2%): %3%")
      % cmd % pid % strerror (ec);
  else
    log::error ("%1% (pid: %2%): %3%")
      % cmd % pid % strerror (ec);
}

inline
void
log_process_exit_(const std::string& cmd, const siginfo_t& info)
{
  switch (info.si_code)
    {
    case CLD_EXITED:
      log::trace ("%1% exited (pid: %2%, status: %3%)")
        % cmd % info.si_pid % info.si_status;
      break;
    case CLD_KILLED:
      log::trace ("%1% killed (pid: %2%, signal: %3%)")
        % cmd % info.si_pid % info.si_status;
      break;
    case CLD_DUMPED:
      log::trace ("%1% dumped core (pid: %2%, signal: %3%)")
        % cmd % info.si_pid % info.si_status;
      break;
    default:
      log::error ("%1% exited (pid: %2%, code: %3)")
        % cmd % info.si_pid % info.si_code;
    }
}

// \todo Read any waiting error messages from \a efd and log them
inline
void
reap_(const std::string& cmd, pid_t pid, int efd)
{
  int ec = 0;

  do
    {
      siginfo_t info;

      if (0 == waitid (P_PID, pid, &info, WEXITED))
        {
          log_process_exit_(cmd, info);
        }
      else
        {
          ec = errno;
          if (EINTR != ec && ECHILD != ec)
            log::debug (strerror (ec));
        }
    }
  while (EINTR == ec);

  close_(efd);
}

inline
void
reserve_(octet *&buffer, ssize_t& size, int pipe)
{
  int pipe_size = fcntl (pipe, F_GETPIPE_SZ);

  if (pipe_size > size)
    {
      octet *p = new (std::nothrow) octet[pipe_size];
      if (p)
        {
          delete [] buffer;
          buffer = p;
          size   = pipe_size;
          log::trace ("shell-pipe: buffer size now %1% octets") % size;
        }
    }

  if (0 > pipe_size) log::error (strerror (errno));
}

inline
void
reset_(int& pipe, int fd)
{
  close_(pipe);
  pipe = fd;
  fcntl (pipe, F_SETFL, O_NONBLOCK);
}

}       // namespace

namespace utsushi {
namespace _flt_ {

using std::runtime_error;

shell_pipe::shell_pipe (const std::string& command)
  : command_(command)
  , process_(-1)
  , i_pipe_(-1)
  , o_pipe_(-1)
  , e_pipe_(-1)
  , buffer_(new octet[default_buffer_size])
  , buffer_size_(default_buffer_size)
{}

shell_pipe::~shell_pipe ()
{
  delete [] buffer_;
  close_(i_pipe_);
  close_(o_pipe_);
  close_(e_pipe_);
  if (0 < process_) waitid (P_PID, process_, nullptr, WEXITED);
}

streamsize
shell_pipe::write (const octet *data, streamsize n)
{
  return service_pipes_(data, n);
}

void
shell_pipe::boi (const context& ctx)
{
  ctx_ = estimate (ctx);
  execute_(command_ + ' ' + arguments (ctx));
}

void
shell_pipe::eoi (const context& ctx)
{
  close_(i_pipe_);              // no more input for process_

  while (-1 != o_pipe_)
    service_pipes_(nullptr, 0);

  ctx_ = finalize (ctx);

  reap_(command_, process_, e_pipe_);
  process_ = -1;
  e_pipe_  = -1;
}

void
shell_pipe::eof (const context& ctx)
{
  close_(i_pipe_);
  close_(o_pipe_);              // trigger EPIPE/SIGPIPE in process_

  reap_(command_, process_, e_pipe_);
  process_ = -1;
  e_pipe_  = -1;
}

context
shell_pipe::estimate (const context& ctx) const
{
  return ctx;
}

context
shell_pipe::finalize (const context& ctx) const
{
  return ctx;
}

std::string
shell_pipe::arguments (const context& ctx)
{
  return std::string ();
}

void
shell_pipe::execute_(const std::string& command_line)
{
  BOOST_ASSERT (0 > process_);

  int in [2] = { -1, -1 };
  int out[2] = { -1, -1 };
  int err[2] = { -1, -1 };

  if (   -1 == pipe (err)
      || -1 == pipe (out)
      || -1 == pipe (in)
      ||  0 > (process_ = fork ()))
    {
      log::error (strerror (errno));

      close (in[0]) ; close (in[1]) ;
      close (out[0]); close (out[1]);
      close (err[0]); close (err[1]);

      return;
    }

  BOOST_ASSERT (0 <= process_);

  if (0 == process_)            // child process
    {
      close (in[1]);            // unused pipe ends
      close (out[0]);
      close (err[0]);

      if (   0 <= dup2 (err[1], STDERR_FILENO)
          && 0 <= dup2 (out[1], STDOUT_FILENO)
          && 0 <= dup2 (in[0] , STDIN_FILENO))
        {
          execl (SHELL, SHELL, "-c", command_line.c_str (), NULL);
        }

      int ec = errno;           // notify parent process of failure
      log::fatal ("shell-pipe(%1%): execl: %2%")
        % command_ % strerror (ec);

      close (in[0]);
      close (out[1]);
      close (err[1]);
      _exit (EXIT_FAILURE);
    }
  else                          // parent process
    {
      close (in[0]);            // unused pipe ends
      close (out[1]);
      close (err[1]);

      reset_(e_pipe_, err[0]);
      reset_(o_pipe_, out[0]);
      reset_(i_pipe_, in[1]);

      reserve_(buffer_, buffer_size_, o_pipe_);

      log::trace ("%1% started (pid: %2%)") % command_ % process_;
      log::debug ("invocation: %1%") % command_line;
    }
}

streamsize
shell_pipe::service_pipes_(const octet *data, streamsize n)
{
  BOOST_ASSERT ((data && 0 < n) || 0 == n);

  int fds;
  int fd_max = 0;
  fd_set r_fds;
  fd_set w_fds;

  FD_ZERO (&r_fds);
  FD_ZERO (&w_fds);

  using std::max;

  if (0 < i_pipe_ && 0 < n)
    {
      FD_SET (i_pipe_, &w_fds);
      fd_max = max (i_pipe_, fd_max);
    }
  if (0 < o_pipe_)
    {
      FD_SET (o_pipe_, &r_fds);
      fd_max = max (o_pipe_, fd_max);
    }
  if (0 < e_pipe_)
    {
      FD_SET (e_pipe_, &r_fds);
      fd_max = max (e_pipe_, fd_max);
    }

  struct timespec nonblocking = { 0, 0 };
  fds = pselect (fd_max + 1, &r_fds, &w_fds, nullptr,
                 &nonblocking, nullptr);

  if (-1 == fds && EINTR == errno)
    return 0;
  if (-1 == fds)
    BOOST_THROW_EXCEPTION (runtime_error (strerror (errno)));

  ssize_t rv;

  if (0 < e_pipe_ && FD_ISSET (e_pipe_, &r_fds))
    {
      rv = ::read (e_pipe_, buffer_, buffer_size_);

      /**/ if (0 < rv) { message_.append (buffer_, rv); }
      else if (0 > rv) { log_io_(command_, process_, errno); }
      else if (!message_.empty ())
        {
          log::error ("%1% (pid: %2%): %3%")
            % command_ % process_ % message_;
          message_.clear ();
        }
    }

  if (0 < o_pipe_ && FD_ISSET (o_pipe_, &r_fds))
    {
      rv = ::read (o_pipe_, buffer_, buffer_size_);

      /**/ if (0 < rv) { io_->write (buffer_, rv); }
      else if (0 > rv) { log_io_(command_, process_, errno); }
      else  /* EOF */  { close_(o_pipe_); }
    }

  if (0 < i_pipe_ && FD_ISSET (i_pipe_, &w_fds) && 0 < n)
    {
      rv = ::write (i_pipe_, data, n);

      /**/ if (0 < rv) { return rv; }
      else if (0 > rv) { log_io_(command_, process_, errno); }
      else             { return rv; }
    }

  return 0;                     // make caller hold onto data
}

}       // namespace _flt_
}       // namespace utsushi
