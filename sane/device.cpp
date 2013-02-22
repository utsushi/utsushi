//  device.cpp -- OO wrapper for SANE_Device instances
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

#include "device.hpp"

namespace sane {

using namespace utsushi;

const SANE_Device **device::list = NULL;

std::vector< device > *device::pool;

void
device::init ()
{
  name   = name_.c_str ();
  vendor = vendor_.c_str ();
  model  = model_.c_str ();
  type   = type_.c_str ();
}

device::device (const scanner::id& id)
  : name_(id.udi ())
  , vendor_(id.vendor ())
  , model_(id.model ())
  , type_(id.type ())
{
  init ();
}

device::device (const device& dev)
  : name_(dev.name_)
  , vendor_(dev.vendor_)
  , model_(dev.model_)
  , type_(dev.type_)
{
  init ();
}

device&
device::operator= (const device& dev)
{
  if (this == &dev) return *this;

  name_ = dev.name_;
  vendor_ = dev.vendor_;
  model_ = dev.model_;
  type_ = dev.type_;

  init ();

  return *this;
}

void
device::release ()
{
  delete[] sane::device::list;
  sane::device::list = NULL;
  sane::device::pool->clear ();
}

}       // namespace sane
