//  file.cpp -- based output devices
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
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

#include "utsushi/file.hpp"

namespace utsushi {

path_generator::path_generator ()
{}

path_generator::path_generator (const fs::path& prefix,
                                const fs::path& extension,
                                unsigned digits, unsigned offset)
  : offset_(offset)
{
  std::ostringstream fmt_spec;
  std::string ext (extension.string ());

  fmt_spec << prefix.string () << "%|0" << digits << "|";
  if (ext.empty () || '.' == ext[0])
    fmt_spec << ext;
  else
    fmt_spec << "." << ext;

  format_ = format (fmt_spec.str ());
}

path_generator::operator bool () const
{
  return 0 < format_.size ();
}

fs::path
path_generator::operator() ()
{
  return (format_ % offset_++).str ();
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
    name_ = generator_();
  }

  return used_ = fs::exists (name_);
}

bool
file_idevice::set_up_image ()
{
  return file_.open (name_, std::ios_base::binary | std::ios_base::in);
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

file_idevice::file_idevice (const fs::path& name)
  : name_(name)
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

file_odevice::file_odevice (const fs::path& name)
  : name_(name)
{}

file_odevice::file_odevice (const path_generator& generator)
  : generator_(generator)
{}

streamsize
file_odevice::write (const octet *data, streamsize n)
{
  return file_.sputn (data, n);
}

void
file_odevice::bos (const context& ctx)
{
  if (!generator_) {
    file_.open (name_, std::ios_base::binary | std::ios_base::out);
  }
}

void
file_odevice::boi (const context& ctx)
{
  if (generator_) {
    name_ = generator_();
    file_.open (name_, std::ios_base::binary | std::ios_base::out);
  }
}

void
file_odevice::eoi (const context& ctx)
{
  if (generator_) {
    file_.close ();
  }
}

void
file_odevice::eos (const context& ctx)
{
  if (!generator_) {
    file_.close ();
  }
}

}       // namespace utsushi
