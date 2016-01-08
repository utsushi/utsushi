//  file.cpp -- based output devices
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#include "utsushi/file.hpp"

#include "utsushi/format.hpp"
#include "utsushi/log.hpp"
#include "utsushi/regex.hpp"

#include <boost/filesystem.hpp>
#include <boost/scoped_array.hpp>
#include <boost/throw_exception.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ios>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fs = boost::filesystem;

using std::ios_base;

namespace utsushi {

path_generator::path_generator ()
{}

path_generator::path_generator (const std::string& pattern)
  : offset_(0)
{
  fs::path p (pattern);
  parent_ = p.parent_path ().string ();         // don't touch this

  std::string filename = p.filename ().string ();

  regex re ("(([^%]|%%)*)%0*([0-9]*)i(([^%]|%%)*)");
  smatch m;

  if (regex_match (filename, m, re))
    {
      format_ = filename;
      if (m.str (3).length ())  // make sure we use zero padding
        {
          format_ = m.str (1) + "%0" + m.str (3) + "i" + m.str (4);
        }
    }
  else
    {
      *this = path_generator ();
    }
}

path_generator::operator bool () const
{
  return 0 < format_.size ();
}

std::string
path_generator::operator() ()
{
  using boost::scoped_array;

  int sz = snprintf (NULL, 0, format_.c_str (), offset_);

  scoped_array< char > buf (new char[sz + 1]);

  snprintf (buf.get (), sz + 1, format_.c_str (), offset_);
  ++offset_;

  return (fs::path (parent_) / buf.get ()).string ();
}

file_idevice::file_idevice (const std::string& filename)
  : filename_(filename)
  , used_(true)
{}

file_idevice::file_idevice (const path_generator& generator)
  : generator_(generator)
  , used_(true)
{}

file_idevice::~file_idevice ()
{
  file_.close ();
}

bool
file_idevice::is_consecutive () const
{
  return generator_;
}

bool
file_idevice::obtain_media ()
{
  if (is_consecutive () && used_) {
    filename_ = generator_();
  }

  return used_ = fs::exists (filename_);
}

bool
file_idevice::set_up_image ()
{
  return file_.open (filename_.c_str (),
                     std::ios_base::binary | std::ios_base::in);
}

void
file_idevice::finish_image ()
{
  file_.close ();
}

streamsize
file_idevice::sgetn (octet *data, streamsize n)
{
  return file_.sgetn (data, n);
}

file_odevice::file_odevice (const std::string& filename)
  : filename_(filename)
  , fd_(-1)
  , fd_flags_(O_RDWR | O_CREAT | O_CLOEXEC)
{}

file_odevice::file_odevice (const path_generator& generator)
  : generator_(generator)
  , fd_(-1)
  , fd_flags_(O_RDWR | O_CREAT | O_CLOEXEC)
{}

file_odevice::~file_odevice ()
{
  close ();
}

void
file_odevice::open ()
{
  if (-1 != fd_)
    {
      log::trace ("file_odevice: may be leaking a file descriptor");
    }

  // Create non-executable files.  Note that these permission flags
  // are still subject to umask() which is assumed to have been set
  // to a value in line with the process' preferences.

  const int fd_perms = (  S_IRUSR | S_IWUSR
                        | S_IRGRP | S_IWGRP
                        | S_IROTH | S_IWOTH
                        );

  fd_ = ::open (filename_.c_str (), fd_flags_ | O_TRUNC, fd_perms);
  if (-1 == fd_)
    {
      BOOST_THROW_EXCEPTION (ios_base::failure (strerror (errno)));
    }
}

void
file_odevice::close ()
{
  if (-1 == fd_) return;

  if (-1 == ::close (fd_))
    {
      // Error conditions upon closing of a file descriptor are for
      // diagnostic purposes only.  Do NOT throw an exception here.

      log::alert (strerror (errno));
    }
  fd_ = -1;
}

streamsize
file_odevice::write (const octet *data, streamsize n)
{
  if (-1 == fd_)
    {
      log::error ("file_odevice::write(): %1%") % strerror (EBADF);
      return n;
    }

  errno = 0;
  int rv = ::write (fd_, data, n);
  int ec = errno;

  if (0 < rv) return rv;

  if (0 > rv)                   // definitely fatal
    {
      eof (ctx_);
      BOOST_THROW_EXCEPTION (ios_base::failure (strerror (ec)));
    }

  // Nothing was written for some reason.  For regular files the
  // EAGAIN and EINTR conditions should not be considered fatal.
  // Anything else is.

  if (!(EINTR == ec || EAGAIN == ec))
    {
      eof (ctx_);
      BOOST_THROW_EXCEPTION (ios_base::failure (strerror (ec)));
    }

  // Now check if fd_ refers to a regular file.  If stat() fails,
  // assume fd_ does not refer to a regular file.

  struct stat m;
  if (-1 == stat (filename_.c_str (), &m))
    {
      log::alert (strerror (errno));
      m.st_mode &= ~S_IFREG;            // make dead sure
    }
  if (!S_ISREG (m.st_mode))
    {
      eof (ctx_);
      BOOST_THROW_EXCEPTION (ios_base::failure (strerror (ec)));
    }

  return 0;
}

void
file_odevice::bos (const context& ctx)
{
  count_ = 0;
  if (!generator_)
    {
      open ();
    }
}

void
file_odevice::boi (const context& ctx)
{
  if (generator_)
    {
      filename_ = generator_();
      open ();
    }
}

void
file_odevice::eoi (const context& ctx)
{
  if (generator_)
    {
      close ();
    }
  ++count_;
}

void
file_odevice::eos (const context& ctx)
{
  if (!generator_)
    {
      if (0 == count_)
        {
          log::alert
            ("removing %1% because no images were produced")
            % filename_;

          if (-1 == remove (filename_.c_str ()))
            {
              log::alert (strerror (errno));
            }
        }
      close ();
    }
}

void
file_odevice::eof (const context& ctx)
{
  if (-1 == fd_) return;

  if (-1 == remove (filename_.c_str ()))
    {
      log::alert (strerror (errno));
    }
  close ();
}

}       // namespace utsushi
