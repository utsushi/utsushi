//  command-line.hpp -- execution for use in test implementation code
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_test_command_line_hpp_
#define utsushi_test_command_line_hpp_

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <iosfwd>
#include <stdexcept>
#include <string>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lambda/lambda.hpp>

#ifndef SHELL
#define SHELL "/bin/sh"
#endif

namespace utsushi {
namespace test {

//! Execute a command-line from within a test case
/*! Sometimes existing utilities already do the job that one wants to
 *  do in the scope of a test case.  At other times, one may need to
 *  write test cases to cover an application's command-line handling.
 *  In these situations, the command_line class comes in handy.  Just
 *  put a (shell) command-line together and execute() it.  The regular
 *  out()put and err()or messages can be read back in by the caller
 *  using standard \c istream API.
 *
 *  \note Users of this class need to link with Boost.Iostreams.
 */
class command_line
{
public:
  //! Create a command-line without any arguments
  command_line (const std::string& executable)
    : command_(executable)
    , err_code_(0)
  {}

  //! Create a command-line with a single \a argument
  command_line (const std::string& executable,
                const std::string& argument)
    : command_(executable + " " + argument)
    , err_code_(0)
  {}

  //! Create a command-line with an arbitrary number of arguments
  /*! Arguments can be passed through any \c std::string container
   *  that supports forward iteration.  Spaces are inserted between
   *  all arguments.
   */
  template< typename forward_iterator >
  command_line (const std::string& executable,
                forward_iterator first, forward_iterator last)
    : command_(executable)
    , err_code_(0)
  {
    using boost::lambda::constant;
    using boost::lambda::_1;

    std::for_each (first, last, command_ += constant (" ") + _1);
  }

  //! Append a (space separated) \a argument to the command-line
  command_line&
  operator+= (const std::string& argument)
  {
    command_ += " " + argument;
    return *this;
  }

  //! Invoke the current command-line
  /*! The current command-line is invoked in a separate process and
   *  its outputs (both regular and error messages) are captured for
   *  later analysis by the caller.  Normally, the command-line's exit
   *  status is returned but in case of internal error conditions a
   *  negative value is returned and \c errno is set.
   *
   *  This function does not modify the value of \c errno unless an
   *  error condition was detected.  However, functions called in the
   *  implementation may change \c errno.
   *
   *  \note The current implementation is synchronous.  The parent
   *        process waits until command-line execution has completed.
   *
   *  \todo Add support for timeout specification
   *  \todo Add multi-thread support so the invoking thread can supply
   *        piece-meal input
   */
  int
  execute ()
  {
    int status = -1;
    int out[2];
    int err[2];

    out[0] = out[1] = -1;
    err[0] = err[1] = -1;

    try
      {
        if (-1 == pipe (err)) throw_(errno);
        if (-1 == pipe (out)) throw_(errno);

        const pid_t pid (fork ());

        if (0 > pid)            // fork() failed
          {
            throw_(errno);
          }
        else if (0 == pid)      // child process
          {
            close (out[0]);     // unused read ends
            close (err[0]);

            if (0 <= dup2 (err[1], STDERR_FILENO))
              {
                if (0 <= dup2 (out[1], STDOUT_FILENO))
                  {
                    execl (SHELL, SHELL, "-c", command_.c_str (),
                           NULL);
                  }
              }

            // notify the parent process of failure, clean up and bomb

            int ec = errno;
            int m = 0;
            int n = strlen (strerror (ec));
            while (0 < n)
              {
                ssize_t rv = write (err[1], strerror (ec)  + m,
                                    strlen (strerror (ec)) - m);
                if (0 >= rv) break;
                m += rv;
                n -= rv;
              }
            fsync (err[1]);

            close (err[1]);
            close (out[1]);
            _exit (EXIT_FAILURE);
          }
        else                    // parent process
          {
            if (pid != waitpid (pid, &status, 0))
              {
                status = -2;
                throw_(errno);
              }
          }
      }
    catch (const std::runtime_error& e)
      {
        //! \todo produce an error message
      }

    close (out[1]);            // unused write ends
    close (err[1]);

    out_stream_.open (boost::iostreams::file_descriptor_source
                      (out[0], boost::iostreams::close_handle));
    err_stream_.open (boost::iostreams::file_descriptor_source
                      (err[0], boost::iostreams::close_handle));

    if (0 != err_code_) errno = err_code_;
    err_code_ = 0;

    return (WIFEXITED (status)
            ? WEXITSTATUS (status)
            : status);
  }

  //! Access the command-line's regular output
  std::istream&
  out ()
  {
    return out_stream_;
  }

  //! Access the command-line's error messages
  std::istream&
  err ()
  {
    return err_stream_;
  }

protected:
  //! Store an error code and throw an appropriate exception
  /*! This helper function simplifies the execute() implementation.
   *  That implementation relies solely on C API so the only exception
   *  that can be encountered is the one throw here.  The error code
   *  is stored so that we can set \c errno to a suitable value when
   *  necessary.
   */
  void
  throw_(int ec)
  {
    err_code_ = ec;
    throw std::runtime_error (strerror (ec));
  }

  std::string command_;

  int err_code_;

  typedef boost::iostreams::stream
  < boost::iostreams::file_descriptor_source > file_descriptor_stream;

  file_descriptor_stream err_stream_;
  file_descriptor_stream out_stream_;
};

} // namespace test
} // namespace utsushi

#endif /* utsushi_test_command_line_hpp_ */
