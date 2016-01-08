//  quantity.cpp -- bounded type for utsushi::value objects
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

#include <sstream>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include "utsushi/quantity.hpp"

namespace utsushi {

#define BINARY_OPERATOR_(OPERATOR_NAME_,OPERATOR_SYMBOL_)       \
  struct OPERATOR_NAME_                                         \
    : public boost::static_visitor< quantity >                  \
  {                                                             \
    template< typename T1, typename T2 >                        \
    quantity operator() (T1& lhs, const T2& rhs) const          \
    {                                                           \
      return lhs OPERATOR_SYMBOL_ rhs;                          \
    }                                                           \
  };                                                            \
  /**/

BINARY_OPERATOR_(increment_by_, +=);
BINARY_OPERATOR_(decrement_by_, -=);
BINARY_OPERATOR_(multiply_by_, *=);
BINARY_OPERATOR_(divide_by_, /=);

template<>
quantity
multiply_by_::operator() (quantity::integer_type& lhs,
                          const quantity::non_integer_type& rhs) const
{
  return rhs * lhs;
}

template<>
quantity
divide_by_::operator() (quantity::integer_type& lhs,
                        const quantity::non_integer_type& rhs) const
{
  return 1. / (rhs / lhs);
}

#undef BINARY_OPERATOR_

#define BINARY_COMPARISON_(OPERATOR_NAME_,OPERATOR_SYMBOL_)     \
  struct OPERATOR_NAME_                                         \
    : public boost::static_visitor< bool >                      \
  {                                                             \
    template< typename T1, typename T2 >                        \
    bool operator() (T1& lhs, const T2& rhs) const              \
    {                                                           \
      return lhs OPERATOR_SYMBOL_ rhs;                          \
    }                                                           \
  };                                                            \
  /**/

BINARY_COMPARISON_(is_less_than_, <);

#undef BINARY_COMPARISON_

quantity::quantity (const integer_type& amount)
  : amount_(amount)
{}

quantity::quantity (const non_integer_type& amount)
  : amount_(amount)
{}

quantity::quantity ()
  : amount_(0)
{}

bool
quantity::is_integral () const
{
  const quantity q (integer_type (0));

  return (amount_.which () == q.amount_.which ());
}

bool
quantity::operator== (const quantity& q) const
{
  return amount_ == q.amount_;
}

bool
quantity::operator<  (const quantity& q) const
{
  // The boost::variant::operator< () member only supports a "proper"
  // notion of LessThanComparable when *both* instances have the same
  // bounded type.  When their bounded types differ, it compares the
  // indices of these types into the set of bounded types.  As a very
  // counter-intuitive result, it would report that an integer_type of
  // 1200 is *less* than a non_integer_type of 100.  Go figure!

  return boost::apply_visitor (is_less_than_(), amount_, q.amount_);
}

quantity&
quantity::operator+= (const quantity& q)
{
  boost::apply_visitor (increment_by_(), amount_, q.amount_);
  return *this;
}

quantity&
quantity::operator-= (const quantity& q)
{
  boost::apply_visitor (decrement_by_(), amount_, q.amount_);
  return *this;
}

quantity&
quantity::operator*= (const quantity& q)
{
  return *this = boost::apply_visitor (multiply_by_(), amount_, q.amount_);
}

quantity&
quantity::operator/= (const quantity& q)
{
  return *this = boost::apply_visitor (divide_by_(), amount_, q.amount_);
}

// FIXME I18N: decimal-point output
std::ostream&
operator<< (std::ostream& os, const quantity& q)
{
  if (q.is_integral ())
    {
      os << q.amount_;
    }
  else
    {
      std::stringstream ss;
      ss << q.amount_;
      if (std::string::npos == ss.str ().find ('.'))
        ss << ".0";
      os << ss.str ();
    }
  return os;
}

// FIXME I18N: isdigit, sign and decimal-point comparison
std::istream&
operator>> (std::istream& is, quantity& q)
{
  std::string s;
  is >> s;                      // get everything until next space

  quantity::integer_type integral_part (0);
  std::string::size_type offset (0);

  if ('+' == s[0] || '-' == s[0])
    ++offset;

  if (!(std::isdigit (s[offset], std::locale::classic ())
        || '.' == s[offset]))
    BOOST_THROW_EXCEPTION (boost::bad_lexical_cast ());

  std::stringstream ss (s.substr (offset));

  if (std::isdigit (s[offset], std::locale::classic ()))
      ss >> integral_part;

  if ('.' == ss.peek ())
    {
      quantity::non_integer_type decimal_part;

      ss >> decimal_part;
      q = decimal_part;
      q += integral_part;
    }
  else
    {
      q = integral_part;
    }

  if ('-' == s[0])
    q *= -1;

  return is;
}

quantity
operator+ (const quantity& q)
{
  return q;
}

quantity
operator- (const quantity& q)
{
  quantity rv (q);
  return rv *= -1;
}

quantity
abs (const quantity& q)
{
  return (q < quantity () ? -q : q);
}

}       // namespace utsushi
