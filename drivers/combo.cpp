//  combo.cpp -- API implementation for an combo driver
//  Copyright (C) 2017  SEIKO EPSON CORPORATION
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

#include <boost/foreach.hpp>
#include <boost/throw_exception.hpp>

#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"
#include "utsushi/regex.hpp"

#include "utsushi/range.hpp"
#include "utsushi/store.hpp"

#include "combo.hpp"

#define for_each BOOST_FOREACH

namespace utsushi {

extern "C" {
void
libdrv_combo_LTX_scanner_factory (const scanner::info& info, scanner::ptr& rv)
{
  const std::string kv ("([^&=]+)=([^&]+)");
  const std::string path (info.query ());
  regex re (kv);
  sregex_iterator pos (path.begin(), path.end(), re);
  sregex_iterator end;

  _drv_::combo::kv_list kvs;
  for (; pos != end; ++pos)
    {
      kvs.push_back (std::make_pair (pos->str(1), pos->str(2)));
    }

  rv = make_shared< _drv_::combo::scanner > (kvs, info.enable_debug ());
}
}       // extern "C"

namespace _drv_ {
namespace combo {

typedef std::list< std::pair< string, std::string > > kv_list;

scanner::scanner (const kv_list& kvs, const bool debug)
  : utsushi::scanner::scanner (connexion::ptr ())
  , key ("doc-source")
{
  kv_list::const_iterator it;

  bool seen = false;
  for (it = kvs.begin (); !seen && kvs.end () != it; ++it)
    {
      if (it->first == "key")
        {
          key = it->second;
          log::brief ("switching device based on '%1%'") % key;
          seen = true;
        }
    }

  store::ptr s (make_shared< store > ());
  for (it = kvs.begin (); kvs.end () != it; ++it)
    {
      if (it->first == "key") continue;

      scanner::info info (it->second);
      info.enable_debug (debug);
      scanner::ptr sp = scanner::create (info);

      if (!sp)
        {
          scanners.clear ();
          BOOST_THROW_EXCEPTION
            (std::runtime_error
             ((format ("cannot create scanner for '%1%'")
               % it->second).str ()));
        }

      string val = it->first;
      if (sp->options ()->count (key))
        {
          // Set key option to it->first (or a shorthand expansion).
          // It may not be the default.

          constraint::ptr cp ((*sp->options ())[key].constraint ());

          if (cp && val == (*cp) (val))
            {
              (*sp->options ())[key] = val;
            }
          else
            {                   // expand shorthand
              if (val == "adf") val = "ADF";
              if (val == "fb")  val = "Document Table";
              if (!cp || val == (*cp) (val))
                {
                  (*sp->options ())[key] = val;
                }
              else
                {
                  scanners.clear ();
                  BOOST_THROW_EXCEPTION
                    (std::runtime_error
                     ((format ("scanner '%1%' does not support %2%=='%3%'")
                       % it->second % key % val).str ()));
                }
            }
          log::brief ("adding scanner '%1%' to handle %2%=='%3%'")
            % it->second % key % val;
        }
      scanners[val] = sp;
      s->alternative (val);

      if (kvs.begin () == it)   // make it the default device
        {
          s->default_value (val);
          active_scanner = scanners[val];
        }
    }

  string name = (*active_scanner->options ())[key].name ();

  configure_options (key, s, name);

  if (!validate (values ()))
    {
      BOOST_THROW_EXCEPTION
        (std::logic_error
         ("combo::scanner(): internal inconsistency"));
    }
  finalize (values ());
}

scanner::~scanner ()
{
  cancel ();
  scanners.clear ();
}

connection
scanner::connect_marker (const marker_signal_type::slot_type& slot) const
{
  return active_scanner->connect_marker (slot);
}

connection
scanner::connect_update (const update_signal_type::slot_type& slot) const
{
  return active_scanner->connect_update (slot);
}

streamsize
scanner::read (octet *data, streamsize n)
{
  return active_scanner->read (data, n);
}

streamsize
scanner::marker ()
{
  return active_scanner->marker ();
}

void
scanner::cancel ()
{
  active_scanner->cancel ();
}

context
scanner::get_context () const
{
  return active_scanner->get_context ();
}

option::map::ptr
scanner::options ()
{
  return active_scanner->options ();
}

streamsize
scanner::buffer_size () const
{
  return active_scanner->buffer_size ();
}

bool
scanner::is_single_image () const
{
  return active_scanner->is_single_image ();
}

void
scanner::configure_options (const std::string& key, store::ptr s,
                            const std::string& name)
{
  combo_opts.add_options ()     // add the device switching option
    (key, s,
     attributes (tag::general)(level::standard),
     name);

  // TODO Generalize the implementation so it does not depend too much
  //      on the particular scanners that we picked up via the UDI.

  combo_opts.add_options ()
    ("sw-resolution", (from< range > ()
                    -> lower (50)
                    -> upper (600)
                    -> default_value (50)),
     attributes (tag::general)(level::standard),
     SEC_N_("Resolution"));

  combo_opts.add_options ()
    ("transfer-format", (from< store > ()
                    -> alternative ("RAW")
                    -> alternative ("JPEG")
                    -> default_value ("JPEG")),
     attributes (level::standard),
     SEC_N_("Transfer Format"));

  // TODO Add other options handled by the combo instance itself

  insert (combo_opts);
  insert (*active_scanner->options ());
  for_each (map::value_type elem, scanners)
    {                           // insert options of other scanners
      if (elem.second == active_scanner) continue;
      insert (*elem.second->options ());
    }
}

bool
scanner::validate (const value::map& vm) const
{
  scanner::ptr validator (active_scanner);

  if (vm.count (key))
    {
      if (scanners.count (vm.at (key)))
        validator = scanners.at (vm.at (key));
      else
        return false;
    }

  option::map& other_opts (*validator->options ());
  value::map   combo_vm (combo_opts.values ());
  value::map   other_vm (other_opts.values ());

  for_each (value::map::value_type elem, vm)
    {
      value::map::key_type    k = elem.first;
      value::map::mapped_type v = elem.second;

      if (combo_vm.count (k))
        {
          combo_vm[k] = v;
          continue;
        }
      if (other_vm.count (k))
        {
          other_vm[k] = v;
          if (   k == "tl-x" || k == "tl-y" || k == "br-x" || k == "br-y"
              || k == "scan-area"
              || k == "resolution"
            )
            {
              constraint::ptr c (other_opts[k].constraint ());
              if (v != (*c) (v))
                {
                  other_vm[k] = c->default_value ();
                }
            }
          continue;
        }
    }

  if (!combo_opts.validate (combo_vm)) return false;

  // TODO Override other_vm values for options the combo instance
  //      handles, insofar necessary

  return other_opts.validate (other_vm);
}

void
scanner::finalize (const value::map& vm)
{
  scanner::ptr finalizer (active_scanner);

  if (vm.count (key))
    {
      if (scanners.count (vm.at (key)))
        finalizer = scanners[vm.at (key)];
      else
        BOOST_THROW_EXCEPTION
          (constraint::violation
           ((format ("no device for %1%='%2%'") % key % vm.at (key)).str ()));
    }

  option::map& other_opts (*finalizer->options ());
  value::map   combo_vm (combo_opts.values ());
  value::map   other_vm (other_opts.values ());
  value::map   final_vm (vm);

  if (finalizer != active_scanner)
    {
      remove (*active_scanner->options (), final_vm);
      insert (combo_opts, final_vm);
      insert (other_opts, final_vm);
    }

  for_each (value::map::value_type elem, final_vm)
    {
      if (combo_vm.count (elem.first))
        {
          combo_vm[elem.first] = elem.second;
          continue;
        }
      if (other_vm.count (elem.first))
        {
          other_vm[elem.first] = elem.second;
          continue;
        }

      if (*values_[elem.first] != elem.second)
        std::cerr << string(elem.first)
                  << " would be inactive after change\n"
                  << "ignoring attempt to change its value\n";
    }

  combo_opts.assign (combo_vm);

  // pass combo option to actual device option
  other_vm["sw-resolution"] = combo_vm["sw-resolution"];

  other_opts.assign (other_vm);
  active_scanner = finalizer;
  relink ();
}

}       // namespace combo
}       // namespace _drv_
}       // namespace utsushi
