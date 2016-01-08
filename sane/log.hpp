//  log.hpp -- feedback in a concise manner
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

#ifndef sane_log_hpp_
#define sane_log_hpp_

/*! \file
 *  \brief  Log backend feedback in a concise manner
 *
 *  To cut back a bit on backend code verbosity, this file provides a
 *  complete set of convenience inlines that interface with Utsushi's
 *  logging mechanism.  All feedback is logged in the \c SANE_BACKEND
 *  category.
 */

#include <utsushi/log.hpp>

namespace sane {

class log
{
public:

#define expand_convenience_ctors(ctor, type, fmt_type)                  \
  inline static type                                                    \
  ctor (const type::fmt_type fmt)                                       \
  {                                                                     \
    return utsushi::log::ctor (utsushi::log::SANE_BACKEND, fmt);        \
  }                                                                     \
  /**/

#define expand_named_format_ctors(ctor,type)            \
  expand_convenience_ctors (ctor, type, format_type)    \
  expand_convenience_ctors (ctor, type, string_type)    \
  expand_convenience_ctors (ctor, type, char_type *)    \
  /**/

#define expand_named_ctors(ctor)                                \
  expand_named_format_ctors (ctor, utsushi::log::message)       \
  expand_named_format_ctors (ctor, utsushi::log::wmessage)      \
  /**/

  expand_named_ctors (fatal);
  expand_named_ctors (alert);
  expand_named_ctors (error);
  expand_named_ctors (brief);
  expand_named_ctors (trace);
  expand_named_ctors (debug);

#undef expand_named_ctors
#undef expand_named_format_ctors
#undef expand_convenience_ctors

};

}       // namespace sane

#endif  /* sane_log_hpp_ */
