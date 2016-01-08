//  value.hpp -- union-like construct for various kinds of settings
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

#ifndef utsushi_value_hpp_
#define utsushi_value_hpp_

#include <iosfwd>
#include <map>
#include <typeinfo>

#include <boost/mpl/contains.hpp>
#include <boost/mpl/list.hpp>
#include <boost/operators.hpp>
#include <boost/variant.hpp>

#include "key.hpp"
#include "quantity.hpp"
#include "memory.hpp"
#include "string.hpp"
#include "toggle.hpp"

namespace utsushi {

//! Generic option values
/*! Options can be given values from a select variety of types.
 *  However, those parts of the code that actually care about the
 *  precise type are limited.  Significant parts only use values in
 *  a generic way.  To avoid the need for excessive overloading (or
 *  templating) in other parts of the @PACKAGE_NAME@ API, supported
 *  %value types, so-called bounded types, are collected in a
 *  union-like type.  Unlike the plain C \c union, the %value class
 *  can handle both plain old data types as well as compound types.
 *
 *  A visitation mechanism is supported that allows one to apply() a
 *  visitor to a value.  One can also bind() a visitor to a function
 *  object for later use as callbacks in iterator based algorithms
 *  such as \c std::for_each().
 *
 *  \sa http://wikipedia.org/wiki/Visitor_pattern
 */
class value
  : private boost::equality_comparable< value >
{
public:
  typedef shared_ptr< value > ptr;
  typedef std::map< key, value > map;

  //! Creates an undefined value
  value ();

  value (const quantity& q);
  value (const string& s);
  value (const toggle& t);

  value (const quantity::integer_type& q);
  value (const quantity::non_integer_type& q);

  value (const std::string& s);
  value (const char *str);

  template< typename T > operator T () const;

  bool operator== (const value& val) const;
  const std::type_info& type () const;

  friend
  std::ostream& operator<< (std::ostream& os, const value& val);

  // Visitation rights

  template< typename Visitor >
  typename Visitor::result_type apply (Visitor& v) const;
  template< typename Visitor >
  typename Visitor::result_type apply (Visitor& v);

  template< typename ResultType = void > class visitor;
  template< typename Visitor > class bound;

  template< typename Visitor >
  static bound< Visitor > bind (Visitor& v);

protected:
  //! Meaningful user visible bounded types
  typedef boost::mpl::list<
    quantity
    , string
    , toggle
    > bounded;

public:
  //! Support for undefined default values
  class none
    : private boost::equality_comparable< none >
  {
  public:
    bool operator== (const none&) const;
  };

protected:
  friend
  std::ostream& operator<< (std::ostream& os, const value::none&);
  friend
  std::istream& operator>> (std::istream& is, value::none&);

  //! Mix in an undefined default value type
  typedef boost::mpl::push_front< bounded, none > guts;
  //! Let the compiler figure the final implementation type
  typedef boost::make_variant_over< guts::type >::type impl_type;

  impl_type value_;

public:
  //! Make it easy to write tests that cover all bounded types
  typedef bounded::type types;
};

//! Minimal requirements for any value visitor
/*! Derive visitors from this template class and implement operator()
 *  for \e all bounded types.  The implementations may be templated.
 *  Failure to implement operator() for one or more bounded types will
 *  result in a compilation error.
 */
template< typename ResultType >
class value::visitor
{
public:
  typedef ResultType result_type;

protected:
  visitor () {}
  ~visitor () {}
};

//! Specialization for visitors that don't return anything
/*! This specialization only adds implementations for value::none
 *  types values so that derived classes no longer have to bother
 *  doing so.  It is expected that most visitors will derive from
 *  this specialization.
 */
template<>
class value::visitor<>
{
public:
  typedef void result_type;

  result_type operator() (const value::none&) {}
  result_type operator() (value::none&) {}

protected:
  visitor () {}
  ~visitor () {}
};

//! Turn a visitor into a function object
/*! Applying a visitor to values in a collection typically involves
 *  generic algorithms such as \c std::for_each.  In that case, one
 *  needs a function object that knows how to apply the visitor to a
 *  value.  This class provides such a function object.
 *
 *  Objects are most conveniently constructed via value::bind().
 */
template< typename Visitor >
class value::bound
{
public:
  typedef typename Visitor::result_type result_type;

  bound (Visitor& visitor)
    : visitor_(visitor)
  {}

  result_type operator() (const value& v);
  result_type operator() (value& v);

protected:
  Visitor& visitor_;
};

template< typename T >
value::operator T () const
{
  BOOST_MPL_ASSERT ((boost::mpl::contains< value::types, T >));

  return boost::get< T > (value_);
}

template< typename Visitor >
typename Visitor::result_type
value::apply (Visitor& v) const
{
  return value_.apply_visitor (v);
}

template< typename Visitor >
typename Visitor::result_type
value::apply (Visitor& v)
{
  return value_.apply_visitor (v);
}

template< typename Visitor >
value::bound< Visitor >
value::bind (Visitor& v)
{
  return bound< Visitor > (v);
}

template< typename Visitor >
typename value::bound< Visitor >::result_type
value::bound< Visitor >::operator() (const value& v)
{
  return v.apply (visitor_);
}

template< typename Visitor >
typename value::bound< Visitor >::result_type
value::bound< Visitor >::operator() (value& v)
{
  return v.apply (visitor_);
}

}       // namespace utsushi

#endif  /* utsushi_value_hpp_ */
