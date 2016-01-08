//  descriptor.hpp -- objects for options and option groups
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

#ifndef utsushi_descriptor_hpp_
#define utsushi_descriptor_hpp_

#include <set>

#include "key.hpp"
#include "memory.hpp"
#include "string.hpp"
#include "tag.hpp"

namespace utsushi {

struct level {

  struct symbol
    : tag::symbol
  {
    symbol (const key& key,
            const string& name, const string& text);
  };

  static const symbol standard;
  static const symbol extended;
  static const symbol complete;
};

//! Meta-information for options and option groups
/*! In order to present the user with an intelligible interface, just
 *  knowing the key of an option or group is not enough.  Knowing the
 *  value and constraint types for an option, while required, doesn't
 *  quite cut it either.  A user interface builder needs more.  This
 *  additional information is kept in a descriptor.
 *
 *  A name() and explanatory text() are available to provide the user
 *  with a more informative view of the supported options and usable
 *  groups.  A set of tags() can be used to direct the UI towards the
 *  aspects that are relevant to the user's task at hand.  A query for
 *  what level an option is_at() is available so UI builders can curb
 *  the number of options that they display.
 *
 *  \todo  Add support for a UI element/widget hinting attribute
 *  \todo  Add support for icons
 */
class descriptor
{
public:
  typedef shared_ptr<descriptor> ptr;

  //! Creates a descriptor with given \a name and explanatory \a text
  /*! The descriptor is created without tags() and at a level that is
   *  meant to keep options out of sight.  Most users get overwhelmed
   *  at large numbers of options.  The operator()() overloads can be
   *  used to add tags and customize the level where deemed necessary.
   */
  descriptor (const string& name = string (),
              const string& text = string ());

  //! Provides a short, yet descriptive name
  /*! User interfaces may use this to put text next to check boxes and
   *  radio buttons or labels on buttons and tabs.
   *
   *  \note  The UI is responsible for translation of the name().
   */
  string name () const;

  //! Provides a more in-depth textual explanation
  /*! Complementing name(), text() returns a more detailed description
   *  of the purpose of an option or group.  A user interface may use
   *  this to provide the user with on-line help or display a tooltip
   *  when hovering over the UI representation of an option or group.
   *
   *  \note  The UI is responsible for translation and formatting of
   *         the text().
   *
   *  \todo  Specify the formatting commands allowed/supported
   */
  string text () const;

  //! Returns a container with tag keys
  /*! User interfaces may use this to decide whether or not an option
   *  or group should be made accessible to the user.
   */
  std::set< key > tags () const;

  bool is_at (const level::symbol& level) const;
  bool is_active () const;
  bool is_emulated () const;
  bool is_read_only () const;

  //! Sets a short, yet descriptive name
  void name (const string& name);

  //! Sets a textual explanation
  void text (const string& text);

  //! Adds a tag key to the set of tags()
  descriptor& operator() (const tag::symbol& tag);
  descriptor& operator() (const level::symbol& level);

  descriptor& active (bool toggle);
  descriptor& emulate (bool toggle);
  descriptor& read_only (bool toggle);

private:
  descriptor& toggle_(bool toggle, int flags);

  string name_;                 // translatable
  string text_;                 // translatable

  std::set< key > tags_;
  level::symbol level_;

  int flags_;
};

typedef descriptor aggregator;

aggregator attributes ();
aggregator attributes (const tag::symbol& tag);
aggregator attributes (const level::symbol& level);

}       // namespace utsushi

#endif  /* utsushi_descriptor_hpp_ */
