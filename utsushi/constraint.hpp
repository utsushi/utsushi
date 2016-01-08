//  constraint.hpp -- imposable on utsushi::value objects
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_constraint_hpp_
#define utsushi_constraint_hpp_

#include <ostream>
#include <stdexcept>

#include <boost/static_assert.hpp>

#include "memory.hpp"
#include "type-traits.hpp"
#include "value.hpp"

namespace utsushi {

//! Impose limitations on allowed values
/*! Settings quite often need to enforce limits on the values they
 *  accept.  Only in the simplest of situations is the programming
 *  language's native type system capable of doing so.  This class
 *  provides a general %constraint interface to support assignment
 *  subject to arbitrary limitations involving a single setting.
 *
 *  The interface has been designed to make %constraint objects act
 *  and feel like regular functions by providing operator().
 *
 *  \sa restriction for limitations that involve multiple settings
 *
 *  \todo  Add a (mathematical) series %constraint (#241)
 *  \todo  Add a regular expression %constraint (#242)
 */
class constraint
{
public:
  typedef shared_ptr< constraint > ptr;

  //! Allow values of a specific type only
  /*! In most all situations, a setting needs to provide an acceptable
   *  \a default_value and maintain the bounded type of its value.
   */
  constraint (const value& default_value);

  virtual ~constraint ();

  //! Determines a %constraint satisfying %value from a %value \a v
  /*! The %value \a v is returned if it possesses the same bounded
   *  type as the object's default %value.  If that is not the case,
   *  the default_value() is returned.
   *
   *  \note
   *  Implementations are at liberty to return a %value \e different
   *  from \a v when \a v does not satisfy the %constraint.  It is
   *  completely up to the implementation to decide what constitutes
   *  an acceptable %value as long as the returned %value satisfies
   *  the %constraint.  That is, for any %constraint \a c and %value
   *  \a v
   *
   *    \code
   *    c(c(v)) == c(v)
   *    \endcode
   *
   *  shall always evaluate to \c true.
   */
  virtual const value& operator() (const value& v) const;

  //! Returns the %constraint's default value
  virtual const value& default_value () const;

  //! Modifies the %constraint's default value
  /*! \throws  a violation when \a v does not satisfy the %constraint
   */
  virtual constraint * default_value (const value& v);

  //! Tells whether only the default_value() is allowed
  virtual bool is_singular () const;

  virtual void operator>> (std::ostream& os) const;

  //! The "anything goes" %constraint symbol
  /*! For the rare cases where one does not even need to maintain the
   *  setting's underlying %value type the \c constraint::none symbol
   *  may be used in group::builder::operator() overloads to say so.
   */
  enum { none };

  class violation : public std::logic_error
  {
  public:
    explicit
    violation (const std::string& arg);
  };

protected:
  constraint ();

  value default_;
};

inline
std::ostream&
operator<< (std::ostream& os, const constraint& c)
{
  c >> os;
  return os;
}

template <typename T>
T * from (const T& t = T ())
{
  BOOST_STATIC_ASSERT ((is_base_of< constraint, T >::value));

  return new T (t);
}

}       // namespace utsushi

#endif  /* utsushi_constraint_hpp_ */
