//  store.hpp -- collections of allowed values
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

#ifndef utsushi_store_hpp_
#define utsushi_store_hpp_

#include <list>

#include <boost/concept_check.hpp>

#include "constraint.hpp"

namespace utsushi {

//! Allow only values from an iterable collection
/*! Many settings allow a choice from a limited number of selected
 *  values.  Examples include image file output format (PNG, JPEG,
 *  PDF, etc.) and the type of input image (photo, text, line art,
 *  etc.).  This %constraint type allows for easy specification of
 *  such limitations on allowed values.
 *
 *  \note
 *  A user interface is at liberty to represent settings with %store
 *  type constraints with UI elements such as pull down menus and/or
 *  list-like input fields.  For small stores, i.e. stores with only
 *  two to four values, a radio button approach may be a reasonable
 *  alternative.
 *
 *  \note  I18N issues such as translation as well as ordering of a
 *         store's values are the user interface's responsibility.
 */
class store : public constraint
{
  typedef std::list< value > container_type;

public:
  typedef shared_ptr< store > ptr;
  typedef container_type::size_type size_type;
  typedef container_type::const_iterator const_iterator;

  virtual ~store ();
  virtual const value& operator() (const value& v) const;

  //! \copybrief constraint::default_value
  /*! The %value \a v will be added as an alternative if necessary.
   */
  virtual constraint * default_value (const value& v);

  virtual bool is_singular () const;

  virtual void operator>> (std::ostream& os) const;

  //! Adds values from the range \c [first,last) to the %store
  template <typename InputIterator>
  store * alternatives (InputIterator first, InputIterator last);
  //! Adds a value \a v to the %store
  store * alternative (const value& v);

  //! replace values from the range \c [first,last) to the %store
  template <typename InputIterator>
  store * assign (InputIterator first, InputIterator last);

  size_type size () const;

  //! Returns an iterator to the first %value in the %store
  const_iterator begin () const;
  //! Returns an iterator to one past the last %value in the %store
  const_iterator end () const;

  //! Returns the first %value in %store
  /*! \note  Behavior is undefined for empty stores
   */
  const value& front () const;
  //! Returns the last %value in %store
  /*! \note  Behavior is undefined for empty stores
   */
  const value& back () const;

private:
  container_type store_;
};

template <typename InputIterator>
store *
store::alternatives (InputIterator first, InputIterator last)
{
  BOOST_CONCEPT_ASSERT ((boost::InputIterator< InputIterator >));

  for (InputIterator it = first; it != last; ++it)
    {
      alternative (*it);
    }
  return this;
}

template <typename InputIterator>
store *
store::assign (InputIterator first, InputIterator last)
{
  BOOST_CONCEPT_ASSERT ((boost::InputIterator< InputIterator >));

  store_.assign (first, last);
  return this;
}

}       // namespace utsushi

#endif  /* utsushi_store_hpp_ */
