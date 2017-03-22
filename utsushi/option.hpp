//  option.hpp -- configurable settings in recursive property maps
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

#ifndef utsushi_option_hpp_
#define utsushi_option_hpp_

#include <map>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include <boost/operators.hpp>
#include <boost/throw_exception.hpp>

#include "constraint.hpp"
#include "descriptor.hpp"
#include "functional.hpp"
#include "key.hpp"
#include "memory.hpp"
#include "value.hpp"

namespace utsushi {

using std::bad_cast;
using std::out_of_range;

// FIXME replace with signal emission for actions
class result_code
{
public:
  result_code ();
  result_code (int value, const std::string& msg);

  std::string message () const;
  operator bool () const;

private:
  int         val_;
  std::string msg_;
};

//! Bundle information about a configurable setting
class option
  : boost::equality_comparable< option, value >
{
public:
  //! Obtain an option's value
  operator value () const;

  //! Compare an option's value against \a v
  bool operator== (const value& v) const;

  //! Change an option's value to \a v
  option& operator= (const value& v);

  //! \warning Experimental API (not portable)
  const std::type_info& value_type () const;

  template< typename T >
  T constraint () const;

  constraint::ptr constraint () const;

  std::string key () const;
  //! \copybrief descriptor::name
  string name () const;
  //! \copybrief descriptor::text
  string text () const;

  std::set< utsushi::key > tags () const;

  bool is_at (const level::symbol& level) const;

  //! Whether the option takes effect
  bool is_active () const;
  void active (bool flag);
  //! Whether the software is responsible for achieving the effect
  bool is_emulated () const;
  //! Whether the option's value can be changed
  bool is_read_only () const;

  result_code run ();

  class map;

protected:
  option (option::map& owner, const utsushi::key& k);

  option::map& owner_;
  utsushi::key key_;
};

typedef bool (*restriction) (const value::map&);

//! Organize configurable settings in recursive property maps
/*! \todo Add const overloads for members where possible
 */
class option::map
{
  class builder;

public:
  typedef shared_ptr< map > ptr;
  typedef size_t size_type;

  class iterator;

  map ();

  //! Returns \c size() \c == \c 0
  bool empty () const;
  //! Check how many options have been collected
  size_type size () const;
  //! Returns the largest number of options that can be in the map
  size_type max_size () const;

  //! Get a hold of an option with a given key \a k
  /*! \throws  out_of_range when called with an unknown key
   */
  option operator[] (const utsushi::key& k);

  iterator begin ();
  iterator end ();

  iterator find (const utsushi::key& k);
  size_type count (const utsushi::key& k) const;

  iterator lower_bound (const utsushi::key& k);
  iterator upper_bound (const utsushi::key& k);
  std::pair< iterator, iterator > equal_range (const utsushi::key& k);

  //! Create a snapshot of all current option values
  value::map values () const;

  //! Returns submap of option map with key
  option::map::ptr submap (const utsushi::key& k);

  //! Change a bunch of option values atomically
  void assign (const value::map& vm);

  void impose (const restriction& r);

  builder add_actions ();
  builder add_options ();
  builder add_option_map ();

  void share_values (option::map& om);

  void relink ();               //!< \todo Remove relink() wart

  virtual bool validate (const value::map& vm) const;

protected:
  virtual void finalize (const value::map& vm);

  void insert (const option::map& om);
  void insert (const option::map& om, value::map& vm);
  void insert (const utsushi::key& name_space, const option::map& om);
  void remove (const utsushi::key& k);
  void remove (const option::map& om, value::map& vm);
  void remove (const utsushi::key& name_space, const option::map& om);
  void relink (const option::map& submap);

  //! Prevent std::map<K,T>::operator[] from modifying its map
  /*! This also allows the use of operator[] on constant maps.
   */
  template< typename K, typename T >
  class container
    : public std::map< K, T >
  {
    typedef std::map< K, T > base;

  public:
    typedef typename base::key_type key_type;
    typedef typename base::mapped_type mapped_type;

    mapped_type&
    operator[] (const key_type& k)
    {
      return base::operator[] (k);
    }

    const mapped_type&
    operator[] (const key_type& k) const
    {
      typename base::const_iterator it (this->find (k));

      if (this->end () == it)
        BOOST_THROW_EXCEPTION (out_of_range (k));

      return it->second;
    }
  };

  container< utsushi::key, value::ptr > values_;
  container< utsushi::key, constraint::ptr > constraints_;
  container< utsushi::key, descriptor::ptr > descriptors_;
  container< utsushi::key, function< result_code () > > callbacks_;
  std::vector< restriction > restrictions_;
  std::map< utsushi::key, map::ptr > submaps_;

  option::map *parent_;
  utsushi::key name_space_;

  friend class option;
};

//! Make option::map construction more palatable
class option::map::builder
{
public:
  builder (option::map& owner);
  const builder& operator() (const utsushi::key& k,
                             function< result_code () > f,
                             const string& name,
                             const string& text = string ()) const;
  //! Creates a %value type constrained %option
  const builder& operator() (const utsushi::key& k,
                             const value& v,
                             const aggregator& attr = attributes (),
                             const string& name = string (),
                             const string& text = string ()) const;
  //! Creates an explicitly unconstrained %option
  const builder& operator() (const utsushi::key& k,
                             const value& v, int,
                             const aggregator& attr = attributes (),
                             const string& name = string (),
                             const string& text = string ()) const;
  //! Creates an %option subject to a %constraint \a c
  /*! The option's %value is guaranteed to satisfy %constraint \a c
   *  when the constructor returns.  Note that the %option takes on
   *  ownership of \a c.
   */
  const builder& operator() (const utsushi::key& k,
                             utsushi::constraint *c,
                             const aggregator& attr = attributes (),
                             const string& name = string (),
                             const string& text = string ()) const;
  //! Creates an %option subject to a %constraint \a c
  /*! The option's %value is guaranteed to satisfy %constraint \a c
   *  when the constructor returns.
   */
  const builder& operator() (const utsushi::key& k,
                             utsushi::constraint::ptr cp,
                             const aggregator& attr = attributes (),
                             const string& name = string (),
                             const string& text = string ()) const;
  const builder& operator() (const utsushi::key& name_space,
                             option::map::ptr m) const;

private:
  const builder& operator() (const utsushi::key& k,
                             const value::ptr& vp,
                             const constraint::ptr& cp,
                             const aggregator& attr,
                             const string name,
                             const string text) const;
  option::map& owner_;
};

class option::map::iterator
  : public boost::forward_iterator_helper< option::map::iterator, option >
{
public:
  bool operator== (const iterator& it) const;

  iterator& operator++ ();
  option& operator* () const;

private:
  iterator (option::map& owner,
            const container< utsushi::key, value::ptr >::iterator& it);

  option::map& owner_;
  container< utsushi::key, value::ptr >::iterator it_;

  mutable shared_ptr< option > option_;

  friend class option::map;
};

template< typename T >
T option::constraint () const
{
  T *t = dynamic_cast< T * > (owner_.constraints_[key_].get ());

  if (!t)
    {
      BOOST_THROW_EXCEPTION (bad_cast ());
    }
  return *t;
}

//! Give all configurable objects a common interface
class configurable
{
public:
  option::map::ptr options ();

protected:
  configurable ();

  option::map::ptr option_;
};

}       // namespace utsushi

#endif  /* utsushi_option_hpp_ */
