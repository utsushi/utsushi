//  descriptor.cpp -- objects for options and option groups
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

#include "utsushi/descriptor.hpp"
#include "utsushi/i18n.hpp"

namespace utsushi {

level::symbol::symbol (const key& key,
                       const string& name, const string& text)
  : tag::symbol (key, name, text)
{}

const level::symbol level::standard (
  "01_standard",
  N_("Standard"),
  N_("If there is any user interface at all, options at the standard "
     "level are meant to be made available to the user.")
);

const level::symbol level::extended (
  "02_extended",
  N_("Extended"),
  N_("Extended options are for those situations where the user needs "
     "a bit more control over how things will be done.")
);

const level::symbol level::complete (
  "03_complete",
  N_("Complete"),
  N_("This is for options that are mostly just relevant for the most "
     "demanding of image acquisition jobs or those users will not be "
     "satisfied unless they are in complete control.")
);

descriptor::descriptor (const string& name, const string& text)
  : name_(name)
  , text_(text)
  , level_(level::complete)
{}

string
descriptor::name () const
{
  return name_;
}

string
descriptor::text () const
{
  return text_;
}

std::set< key >
descriptor::tags () const
{
  return tags_;
}

bool
descriptor::is_at (const level::symbol& level) const
{
  return (level_ == level);
}

void
descriptor::name (const string& name)
{
  name_ = name;
}

void
descriptor::text (const string& text)
{
  text_ = text;
}

descriptor&
descriptor::operator() (const tag::symbol& tag)
{
  tags_.insert (tag);
  return *this;
}

descriptor&
descriptor::operator() (const level::symbol& level)
{
  level_ = level;
  return *this;
}

aggregator
attributes ()
{
  return descriptor ();
}

aggregator
attributes (const tag::symbol& tag)
{
  descriptor d;
  return d (tag);
}

aggregator
attributes (const level::symbol& level)
{
  descriptor d;
  return d (level);
}

}       // namespace utsushi
