//  quantity.hpp -- bounded type for utsushi::value objects
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

#ifndef utsushi_quantity_hpp_
#define utsushi_quantity_hpp_

#include <iosfwd>

#include <boost/mpl/list.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/operators.hpp>
#include <boost/static_assert.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

namespace utsushi {

class quantity
  : boost::totally_ordered< quantity
  , boost::arithmetic     < quantity
  > >
{
public:
  typedef int32_t integer_type;
  typedef double  non_integer_type;

  quantity (const integer_type& amount);
  quantity (const non_integer_type& amount);

  explicit quantity ();

  bool is_integral () const;

  bool operator== (const quantity& q) const;
  bool operator<  (const quantity& q) const;

  quantity& operator+= (const quantity& q);
  quantity& operator-= (const quantity& q);
  quantity& operator*= (const quantity& q);
  quantity& operator/= (const quantity& q);

  template< typename T > T amount () const;

  friend
  std::ostream& operator<< (std::ostream& os, const quantity& q);
  friend
  std::istream& operator>> (std::istream& is, quantity& q);

private:
  typedef boost::mpl::list< integer_type, non_integer_type > bounded;
  typedef boost::make_variant_over< bounded::type >::type value_type;

  value_type amount_;

public:
  typedef bounded::type types;
};

quantity operator+ (const quantity& q);
quantity operator- (const quantity& q);

quantity abs (const quantity& q);

template< typename T >
T quantity::amount () const
{
  BOOST_STATIC_ASSERT ((boost::is_arithmetic< T >::value));

  return boost::numeric_cast< T >
    (is_integral ()
     ? boost::get< integer_type > (amount_)
     : boost::get< non_integer_type > (amount_));
}

}       // namespace utsushi

#endif  /* utsushi_quantity_hpp_ */
