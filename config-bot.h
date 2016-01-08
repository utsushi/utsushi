/*  config-bot.h -- config.h dependent boiler plate
 *  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
 *
 *  License: GPL-3.0+
 *  Author : EPSON AVASYS CORPORATION
 *
 *  This file is part of the 'Utsushi' package.
 *  This package is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License or, at
 *  your option, any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *  You ought to have received a copy of the GNU General Public License
 *  along with this package.  If not, see <http: *www.gnu.org/licenses/>.
 */

/*  The __attribute__ keyword originated as a GNU extension to GCC and
 *  may not be available with other compilers.  We have `./configure`
 *  check for basic support for the keyword but the test only defines
 *  a `cpp` symbol for the programmer to use.
 *  Here we make the keyword disappear for those compilers that don't
 *  support it.
 */
#ifndef HAVE___ATTRIBUTE__
#define __attribute__(attr)
#endif

#define HAVE_MAGICK     (HAVE_GRAPHICS_MAGICK    || HAVE_IMAGE_MAGICK)
#define HAVE_MAGICK_PP  (HAVE_GRAPHICS_MAGICK_PP || HAVE_IMAGE_MAGICK_PP)

#if __cplusplus < 201103L
#define nullptr 0
#endif
