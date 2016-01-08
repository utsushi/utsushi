//  version.hpp -- information for the SANE utsushi backend
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

#ifndef sane_version_hpp_
#define sane_version_hpp_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*! \file
 *  \brief  Backend version information and checking
 *
 *  Not only does this file define the various bits of backend version
 *  information, it also contains code that performs sanity checking
 *  on these definitions.
 */

extern "C" {                    // needed until sane-backends-1.0.14
#include <sane/sane.h>
}

#if !((SANE_MAJOR == SANE_CURRENT_MAJOR) && (SANE_MINOR == 0))
#error "SANE installation violates versioning portability constraints."
#endif

//! Version of the SANE C API that the backend implements
/*! \hideinitializer
 */
#define BACKEND_MAJOR  1

#if (BACKEND_MAJOR != SANE_CURRENT_MAJOR)
#error "Backend assumptions do not match current SANE C API version."
#endif

#if (0 > BACKEND_MAJOR || 255 < BACKEND_MAJOR)
#error "BACKEND_MAJOR value out of range."
#endif

//! Version of the backend implementation
/*! This number is typically increased with every external release of
 *  the backend when there have been changes to the implementation.
 *
 *  \hideinitializer
 */
#define BACKEND_MINOR  1

#if (0 > BACKEND_MINOR || 255 < BACKEND_MINOR)
#error "BACKEND_MINOR value out of range."
#endif

//! Build revision of the backend implementation
/*! This number can be used to track the version of internal releases
 *  or to differentiate between different builds of the backend.
 *
 *  It takes a default value of zero and should probably be reset to
 *  that value when the ::BACKEND_MINOR value is increased.
 *
 *  \hideinitializer
 */
#define BACKEND_BUILD  0

#if (0 > BACKEND_BUILD || 65535 < BACKEND_BUILD)
#error "BACKEND_BUILD value out of range."
#endif

//! Indicates where the backend originates from
/*! This information includes both the name and the version of the
 *  originating software package.
 *
 *  \hideinitializer
 */
#define BACKEND_SOURCE PACKAGE_STRING

#ifndef PACKAGE_STRING
#error "PACKAGE_STRING is not defined at this point."
#endif

#endif  /* sane_version_hpp_ */
