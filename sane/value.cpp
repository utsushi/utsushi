//  value.cpp -- mediate between utsushi::value and SANE API conventions
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/store.hpp>

#include "value.hpp"

namespace sane {

using namespace utsushi;
using std::bad_cast;
using std::logic_error;

/*! \defgroup visitor  Visitor Objects
 *  \todo  Explain visitation mechanism.
 *  @{
 */

//! Stuff SANE frontend managed memory into a bounded type
struct get
  : boost::static_visitor<>
  , utsushi::value
{
  const void *v_;
  const SANE_Value_Type& t_;

  get (const void *v, const SANE_Value_Type& t)
    : v_(v), t_(t)
  {}

  void operator() (value::none&) const
  {}

  void operator() (quantity& q) const
  {
    /**/ if (SANE_TYPE_INT   == t_)
      {
        q = * static_cast< const SANE_Int * > (v_);
      }
    else if (SANE_TYPE_FIXED == t_)
      {
        q = SANE_UNFIX (* static_cast< const SANE_Fixed * > (v_));
      }
    else
      {
        BOOST_THROW_EXCEPTION
          (logic_error ("internal inconsistency"));
      }
  }

  void operator() (string& s) const
  {
    s = static_cast< const SANE_String_Const > (v_);
  }

  void operator() (toggle& t) const
  {
    t = * static_cast< const SANE_Bool * > (v_);
  }
};

//! Stuff a bounded type into SANE frontend managed memory
struct put
  : boost::static_visitor<>
  , utsushi::value
{
  void *v_;

  put (void *v)
    : v_(v)
  {}

  void operator() (const value::none&) const
  {}

  void operator() (const quantity& q) const
  {
    if (q.is_integral ())
      * static_cast< SANE_Int   * > (v_) = q.amount< SANE_Int > ();
    else
      * static_cast< SANE_Fixed * > (v_) = SANE_FIX (q.amount< double > ());
  }

  void operator() (const string& s) const
  {
    SANE_String v = static_cast< SANE_String > (v_);

    s.copy (v, s.size ());
    v[s.size ()] = '\0';
  }

  void operator() (const toggle& t) const
  {
    * static_cast< SANE_Bool * > (v_) = t;
  }
};

//! Map bounded value type sizes to SANE type sizes
struct size_mapper
  : boost::static_visitor< size_t >
  , utsushi::value
{
  size_t operator() (const value::none&) const
  {
    return 0;
  }

  size_t operator() (const quantity&) const
  {
    BOOST_STATIC_ASSERT ((sizeof (SANE_Word) == sizeof (SANE_Int)));
    BOOST_STATIC_ASSERT ((sizeof (SANE_Word) == sizeof (SANE_Fixed)));

    return sizeof (SANE_Word);
  }

  size_t operator() (const string& s) const
  {
    return s.size () + 1;
  }

  size_t operator() (const toggle&) const
  {
    return sizeof (SANE_Bool);
  }
};

//! Map bounded value types to SANE types
struct type_mapper
  : boost::static_visitor< SANE_Value_Type >
  , utsushi::value
{
  //! \todo Review whether utsushi::value really supports value::none
  SANE_Value_Type operator() (const value::none&) const
  {
    return SANE_TYPE_BUTTON;
  }

  SANE_Value_Type operator() (const quantity& q) const
  {
    return (q.is_integral ()
            ? SANE_TYPE_INT
            : SANE_TYPE_FIXED);
  }

  SANE_Value_Type operator() (const string&) const
  {
    return SANE_TYPE_STRING;
  }

  SANE_Value_Type operator() (const toggle&) const
  {
    return SANE_TYPE_BOOL;
  }
};

//! Maps utsushi::quantity units to SANE units
struct unit_mapper
  : boost::static_visitor< SANE_Unit >
{
  //! Many bounded types do not have any units associated with them
  /*! \rationale
   *  Units only make sense in the context of numeric bounded types.
   *  Hence, we can do the same thing for all non-numeric types.
   */
  template< typename T >
  SANE_Unit operator() (const T&) const
  {
    return SANE_UNIT_NONE;
  }
};

//! Specialisation for numeric types
template<>
SANE_Unit
unit_mapper::operator() (const quantity& q) const
{
  return SANE_UNIT_NONE;
}

/*! @} */

struct multiply_by
  : boost::static_visitor<>
{
  const quantity& factor_;

  multiply_by (const quantity& factor)
    : factor_(factor)
  {}

  template< typename T >
  void operator() (T&) const
  {
    BOOST_THROW_EXCEPTION
      (logic_error("value type does not support multiplication"));
  }
};

template<>
void
multiply_by::operator() (quantity& q) const
{
  q *= factor_;
}

struct divide_by
  : boost::static_visitor<>
{
  const quantity& factor_;

  divide_by (const quantity& factor)
    : factor_(factor)
  {}

  template< typename T >
  void operator() (T&) const
  {
    BOOST_THROW_EXCEPTION
      (logic_error ("value type does not support division"));
  }
};

template<>
void
divide_by::operator() (quantity& q) const
{
  q /= factor_;
}

value::value (const utsushi::value& uv)
  : utsushi::value (uv)
{}

value::value (const utsushi::value& uv, const constraint::ptr& cp)
  : utsushi::value (uv)
  , cp_(cp)
{}

value::value (const utsushi::quantity& q, const SANE_Value_Type& type)
{
  BOOST_ASSERT (   type == SANE_TYPE_INT
                || type == SANE_TYPE_FIXED);

  if (SANE_TYPE_INT == type)
    {
      value_ = quantity (q.amount< quantity::integer_type > ());
    }
  if (SANE_TYPE_FIXED == type)
    {
      value_ = 1.0 * q;
    }
}

value::value (const option& gv)
  : utsushi::value (gv)
  , cp_(gv.constraint ())
{}

//! \todo Determine suitable fallback at run-time (during sane_init)
/*! \bug  Assumes all store elements for a value with string bounded
 *        type are also values with string bounded type.  Note, SANE
 *        API limitations require that to be the case though.
 */
SANE_Int
value::size () const
{
  size_t rv = boost::apply_visitor (size_mapper (), value_);

  if (SANE_TYPE_STRING == type ())
    {
      using std::max;
      if (store *sp = dynamic_cast< store * > (cp_.get ()))
        {
          store::const_iterator it;
          for (it = sp->begin (); sp->end () != it; ++it)
            {
              string s = *it;
              rv = max (rv, s.size () + 1);
            }
        }
      else
        {
          rv = max (rv, size_t (default_string_size));
        }
    }

  return std::min (rv, size_t (std::numeric_limits< SANE_Int >::max()));
}

SANE_Value_Type
value::type () const
{
  return boost::apply_visitor (type_mapper (), value_);
}

SANE_Unit
value::unit () const
{
  return boost::apply_visitor (unit_mapper (), value_);
}

value&
value::operator*= (const quantity& factor)
{
  boost::apply_visitor (multiply_by (factor), value_);
  return *this;
}

value&
value::operator/= (const quantity& factor)
{
  boost::apply_visitor (divide_by (factor), value_);
  return *this;
}

const value&
value::operator>> (void *v) const
{
  boost::apply_visitor (put (v), value_);
  return *this;
}

value&
value::operator<< (const void *v)
{
  boost::apply_visitor (get (v, type ()), value_);
  return *this;
}

}       // namespace sane
