//  scanner.cpp -- interface and support classes
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

#include <cstring>
#include <ltdl.h>

#include <list>
#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/throw_exception.hpp>

#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"
#include "utsushi/run-time.hpp"
#include "utsushi/scanner.hpp"

namespace utsushi {

using std::invalid_argument;
using std::runtime_error;
using boost::filesystem::path;

typedef scanner::ptr (*scanner_factory) (connexion::ptr);

scanner_factory
get_scanner_factory (const lt_dlhandle& handle)
{
  return reinterpret_cast< scanner_factory > (lt_dlsym
                                              (handle, "scanner_factory"));
}

scanner::ptr
scanner::create (connexion::ptr cnx, const scanner::id& id)
{
  if (!id.has_driver ())
    {
      log::error ("driver not known for %1% (%2%)")
        % id.name ()
        % id.udi ()
        ;
      return scanner::ptr ();
    }

  std::string plugin = "libdrv-" + id.driver ();

  lt_dlhandle handle = NULL;
  scanner_factory factory = 0;
  std::string error (_("driver not found"));

# if (HAVE_LT_DLADVISE)
    {
      log::brief ("looking for preloaded '%1%' driver")
        % id.driver ();

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
                % id.driver ();
            }
          else
            {
              lt_dlclose (handle);
            }
        }
      lt_dladvise_destroy (&advice);
    }
# endif /* HAVE_LT_DLADVISE */

  if (!factory) {               // trawl the file system
    run_time rt;
    run_time::sequence_type search (rt.load_dirs (run_time::pkg, "driver"));

  run_time::sequence_type::const_iterator it;
  for (it = search.begin (); !handle && it != search.end (); ++it)
    {
      path p (*it);

      log::brief ("looking for '%1%' driver in '%2%'")
        % id.driver ()
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

  return factory (cnx);
}

const std::string default_type_("scanner");

bool
scanner::id::invalid_(const std::string& fragment) const
{
  const std::string lower = "abcdefghijklmnopqrstuvwxyz";
  const std::string upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  const std::string valid = lower + upper;

  return std::string::npos != fragment.find_first_not_of (valid);
}

bool
scanner::id::promise_(bool nothrow) const
{
  bool all_is_well = true;

  std::string::size_type pos = udi_.find (separator);

  {                             // non-empty interface fragment
    if (all_is_well)
      all_is_well = (std::string::npos != pos && 0 != pos);
    if (all_is_well)
      all_is_well = !invalid_(iftype ());
  }
  {                             // possibly empty driver fragment
    if (all_is_well)
      all_is_well = (std::string::npos != udi_.find (separator, pos + 1));
    if (all_is_well)
      all_is_well = !has_driver () || !invalid_(driver ());
  }

  if (!nothrow && !all_is_well)
    {
      BOOST_THROW_EXCEPTION
        (invalid_argument
         ((format (_("syntax error: invalid udi '%1%'")) % udi_).str ()));
    }

  return all_is_well;
}

scanner::id::id (const std::string& udi)
  : udi_(udi)
  , type_(default_type_)
{
  promise_();
}

scanner::id::id (const std::string& model, const std::string& vendor,
                 const std::string& udi)
  : udi_(udi)
  , model_(model)
  , vendor_(vendor)
  , type_(default_type_)
{
  promise_();
}

scanner::id::id (const std::string& model, const std::string& vendor,
                 const std::string& path, const std::string& iftype,
                 const std::string& driver)
  : model_(model)
  , vendor_(vendor)
  , type_(default_type_)
{
  if (invalid_(iftype))
    {
      BOOST_THROW_EXCEPTION
        (invalid_argument
         ((format (_("syntax error: interface name '%1%' not valid")
                   ) % iftype).str ()));
    }
  if (!driver.empty () && invalid_(driver))
    {
      BOOST_THROW_EXCEPTION
        (invalid_argument
         ((format (_("syntax error: driver name '%1%' not valid")
                   ) % driver).str ()));
    }
  udi_ = iftype + separator + driver + separator + path;

  promise_();
}

std::string
scanner::id::path () const
{
  std::string::size_type pos1 = udi_.find (separator) + 1;
  std::string::size_type pos2 = udi_.find (separator, pos1);

  return udi_.substr (pos2 + 1);
}

std::string
scanner::id::name () const
{
  if (!nick_.empty ())
    {
      return nick_;
    }

  if (0 == model_.find (vendor_))
    {
      return model_;
    }

  return vendor_ + " " + model_;
}

std::string
scanner::id::text () const
{
  return text_;
}

std::string
scanner::id::model () const
{
  return model_;
}

std::string
scanner::id::vendor () const
{
  return vendor_;
}

std::string
scanner::id::type () const
{
  return type_;
}

std::string
scanner::id::driver () const
{
  std::string::size_type pos1 = udi_.find (separator) + 1;
  std::string::size_type pos2 = udi_.find (separator, pos1);

  return udi_.substr (pos1, pos2 - pos1);
}

std::string
scanner::id::iftype () const
{
  return udi_.substr (0, udi_.find (separator));
}

std::string
scanner::id::udi () const
{
  return udi_;
}

void
scanner::id::name (const std::string& name)
{
  nick_ = name;
}

void
scanner::id::text (const std::string& text)
{
  text_ = text;
}

void
scanner::id::driver (const std::string& driver)
{
  std::string::size_type pos1 = udi_.find (separator) + 1;

  if (has_driver ())
    {
      std::string::size_type pos2 = udi_.find (separator, pos1);
      udi_.replace (pos1, pos2 - pos1, driver);
    }
  else
    {
      udi_.insert (pos1, driver);
    }

  promise_();
}

bool
scanner::id::is_configured () const
{
  return "virtual" == iftype ();
}

bool
scanner::id::is_local () const
{
  return "net" != iftype ();
}

bool
scanner::id::has_driver () const
{
  return ! driver ().empty ();
}

bool
scanner::id::is_valid (const std::string& udi)
{
  id id (udi, true);
  return id.promise_(true);
}

const char scanner::id::separator = ':';

scanner::id::id (const std::string& udi, bool nocheck)
  : udi_(udi)
{}

}       // namespace utsushi
