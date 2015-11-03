//  scanner.cpp -- interface and support classes
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#include <cstring>
#include <ltdl.h>

#include <list>
#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>

#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"
#include "utsushi/regex.hpp"
#include "utsushi/run-time.hpp"
#include "utsushi/scanner.hpp"

namespace utsushi {

using std::invalid_argument;
using std::runtime_error;
using boost::filesystem::path;

typedef void (*scanner_factory) (scanner::ptr&, connexion::ptr);

scanner_factory
get_scanner_factory (const lt_dlhandle& handle)
{
  return reinterpret_cast< scanner_factory > (lt_dlsym
                                              (handle, "scanner_factory"));
}

scanner::ptr
scanner::create (connexion::ptr cnx, const scanner::info& info)
{
  if (!info.is_driver_set ())
    {
      log::error ("driver not known for %1% (%2%)")
        % info.name ()
        % info.udi ()
        ;
      return scanner::ptr ();
    }

  std::string plugin = "libdrv-" + info.driver ();

  lt_dlhandle handle = NULL;
  scanner_factory factory = 0;
  std::string error (_("driver not found"));

  log::brief ("looking for preloaded '%1%' driver")
    % info.driver ();

  lt_dladvise advice;
  lt_dladvise_init (&advice);
  lt_dladvise_preload (&advice);
  lt_dladvise_ext (&advice);

  handle = lt_dlopenadvise (plugin.c_str (), advice);

  if (handle)
    {
      factory = get_scanner_factory (handle);
      if (factory)
        {
          log::brief ("using preloaded '%1%' driver")
            % info.driver ();
        }
      else
        {
          lt_dlclose (handle);
        }
    }
  lt_dladvise_destroy (&advice);

  if (!factory) {               // trawl the file system
    run_time rt;
    run_time::sequence_type search (rt.load_dirs (run_time::pkg, "driver"));

  run_time::sequence_type::const_iterator it;
  for (it = search.begin (); !handle && it != search.end (); ++it)
    {
      path p (*it);

      log::brief ("looking for '%1%' driver in '%2%'")
        % info.driver ()
        % p.string ()
        ;

      p /= plugin;

      handle = lt_dlopenext (p.string ().c_str ());

      if (handle) {
        factory = get_scanner_factory (handle);

        if (factory) {
          log::brief ("using '%1%'") % p.string ();
        } else {
          error = lt_dlerror ();
          lt_dlclose (handle);
          handle = NULL;
        }
      } else {
        error = lt_dlerror ();
      }
    }
  }

  if (!factory) BOOST_THROW_EXCEPTION (runtime_error (error));

  scanner::ptr rv;
  factory (rv, cnx);
  return rv;
}

scanner::scanner (connexion::ptr cnx)
  : cnx_(cnx)
{
  option_.reset (static_cast< option::map * > (this),
                 null_deleter ());
}

scanner::info::info (const std::string& udi)
  : udi_(udi)
{
  if (!is_valid (udi_))
    {
      BOOST_THROW_EXCEPTION
        (invalid_argument
         ((format (_("syntax error: invalid UDI '%1%'")) % udi_).str ()));
    }
}

std::string
scanner::info::name () const
{
  if (!name_.empty ())
    {
      return name_;
    }

  if (!model_.empty ())
    {
      if (!vendor_.empty ()
          && 0 != model_.find (vendor_))
        {
          return vendor_ + " " + model_;
        }
      return model_;
    }

  if (!vendor_.empty ())
    {
      return vendor_;
    }

  return udi_;
}

std::string
scanner::info::text () const
{
  return text_;
}

void
scanner::info::name (const std::string& name)
{
  name_ = name;
}

void
scanner::info::text (const std::string& text)
{
  text_ = text;
}

std::string
scanner::info::type () const
{
  return type_;
}

std::string
scanner::info::model () const
{
  return model_;
}

std::string
scanner::info::vendor () const
{
  return vendor_;
}

void
scanner::info::type (const std::string& type)
{
  type_ = type;
}

void
scanner::info::model (const std::string& model)
{
  model_ = model;
}

void
scanner::info::vendor (const std::string& vendor)
{
  vendor_ = vendor;
}

std::string
scanner::info::connexion () const
{
  return udi_.substr (0, udi_.find (separator));
}

std::string
scanner::info::driver () const
{
  std::string::size_type pos1 = udi_.find (separator) + 1;
  std::string::size_type pos2 = udi_.find (separator, pos1);

  return udi_.substr (pos1, pos2 - pos1);
}

void
scanner::info::driver (const std::string& driver)
{
  std::string::size_type pos1 = udi_.find (separator) + 1;

  if (is_driver_set ())
    {
      std::string::size_type pos2 = udi_.find (separator, pos1);
      udi_.replace (pos1, pos2 - pos1, driver);
    }
  else
    {
      udi_.insert (pos1, driver);
    }
}

std::string
scanner::info::host () const
{
  return std::string ();
}

std::string
scanner::info::port () const
{
  return std::string ();
}

std::string
scanner::info::path () const
{
  std::string::size_type pos1 = udi_.find (separator) + 1;
  std::string::size_type pos2 = udi_.find (separator, pos1);

  return udi_.substr (pos2 + 1);
}

std::string
scanner::info::udi () const
{
  return udi_;
}

bool
scanner::info::is_driver_set () const
{
  return !driver ().empty ();
}

bool
scanner::info::is_local () const
{
  return (2 > path ().find_first_not_of ('/'));
}

bool
scanner::info::is_valid (const std::string& udi)
{
  using std::string;

  if (3 > udi.length ())
    return false;

  if (2 <= udi.find_first_not_of (separator))
    return false;

  string::size_type sep1 = udi.find (separator);
  if (string::npos == sep1)
    return false;

  const string cnx (udi.substr (0, sep1));
  sep1 += 1;

  string::size_type sep2 = udi.find (separator, sep1);
  if (string::npos == sep2)
    return false;

  const string drv (udi.substr (sep1, sep2 - sep1));
  sep2 += 1;

  if (cnx.empty () && drv.empty ())
    return false;

  const regex scheme ("[[:alpha:]][-+.[:alnum:]]*");

  if (!cnx.empty () && !regex_match (cnx, scheme))
    return false;
  if (!drv.empty () && !regex_match (drv, scheme))
    return false;

  return true;
}

const char scanner::info::separator = ':';

}       // namespace utsushi
