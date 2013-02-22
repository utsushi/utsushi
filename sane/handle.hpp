//  handle.hpp -- for a SANE scanner object
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#ifndef sane_handle_hpp_
#define sane_handle_hpp_

extern "C" {                    // needed until sane-backends-1.0.14
#include <sane/sane.h>
}

#include <csignal>
#include <string>
#include <vector>

#include <utsushi/scanner.hpp>
#include <utsushi/stream.hpp>

namespace sane {

//! Implements a SANE scanner object
/*! \remark  The implementation assumes that the SANE API entries
 *           handle argument screening and \e never pass invalid
 *           arguments to the public handle API.
 */
class handle
{
public:
  handle (const utsushi::scanner::id& id);
  ~handle ();

  std::string name () const;

  //! Returns the number of options
  SANE_Int size () const;

  //! Grabs a hold of the SANE option descriptor at \a index
  const SANE_Option_Descriptor * descriptor (SANE_Int index) const;

  bool is_active (SANE_Int index) const;
  bool is_button (SANE_Int index) const;
  bool is_group (SANE_Int index) const;
  bool is_settable (SANE_Int index) const;
  bool is_automatic (SANE_Int index) const;

  bool is_scanning () const;

  //! Handles \c SANE_ACTION_GET_VALUE option control requests
  SANE_Status get (SANE_Int index, void *value) const;
  //! Handles \c SANE_ACTION_SET_VALUE option control requests
  SANE_Status set (SANE_Int index, void *value, SANE_Word *info);
  //! Handles \c SANE_ACTION_SET_AUTO option control requests
  SANE_Status set (SANE_Int index, SANE_Word *info);

  utsushi::context get_context () const;

  utsushi::streamsize start ();
  utsushi::streamsize read (utsushi::octet *buffer,
                            utsushi::streamsize length);
  void cancel ();

protected:
  utsushi::istream::ptr is_;
  utsushi::scanner::ptr ptr_;
  std::string name_;

public:
  utsushi::streamsize last_marker_;

  sig_atomic_t is_cancelling_;
  sig_atomic_t is_terminating_;

private:
  void add_option (utsushi::option& visitor);

  utsushi::option::map opt_;

  //! Add a key dictionary to SANE_Option_Descriptor objects
  struct option_descriptor
    : SANE_Option_Descriptor
  {
    utsushi::key orig_key;
    std::string  sane_key;
    utsushi::string name_;
    utsushi::string desc_;

    option_descriptor () {}
    option_descriptor (const utsushi::option& visitor);
  };

  void add_group (const utsushi::key& key,
                  const utsushi::string& name,
                  const utsushi::string& text = utsushi::string ());

  std::vector< option_descriptor > sod_;
};

}       // namespace sane

#endif  /* sane_handle_hpp_ */
