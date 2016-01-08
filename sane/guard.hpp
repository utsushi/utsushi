//  guard.hpp -- clauses for common argument checks
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2007  EPSON AVASYS CORPORATION
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

#ifndef sane_guard_hpp_
#define sane_guard_hpp_

/*! \file
 *  \brief  Provides handy guard clauses for argument checking.
 *
 *  Argument checking is often not performed because it has a tendency
 *  to lead to bulky code at function entry.  This file provides a set
 *  of preprocessor macros that try to make that code less bulky.
 */

extern "C" {                    // needed until sane-backends-1.0.14
#include <sane/sane.h>
}

#include "log.hpp"

//! SANE_Status used to indicate failure
/*! \rationale
 *  The SANE API does not define a generic failure status so we have
 *  to make do with the next best SANE_Status.  Based on the current
 *  definitions, that probably means one of:
 *
 *   - \c SANE_STATUS_UNSUPPORTED
 *   - \c SANE_STATUS_INVAL
 *   - \c SANE_STATUS_ACCESS_DENIED
 *
 *  \c SANE_STATUS_INVAL is already used for other purposes, see
 *  ::invalid_status_, and \c SANE_STATUS_ACCESS_DENIED hints at a
 *  permission problem with the user's configuration more than at a
 *  problem with the SANE frontend using this backend.
 *
 *  There is also the option of using a value that is \e not defined
 *  in the SANE API, \c ~SANE_STATUS_GOOD for example, but there is no
 *  guarantee that all SANE frontends can cope with such an undefined
 *  status value.
 */
#define failure_status_ SANE_STATUS_UNSUPPORTED

//! SANE_Status used to signal passing of invalid arguments
#define invalid_status_ SANE_STATUS_INVAL

//! Guard macro that returns from the calling scope
/*! The macro evaluates an \a condition and, if it evaluates to \c
 *  true, returns the given \a value from the calling scope.
 */
#define return_value_if(condition,value)                \
        if (condition)                                  \
          {                                             \
            return value;                               \
          }                                             \
        /**/

//! Negating version of ::return_value_if
#define return_value_unless(condition,value)            \
        return_value_if (!(condition),value)            \
        /**/

//! Convenience guard macro that just returns
/*! Like ::return_value_if, this macro evaluates an \a condition but
 *  returns from the calling scope without value.
 *
 *  \rationale
 *  This macro makes for less awkward source code because it removes
 *  the unusual trailing comma in the argument list from view.
 */
#define return_if(condition)                            \
        return_value_if (condition,)                    \
        /**/

//! Negating version of ::return_if
#define return_unless(condition)                        \
        return_if (!(condition))                        \
        /**/

//! Convenience guard clause returning a failure indication
#define return_failure_if(condition)                    \
        return_value_if (condition, failure_status_)    \
        /**/

//! Negating version of ::return_failure_if
#define return_failure_unless(condition)                \
        return_failure_if (!(condition))                \
        /**/

//! Convenience guard clause returning an invalid indication
#define return_invalid_if(condition)                    \
        return_value_if (condition, invalid_status_)    \
        /**/

//! Negating version of ::return_invalid_if
#define return_invalid_unless(condition)                \
        return_invalid_if (!(condition))                \
        /**/

//! Verbose variant of ::return_value_if
/*! The macro evaluates a \a condition and, if it evaluates to \c
 *  true, sends a formatted error \a message to the logger before
 *  it returns the given \a value from the calling scope.
 */
#define return_value_verbosely_if(condition,message,value)              \
        if (condition)                                                  \
          {                                                             \
            log::error ("%1%: %2%") % fn_name % message;                \
            return value;                                               \
          }                                                             \
        /**/

//! Verbose variant of ::return_value_unless
#define return_value_verbosely_unless(condition,message,value)          \
        return_value_verbosely_if (!(condition),message,value)          \
        /**/

//! Verbose variant of ::return_if
#define return_verbosely_if(condition,message)                          \
        return_value_verbosely_if(condition,message,)                   \
        /**/

//! Verbose variant of ::return_unless
#define return_verbosely_unless(condition,message)                      \
        return_verbosely_if (!(condition),message)                      \
        /**/

//! Verbose variant of ::return_failure_if
#define return_failure_verbosely_if(condition,message)                  \
        return_value_verbosely_if (condition, message, failure_status_) \
        /**/

//! Verbose variant of ::return_failure_unless
#define return_failure_verbosely_unless(condition,message)              \
        return_failure_verbosely_if (!(condition), message)             \
        /**/

//! Verbose variant of ::return_invalid_if
#define return_invalid_verbosely_if(condition,message)                  \
        return_value_verbosely_if (condition, message, invalid_status_) \
        /**/

//! Verbose variant of ::return_invalid_unless
#define return_invalid_verbosely_unless(condition,message)              \
        return_invalid_verbosely_if (!(condition), message)             \
        /**/

#endif  /* sane_guard_hpp_ */
