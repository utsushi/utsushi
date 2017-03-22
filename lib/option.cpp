//  option.cpp -- configurable settings in recursive property maps
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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
#include "utsushi/option.hpp"
#include "utsushi/range.hpp"
#include "utsushi/store.hpp"

#define for_each BOOST_FOREACH

using std::logic_error;
using std::out_of_range;

namespace utsushi {

result_code::result_code ()
  : val_(0)
  , msg_("Success")
{}

result_code::result_code (int value, const std::string& msg)
  : val_(value)
  , msg_(msg)
{}

std::string
result_code::message () const
{
  return msg_;
}

result_code::operator bool () const
{
  return 0 != val_;
}

option::operator value () const
{
  return *owner_.values_[key_];
}

bool
option::operator== (const value& v) const
{
  return v == *owner_.values_[key_];
}

option&
option::operator= (const value & v)
{
  value::map vm;

  vm[key_] = v;
  owner_.assign (vm);

  return *this;
}

const std::type_info&
option::value_type () const
{
  if (!constraint ())
    return typeid (void);

  return owner_.values_[key_]->type ();
}

constraint::ptr
option::constraint () const
{
  return owner_.constraints_[key_];
}

std::string
option::key () const
{
  return key_;
}

string
option::name () const
{
  return owner_.descriptors_[key_]->name ();
}

string
option::text () const
{
  return owner_.descriptors_[key_]->text ();
}

std::set< utsushi::key >
option::tags () const
{
  return owner_.descriptors_[key_]->tags ();
}

bool
option::is_at (const level::symbol& level) const
{
  return owner_.descriptors_[key_]->is_at (level);
}

bool
option::is_emulated () const
{
  return owner_.descriptors_[key_]->is_emulated ();
}

bool
option::is_active () const
{
  return owner_.descriptors_[key_]->is_active ();
}

void
option::active (bool flag)
{
  owner_.descriptors_[key_]->active (flag);
}

bool
option::is_read_only () const
{
  return (owner_.constraints_[key_]->is_singular ()
          || owner_.descriptors_[key_]->is_read_only ());
}

result_code
option::run ()
{
  return owner_.callbacks_[key_] ();
}

option::option (option::map& owner, const utsushi::key& k)
  : owner_(owner), key_(k)
{
  if (!owner_.values_.count (key_))
    {
      BOOST_THROW_EXCEPTION (out_of_range (key_));
    }
}

option::map::map ()
  : parent_(nullptr)
  , name_space_()
{}

bool
option::map::empty () const
{
  return values_.empty ();
}

option::map::size_type
option::map::size () const
{
  return values_.size ();
}

option::map::size_type
option::map::max_size () const
{
  return values_.max_size ();
}

option
option::map::operator[] (const utsushi::key& k)
{
  return option (*this, k);
}

option::map::iterator
option::map::begin ()
{
  return iterator (*this, values_.begin ());
}

option::map::iterator
option::map::end ()
{
  return iterator (*this, values_.end ());
}

option::map::iterator
option::map::find (const utsushi::key& k)
{
  return iterator (*this, values_.find (k));
}

option::map::size_type
option::map::count (const utsushi::key& k) const
{
  return values_.count (k);
}

option::map::iterator
option::map::lower_bound (const utsushi::key& k)
{
  return iterator (*this, values_.lower_bound (k));
}

option::map::iterator
option::map::upper_bound (const utsushi::key& k)
{
  return iterator (*this, values_.upper_bound (k));
}

std::pair< option::map::iterator, option::map::iterator >
option::map::equal_range (const utsushi::key& k)
{
  return std::make_pair (iterator (*this, values_.lower_bound (k)),
                         iterator (*this, values_.upper_bound (k)));
}

value::map
option::map::values () const
{
  value::map vm;

  container< utsushi::key, value::ptr >::const_iterator vit;
  for (vit = values_.begin (); values_.end () != vit; ++vit)
    {
      vm[vit->first] = *vit->second;
    }
  return vm;
}

option::map::ptr
option::map::submap (const utsushi::key& k)
{
  option::map::ptr sm;
  try
    {
      sm = submaps_.at (k);
    }
  catch (const std::out_of_range&)
    {
      log::error ("No such submap [%1%]") % k;
    }
  return sm;
}

void
option::map::assign (const value::map& vm)
{
  value::map candidate (values ());

  for_each (value::map::value_type element, vm)
    {
      candidate[element.first] = element.second;
    }

  if (validate (candidate))
    {
      finalize (candidate);
    }
  else
    {
      log::debug ("Invalid value combination");
      for_each (value::map::value_type element, vm)
        log::debug ("%1% = %2%") % string (element.first) % element.second;

      //! \todo generate (ranked) candidate alternatives to choose from
      BOOST_THROW_EXCEPTION
        (constraint::violation ("value combination not acceptable"));
    }
}

void
option::map::impose (const restriction& r)
{
  restrictions_.push_back (r);
}

option::map::builder
option::map::add_actions ()
{
  return builder (*this);
}

option::map::builder
option::map::add_options ()
{
  return builder (*this);
}

option::map::builder
option::map::add_option_map ()
{
  return builder (*this);
}

void
option::map::share_values (option::map& om)
{
  container< utsushi::key, value::ptr >::iterator vit;
  for (vit = om.values_.begin (); om.values_.end () != vit; ++vit)
    {
      container< utsushi::key, value::ptr >::iterator vjt;

      vjt = values_.find (vit->first);
      if (values_.end () != vjt)
        {
          vit->second = vjt->second;
        }
    }
}

// Helper function to let validate() and finalize() call the member
// function implementation of its submaps.  This is an ugly kludge
// until we reimplement option::map with an inherently recursive
// container.
static
std::map< std::string, value::map >
split (const value::map& vm)
{
  std::map< std::string, value::map > rv;

  value::map::const_iterator it;
  for (it = vm.begin (); vm.end () != it; ++it)
    {
      std::string s (it->first);
      std::string::size_type pos (s.find ("/"));

      std::string head;
      std::string tail;

      if (std::string::npos == pos)
        {
          tail = s;
        }
      else
        {
          head = s.substr (0, pos);
          tail = s.substr (pos + 1);
        }
      rv[head][tail] = it->second;
    }
  return rv;
}

bool
option::map::validate (const value::map& vm) const
{
  bool satisfied = true;

  std::map< std::string, value::map > submaps (split (vm));

  std::map< std::string, value::map >::const_iterator it;
  for (it = submaps.begin (); satisfied && submaps.end () != it; ++it)
    {
      if (it->first.empty ())
        {
          value::map submap (it->second);
          value::map::const_iterator jt;
          for (jt = submap.begin (); submap.end () != jt; ++jt)
            {
              if (!values_.count (jt->first))
                {
                  return false;
                }
              if (constraints_[jt->first])
                {
                  value okay = (*constraints_[jt->first]) (jt->second);
                  if (jt->second != okay)
                    {
                      return false;
                    }
                }
            }

          std::vector< restriction >::const_iterator rit;
          for (rit = restrictions_.begin ();
               satisfied && restrictions_.end () != rit; ++rit)
            {
              satisfied &= (*rit) (vm);
            }
        }
      else
        {
          satisfied &= (submaps_.find (it->first)->second.get ()
                        ->validate (it->second));
        }
    }
  return satisfied;
}

void
option::map::finalize (const value::map& vm)
{
  std::map< std::string, value::map > submaps (split (vm));

  std::map< std::string, value::map >::const_iterator it;
  for (it = submaps.begin (); submaps.end () != it; ++it)
    {
      if (it->first.empty ())
        {
          value::map submap (it->second);
          value::map::const_iterator jt;
          for (jt = submap.begin (); submap.end () != jt; ++jt)
            {
              *values_[jt->first] = jt->second;
            }
        }
      else
        {
          submaps_.find (it->first)->second.get ()->finalize (it->second);
        }
    }
}

void
option::map::insert (const option::map& om)
{
  values_.insert (om.values_.begin (), om.values_.end ());
  constraints_.insert (om.constraints_.begin (), om.constraints_.end ());
  descriptors_.insert (om.descriptors_.begin (), om.descriptors_.end ());
}

void
option::map::insert (const option::map& om, value::map& vm)
{
  insert (om);

  container< utsushi::key, value::ptr >::const_iterator vit;
  for (vit = om.values_.begin (); om.values_.end () != vit; ++vit)
    {
      if (!vm.count (vit->first))
        vm[vit->first] = *(vit->second);
    }

  if (parent_) parent_->insert (name_space_, om);
}

void
option::map::insert (const utsushi::key& name_space, const option::map& om)
{
  container< utsushi::key, value::ptr >::const_iterator it;

  for (it = om.values_.begin (); om.values_.end () != it; ++it)
    {
      utsushi::key k = name_space / it->first;

      values_[k] = it->second;
      constraints_[k] = om.constraints_.find (it->first)->second;
      descriptors_[k] = om.descriptors_.find (it->first)->second;
    }

  if (parent_) parent_->insert (name_space_ / name_space, om);
}

void
option::map::remove (const utsushi::key& k)
{
  values_.erase (k);
  constraints_.erase (k);
  descriptors_.erase (k);

  if (parent_) parent_->remove (name_space_ / k);
}

void
option::map::remove (const option::map& om, value::map& vm)
{
  container< utsushi::key, value::ptr >::const_iterator vit;
  for (vit = om.values_.begin (); om.values_.end () != vit; ++vit)
    {
      values_.erase (vit->first);
      if (vm.count (vit->first)) vm.erase (vit->first);
    }

  container< utsushi::key, constraint::ptr >::const_iterator cit;
  for (cit = om.constraints_.begin (); om.constraints_.end () != cit; ++cit)
    constraints_.erase (cit->first);

  container< utsushi::key, descriptor::ptr >::const_iterator dit;
  for (dit = om.descriptors_.begin (); om.descriptors_.end () != dit; ++dit)
    descriptors_.erase (dit->first);

  if (parent_) parent_->remove (name_space_, om);
}

void
option::map::remove (const utsushi::key& name_space, const option::map& om)
{
  container< utsushi::key, value::ptr >::const_iterator it;

  for (it = om.values_.begin (); om.values_.end () != it; ++it)
    {
      utsushi::key k = name_space / it->first;

      values_.erase (k);
      constraints_.erase (k);
      descriptors_.erase (k);
    }

  if (parent_) parent_->remove (name_space_ / name_space, om);
}

void
option::map::relink ()
{
  if (parent_) parent_->relink (*this);
}

void
option::map::relink (const option::map& om)
{
  if (om.parent_ != this)
    {
      log::error ("relink request from non-child");
      return;
    }

  container< utsushi::key, constraint::ptr >::const_iterator cit;
  for (cit = om.constraints_.begin (); om.constraints_.end () != cit; ++cit)
    {
      constraints_[om.name_space_ / cit->first] = cit->second;
    }

  if (parent_) parent_->relink ();
}

option::map::builder::builder (option::map& owner)
  : owner_(owner)
{}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& k,
                                  function< result_code () > f,
                                  const string& name,
                                  const string& text) const
{
  operator() (k, value (), attributes (), name, text);

  owner_.callbacks_[k] = f;

  return *this;
}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& k,
                                  const value& v,
                                  const aggregator& attr,
                                  const string& name,
                                  const string& text) const
{
  constraint::ptr cp = make_shared< utsushi::constraint > (v);
  value::ptr      vp = make_shared< value > (v);

  return operator() (k, vp, cp, attr, name, text);
}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& k,
                                  const value& v, int,
                                  const aggregator& attr,
                                  const string& name,
                                  const string& text) const
{
  constraint::ptr cp;
  value::ptr      vp = make_shared< value > (v);

  return operator() (k, vp, cp, attr, name, text);
}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& k,
                                  utsushi::constraint *c,
                                  const aggregator& attr,
                                  const string& name,
                                  const string& text) const
{
  constraint::ptr cp (c);
  value::ptr      vp = make_shared< value > ((*c) (value ()));

  return operator() (k, vp, cp, attr, name, text);
}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& k,
                                  utsushi::constraint::ptr cp,
                                  const aggregator& attr,
                                  const string& name,
                                  const string& text) const
{
  value::ptr vp = make_shared< value > ((*cp) (value ()));

  return operator() (k, vp, cp, attr, name, text);
}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& name_space,
                                  option::map::ptr m) const
{
  if (&owner_ == m.get ())
    BOOST_THROW_EXCEPTION
      (logic_error ("cannot add option::map to self"));

  container< utsushi::key, value::ptr >::const_iterator it;

  for (it = m->values_.begin (); m->values_.end () != it; ++it)
    {
      utsushi::key k = name_space / it->first;

      if (owner_.values_.count (k))
        {
          BOOST_THROW_EXCEPTION (logic_error (k));
        }
      owner_.values_[k] = it->second;
      owner_.constraints_[k] = m->constraints_.find (it->first)->second;
      owner_.descriptors_[k] = m->descriptors_.find (it->first)->second;
    }
  owner_.submaps_.insert (std::make_pair (name_space, m));

  m->parent_ = &owner_;
  m->name_space_ = name_space;

  return *this;
}

const option::map::builder&
option::map::builder::operator() (const utsushi::key& k,
                                  const value::ptr& vp,
                                  const constraint::ptr& cp,
                                  const aggregator& attr,
                                  const string name,
                                  const string text) const
{
  if (owner_.values_.count (k))
    {
      BOOST_THROW_EXCEPTION (logic_error (k));
    }

  descriptor::ptr dp = make_shared< descriptor > (attr);
  dp->name (name);
  dp->text (text);

  owner_.values_[k] = vp;
  owner_.constraints_[k] = cp;
  owner_.descriptors_[k] = dp;

  return *this;
}

bool
option::map::iterator::operator== (const option::map::iterator& it) const
{
  //  it_ points to an item in a collection owned by owner_, hence
  //  just comparing it_ is sufficient.  This way we don't have to
  //  provide option::map::operator==

  return it_ == it.it_;
}

option::map::iterator&
option::map::iterator::operator++ ()
{
  ++it_;
  return *this;
}

option&
option::map::iterator::operator* () const
{
  //  option_ holds on to the shared pointer so we can pass the caller
  //  a non-temporary reference

  if (!option_ || option_->key () != it_->first)
    {
      option_ = shared_ptr< option >
        (new option (owner_, it_->first));
    }
  return *option_;
}

option::map::iterator::iterator (option::map& owner,
                                 const container< utsushi::key,
                                                  value::ptr >::iterator& it)
  : owner_(owner), it_(it)
{}

configurable::configurable ()
  : option_(new option::map)
{}

option::map::ptr
configurable::options ()
{
  return option_;
}

}       // namespace utsushi
