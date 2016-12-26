dnl  configure.ac -- an autoconf template for configure
dnl  Copyright (C) 2012-2016  SEIKO EPSON CORPORATION
dnl  Copyright (C) 2013-2015  Olaf Meeuwissen
dnl
dnl  License: GPL-3.0+
dnl  Author : EPSON AVASYS CORPORATION
dnl
dnl  This file is part of the 'Utsushi' package.
dnl  This package is free software: you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation, either version 3 of the License or, at
dnl  your option, any later version.
dnl
dnl    This program is distributed in the hope that it will be useful,
dnl    but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl    GNU General Public License for more details.
dnl
dnl  You ought to have received a copy of the GNU General Public License
dnl  along with this package.  If not, see <http://www.gnu.org/licenses/>.

dnl  Process this file with autoconf to make a configure file.

dnl  preamble

AC_PREREQ(2.62)
AC_INIT([Utsushi], [0.24.0],
        [linux-scanner@epson.jp],
        [utsushi])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AH_BOTTOM([#include <config-bot.h>])

AC_CONFIG_AUX_DIR([upstream])
AC_CONFIG_MACRO_DIR([upstream/aclocal])

dnl  extend the set of Automake pkg*dir variables
dnl  This is meant to expand to the same form as all the other pkg*dir
dnl  variables in the Makefiles.

pkgsysconfdir="\$(sysconfdir)/$PACKAGE_TARNAME"
AC_SUBST([pkgsysconfdir])

dnl  additional command-line options and dependency handling

AC_ARG_ENABLE(test-reports,
  AS_HELP_STRING([--enable-test-reports],
    [generate XML test reports @<:@no@:>@.
     Implies --with-boost-unit-test-framework.
     XML test reports can be post-processed with tools/boost-test-report.]),
  [],
  [enable_test_reports=no])
AS_IF([test xno != "x$enable_test_reports"],
  [AS_IF([test xyes != x$with_boost_unit_test_framework],
     [AC_MSG_NOTICE([--enable-test-reports activates --with-boost-unit-test-framework])
      with_boost_unit_test_framework=yes
      boost_unit_test_framework_implied=yes
     ])
  ])
AM_CONDITIONAL([enable_test_reports], [test xyes = x$enable_test_reports])

AC_ARG_ENABLE(code-coverage,
  AS_HELP_STRING([--enable-code-coverage],
    [collect run-time code coverage statistics @<:@no@:>@.
     Implies --with-boost-unit-test-framework and --disable-shared.
     This requires a GNU C/C++ compiler and turns off optimization for
     best results.]),
  [],
  [enable_code_coverage=no])
AS_IF([test xno != "x$enable_code_coverage"],
  [AS_IF([test xyes != x$with_boost_unit_test_framework],
     [AC_MSG_NOTICE([--enable-code-coverage activates --with-boost-unit-test-framework])
      with_boost_unit_test_framework=yes
      boost_unit_test_framework_implied=yes
     ])
   AS_IF([test xno != "x$enable_shared"],
     [AC_MSG_NOTICE([--enable-code-coverage activates --disable-shared])
      enable_shared=no
     ])
  ])
AM_CONDITIONAL([enable_code_coverage], [test xyes = x$enable_code_coverage])

AC_ARG_WITH(gtkmm,
  AS_HELP_STRING([--with-gtkmm],
    [build a gtkmm based graphical user interface @<:@check@:>@]),
  [],
  [with_gtkmm=check])

AC_ARG_WITH(jpeg,
  AS_HELP_STRING([--with-jpeg],
    [include JPEG image format related filters @<:@check@:>@]),
  [],
  [with_jpeg=check])

AC_ARG_WITH(magick,
  AS_HELP_STRING([--with-magick],
    [include image processing with convert @<:@check@:>@.
     Both GraphicsMagick and ImageMagick are supported but the latter provides
     somewhat more functionality.]),
  [],
  [with_magick=check])
AC_ARG_WITH(magick-pp,
  AS_HELP_STRING([--with-magick-pp],
    [include autocrop and deskew support @<:@check@:>@.
     Both the GraphicsMagick and ImageMagick C++ libraries are supported.]),
  [],
  [with_magick_pp=check])

AC_ARG_WITH(tiff,
  AS_HELP_STRING([--with-tiff],
    [include TIFF image format related support @<:@check@:>@]),
  [],
  [with_tiff=check])

AC_ARG_WITH(sane,
  AS_HELP_STRING([--with-sane],
    [build SANE compatibility components @<:@check@:>@]),
  [],
  [with_sane=check])
AC_ARG_ENABLE(sane-config,
  AS_HELP_STRING([--enable-sane-config],
    [activate SANE compatibility component @<:@yes@:>@.
     Disable this if you do not want the install step to touch your
     system's SANE configuration.  This is useful if you do not have
     the privileges needed to make these changes.
     For example when you configure with --prefix=$HOME.]),
  [],
  [enable_sane_config=yes])
AC_ARG_WITH(sane-confdir,
  AS_HELP_STRING([--with-sane-confdir@<:@=DIR@:>@],
    [where to find the SANE configuration directory @<:@guess@:>@.
     Only use this if the guess is incorrect and you know exactly
     where your SANE configuration directory is.]),
  [],
  [with_sane_confdir=guess])

AC_ARG_WITH(included-boost,
  AS_HELP_STRING([--with-included-boost],
     [use the Boost sources included here, if any @<:@check@:>@.
      Directory locations for other --with-boost-* options will be
      overridden.]),
  [],
  [with_included_boost=check])

AC_ARG_ENABLE(udev-config,
  AS_HELP_STRING([--enable-udev-config],
    [integrate udev activated device set up @<:@yes@:>@.
     Disable this if you do not want the install step to touch your
     system's udev configuration.  This is useful if you do not have
     the privileges needed to make these changes.
     For example when you configure with --prefix=$HOME.]),
  [],
  [enable_udev_config=yes])
AC_ARG_WITH(udev-confdir,
  AS_HELP_STRING([--with-udev-confdir@<:@=DIR@:>@],
    [udev configuration directory @<:@guess@:>@.
     Only use this if the guess is incorrect and you know exactly
     where your udev configuration directory is.]),
  [],
  [with_udev_confdir=guess])
AC_ARG_WITH(download-dir,
  AS_HELP_STRING([--with-download-dir@<:@=ABS_DIR@:>@],
    [where to cache upstream source downloads @<:@/path/to/upstream/downloads@:>@.
     Distributable creation may include upstream sources in a way that depends
     on our own distributed sources.  For that to work as intended, it attempts
     to download upstream archives into this location.]))

dnl  preprocessor symbols for optional/conditional code

AC_DEFINE([HAVE_LIBGTKMM],
  [0], [Define to 1 if the gtkmm library is available])
AC_DEFINE([HAVE_LIBJPEG],
  [0], [Define to 1 if a JPEG library is available])
AC_DEFINE([HAVE_LIBMAGIC],
  [0], [Define to 1 if libmagic is available])
AC_DEFINE([HAVE_LIBSANE],
  [0], [Define to 1 if the SANE library is available])
AC_DEFINE([HAVE_LIBTIFF],
  [0], [Define to 1 if a TIFF library is available])
AC_DEFINE([HAVE_LIBUDEV],
  [0], [Define to 1 if the udev library is available])
AC_DEFINE([HAVE_LIBUSB],
  [0], [Define to 1 if libusb-1.0 is available])
AC_DEFINE([HAVE_LIBUSB_ERROR_NAME],
  [0], [Define to 1 if libusb_error_name() is available])
AC_DEFINE([HAVE_LIBUSB_GET_PORT_NUMBER],
  [0], [Define to 1 if libusb_get_port_number() is available])
AC_DEFINE([HAVE_GRAPHICS_MAGICK],
  [0], [Define to 1 if GraphicsMagick is available])
AC_DEFINE([HAVE_GRAPHICS_MAGICK_PP],
  [0], [Define to 1 if GraphicsMagick's C++ API is usable])
AC_DEFINE([HAVE_IMAGE_MAGICK],
  [0], [Define to 1 if ImageMagick is available])
AC_DEFINE([HAVE_IMAGE_MAGICK_PP],
  [0], [Define to 1 if ImageMagick's C++ API is usable])
AC_DEFINE([MAGICK_CONVERT],
  [], [Define to an appropriate command-line invocation for convert])
AC_DEFINE([WITH_INCLUDED_BOOST],
  [0], [Define to 1 if using the included Boost sources])

dnl  checks for programs, modules and libraries

AM_INIT_AUTOMAKE([1.11.2 gnu tar-ustar -Wall -Werror std-options nostdinc subdir-objects])
AM_MAINTAINER_MODE([enable])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_OPENMP([C])
AC_PROG_CXX
AC_LANG_PUSH([C++])
AC_MSG_CHECKING([standard compliance level of $CXX])
AC_COMPUTE_INT([__cplusplus], [__cplusplus])
AC_MSG_RESULT([${__cplusplus}L])
AC_LANG_POP([C++])
AS_IF([test xno != x$enable_code_coverage],
  [AS_IF([test xyes = x$ac_cv_cxx_compiler_gnu],
     [code_coverage_cxxflags="-fprofile-arcs -ftest-coverage"
      CXXFLAGS="$CXXFLAGS -O0"
     ],
     [AC_MSG_ERROR([code coverage support requires a GNU C/C++ compiler])])
  ])
AM_PROG_AR

PKG_PROG_PKG_CONFIG

AM_GNU_GETTEXT([external])      dnl  i18n support
AM_GNU_GETTEXT_VERSION([0.19.3])
AM_XGETTEXT_OPTION([--boost])
AC_DEFINE_UNQUOTED([DEFAULT_TEXT_DOMAIN], ["$PACKAGE"],
  [Define to the default text domain for message translation.])

LT_PREREQ([2.2.6])              dnl  portable plugin (build) support
LT_INIT([dlopen disable-static])
LT_LANG([C++])
LT_CONFIG_LTDL_DIR([upstream/ltdl])
LTDL_INIT([recursive])
AM_CONDITIONAL([WITH_INCLUDED_LTDL], [test xyes = "x$with_included_ltdl"])
                                dnl  extra exports for use in Makefile.am
AC_SUBST([LT_MODULE_EXT], [$libltdl_cv_shlibext])
AC_SUBST([LT_MODULE_PATH_VAR], [$lt_cv_module_path_var])
AM_CONDITIONAL([with_shared_enabled], [test xyes = x$enable_shared])
AM_CONDITIONAL([with_static_enabled], [test xyes = x$enable_static])

                                dnl  cross-platform C++ extensions
AS_IF([test xyes != x$with_included_boost],
  [AX_BOOST_BASE([1.49.0],
     [AX_BOOST_SYSTEM
      AX_BOOST_FILESYSTEM
      AX_BOOST_IOSTREAMS
      AX_BOOST_PROGRAM_OPTIONS
      AS_IF([test $__cplusplus -lt 201103], [AX_BOOST_REGEX])
      AS_IF([test $__cplusplus -lt 201103], [AX_BOOST_THREAD])
      AX_BOOST_UNIT_TEST_FRAMEWORK
     ],
     [AS_IF([test -d $ac_aux_dir/boost],
        [AS_IF([test xno != x$with_included_boost],
           [AC_MSG_NOTICE([using included Boost sources instead])
            with_included_boost=yes
           ],
           [AC_MSG_ERROR([use included sources or install Boost libraries])])
        ],
        [AC_MSG_ERROR([install Boost libraries])
        ])
     ])
  ])
AS_IF([test xyes = x$with_included_boost],
  [AS_IF([test -d $ac_aux_dir/boost],
     [ax_cv_boost_unit_test_framework=yes
      aux_dir=`echo $ac_aux_dir | sed "s,^$srcdir/,,"`
      BOOST_CPPFLAGS="-I\$(top_srcdir)/$aux_dir/boost"
      BOOST_LDFLAGS="-L\$(top_builddir)/$aux_dir/boost/lib"
      BOOST_SYSTEM_LIB=-lboost_system
      BOOST_FILESYSTEM_LIB=-lboost_filesystem
      BOOST_IOSTREAMS_LIB=-lboost_iostreams
      BOOST_PROGRAM_OPTIONS_LIB=-lboost_program_options
      BOOST_REGEX_LIB=-lboost_regex
      BOOST_THREAD_LIB=-lboost_thread
      BOOST_UNIT_TEST_FRAMEWORK_LIB=-lboost_unit_test_framework

      #  Make sure we can build Boost.Iostreams

      AC_CHECK_LIB([bz2], [BZ2_bzReadOpen],
        [true],
        [AC_MSG_ERROR([included Boost sources require libbz2 to build])])
      AC_CHECK_LIB([z], [gzopen],
        [true],
        [AC_MSG_ERROR([included Boost sources require zlib to build])])

      #  Only build with those feature sets that we need.  There is
      #  no point in building static libraries that we don't use.

      AS_IF([test xyes = x$enable_shared],
        [boost_link_props="$boost_link_props link=shared"])
      AS_IF([test xyes = x$enable_static],
        [boost_link_props="$boost_link_props link=static"])
      AC_SUBST(boost_build_link_properties,[$boost_link_props])

      #  Making the Boost header files system headers allows us to
      #  treat all warnings in our code as errors.  The '-isystem'
      #  option is GCC C/C++ compiler specific.

      AS_IF([test xyes = x$ac_cv_cxx_compiler_gnu],
        [BOOST_CPPFLAGS="-isystem \$(top_srcdir)/$aux_dir/boost"])

      AC_DEFINE([WITH_INCLUDED_BOOST], [1])
     ],
     [AC_MSG_ERROR([install Boost libraries])
     ])
  ])

AM_CONDITIONAL([with_included_boost], [test xyes = x$with_included_boost])
AM_CONDITIONAL([enable_boost_unit_test_framework],
  [test xyes = x$ax_cv_boost_unit_test_framework])

AS_IF([test xyes = x$with_boost_unit_test_framework],
  [AS_IF([test xyes != x$ax_cv_boost_unit_test_framework],
     [AS_IF([test xyes != x$boost_unit_test_framework_implied],
        [AC_MSG_ERROR([Boost Unit Test Framework requested but not found])],
        [AC_MSG_ERROR([Boost Unit Test Framework activated but not found])])
     ])
  ])

#  Work around implicit dependencies on Boost.System through the use
#  of Boost.Filesystem in several locations.

BOOST_FILESYSTEM_LIB="$BOOST_FILESYSTEM_LIB $BOOST_SYSTEM_LIB"

                                dnl  documentation generation tools
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AC_PATH_PROG([PERL], [perl])
AC_PATH_PROG([DOT_PATH], [dot])
AS_IF([test x = x$DOT_PATH],
  [HAVE_DOT=NO],
  [HAVE_DOT=YES
   DOT_PATH=`AS_DIRNAME(["$DOT_PATH"])`
  ])
AC_SUBST([HAVE_DOT])
AC_PATH_PROG([PDFLATEX], [pdflatex])
AM_CONDITIONAL([have_pdflatex], [test x != x$PDFLATEX])

                                dnl  modules for optional functionality
PKG_CHECK_MODULES([LIBUDEV], [libudev],
  [AC_DEFINE([HAVE_LIBUDEV], [1])],
  [false])
AM_CONDITIONAL([have_libudev], [test x != "x$LIBUDEV_LIBS"])
AS_IF([test xno != "x$with_usb"],
  [PKG_CHECK_MODULES([LIBUSB], [libusb-1.0],
    [AC_DEFINE([HAVE_LIBUSB], [1])],
    [true])
  ])
AM_CONDITIONAL([have_libusb], [test x != "x$LIBUSB_LIBS"])

AS_IF([test xno != "x$with_magick_pp"],
   AS_CASE("x$with_magick_pp",
     [xGraphicsMagick],
       [PKG_CHECK_MODULES([LIBMAGICK_PP], [GraphicsMagick++],
          [AC_DEFINE([HAVE_GRAPHICS_MAGICK_PP], [1])])
       ],
     [xImageMagick],
       [PKG_CHECK_MODULES([LIBMAGICK_PP], [Magick++ > 6.4.2],
          [AC_DEFINE([HAVE_IMAGE_MAGICK_PP], [1])])
       ],
     [xyes|xcheck],
       [PKG_CHECK_MODULES([LIBMAGICK_PP], [GraphicsMagick++],
          [AC_DEFINE([HAVE_GRAPHICS_MAGICK_PP], [1])],
          [PKG_CHECK_MODULES([LIBMAGICK_PP], [Magick++ > 6.4.2],
             [AC_DEFINE([HAVE_IMAGE_MAGICK_PP], [1])],
             [AS_IF([test xcheck != "x$with_magick_pp"],
                [AC_MSG_ERROR([magick-pp required but not found])])
             ])
          ])
       ],
     [dnl default case
      AC_MSG_ERROR([unknown value: --with-magick-pp=$with_magick_pp])
     ]))
AM_CONDITIONAL([have_libmagick_pp], [test x != "x$LIBMAGICK_PP_LIBS"])

AS_IF([test xno != "x$with_magick"],
   AS_CASE("x$with_magick",
     [xGraphicsMagick],
       [AC_CHECK_PROGS([MAGICK_CONVERT], [gm])
        AS_IF([test xgm != x$MAGICK_CONVERT],
          [AC_MSG_ERROR([$with_magick requested but not found])])
        AC_DEFINE([HAVE_GRAPHICS_MAGICK], [1])
        MAGICK_CONVERT="gm convert"
       ],
     [xImageMagick],
       [AC_CHECK_PROGS([MAGICK_CONVERT], [convert])
        AS_IF([test xconvert != x$MAGICK_CONVERT],
          [AC_MSG_ERROR([$with_magick requested but not found])])
        AC_DEFINE([HAVE_IMAGE_MAGICK], [1])
       ],
     [xyes|xcheck],
       [AC_CHECK_PROGS([MAGICK_CONVERT], [gm convert])
        AS_CASE(x$MAGICK_CONVERT,
          [xgm],
            [AC_DEFINE([HAVE_GRAPHICS_MAGICK], [1])
             MAGICK_CONVERT="gm convert"
            ],
          [xconvert],
            [AC_DEFINE([HAVE_IMAGE_MAGICK], [1])
            ],
          [dnl default case
           AS_IF([test xcheck != "x$with_magick"],
             [AC_MSG_ERROR([magick required but not found])])
          ])
       ],
     [dnl default case
      AC_MSG_ERROR([unknown value: --with-magick=$with_magick])
     ]))
AC_DEFINE_UNQUOTED([MAGICK_CONVERT], ["$MAGICK_CONVERT"])
AM_CONDITIONAL([have_magick], [test x != "x$MAGICK_CONVERT"])

AS_IF([test xno != "x$with_gtkmm"],
  [PKG_CHECK_MODULES([LIBGTKMM], [gtkmm-2.4 >= 2.20],
     [AC_DEFINE([HAVE_LIBGTKMM], [1])],
     [AS_IF([test xcheck != "x$with_gtkmm"],
        [AC_MSG_ERROR([gtkmm library required but not found])])
     ])
  ])
AM_CONDITIONAL([with_gtkmm], [test x != "x$LIBGTKMM_LIBS"])

                                dnl  SANE compatibility
AS_IF([test xno != "x$with_sane"],
  [AC_CHECK_HEADER([sane/sane.h],
    [AC_MSG_CHECKING([SANE version])
     sane_version="(unknown)"
     PKG_CHECK_EXISTS([sane-backends],          dnl  sane-backends >= 1.0.21
       [sane_version=`$PKG_CONFIG --modversion sane-backends | sed 's/git//g'`
        sane_backenddir=`$PKG_CONFIG --variable libdir sane-backends`/sane
       ],
       [dnl  sane-backends >= 1.0.2
        dnl  Guess that the backends are in a sane directory located on
        dnl  the same level as the library to which frontends link.  If
        dnl  such a directory does not exist use a sensible fallback.
        dnl  Note that there may be an architecture between lib and sane
        dnl  in the fallback but that is something we don't deal with.
        AS_IF([type sane-config >/dev/null 2>&1],
          [sane_version=`sane-config --version`
           sane_backenddir=`sane-config --ldflags | sed 's/^-L\(@<:@^ @:>@*\).*/\1/'`
           AS_IF([test -d "$sane_backenddir/sane"],
             [sane_backenddir="$sane_backenddir/sane"],
             [sane_backenddir=`sane-config --exec-prefix`/lib/sane
             ])
          ])
       ])
     AC_MSG_RESULT([$sane_version])
     AC_MSG_CHECKING([where SANE backends are installed])
     AC_MSG_RESULT([$sane_backenddir])
     AC_MSG_CHECKING([where SANE configuration is kept])
     AS_IF([test xguess == "x$with_sane_confdir"],
       [AS_CASE($sane_backenddir,
          [/usr/local/*], [sane_sysconfdir=/usr/local/etc/sane.d],
          [sane_sysconfdir=/etc/sane.d])
       ],
       [sane_sysconfdir=$with_sane_confdir])
     AC_MSG_RESULT([$sane_sysconfdir])
     AS_IF([test xno == x$cross_compiling],
       [dnl  When cross-compiling, you cannot access the target system's
        dnl  file system.  File existence checks are guaranteed to fail.
        AC_CHECK_FILE([$sane_sysconfdir/dll.conf],
          [],
          [sane_sysconfdir=
           enable_sane_config=no
           AC_MSG_WARN([disabling SANE component activation])
          ])
       ])
     AS_IF([test "x(unknown)" = "x$sane_version"],
       [sane_version=
        sane_backenddir=
        sane_sysconfdir=
        AS_IF([test xcheck != "x$with_sane"],
          [AC_MSG_ERROR([--with-sane was given but test for SANE failed])])
       ])
    ],
    [AS_IF([test xcheck != "x$with_sane"],
       [AC_MSG_ERROR([--with-sane was given but test for SANE failed])])
    ])])
AS_IF([test x != "x$sane_version"],
  [AC_DEFINE([HAVE_LIBSANE], [1])])
AM_CONDITIONAL([WITH_SANE], [test x != "x$sane_version"])
AM_CONDITIONAL([enable_sane_config], [test xyes = "x$enable_sane_config"])
AC_SUBST(SANE_MAJOR, `echo $sane_version | $AWK -F. '{print $1}'`)
AC_SUBST(SANE_MINOR, `echo $sane_version | $AWK -F. '{print $2}'`)
AC_SUBST(SANE_REVISION, `echo $sane_version | $AWK -F. '{print $3}'`)
AC_SUBST(SANE_BACKENDDIR, $sane_backenddir)
AC_SUBST(SANE_SYSCONFDIR, $sane_sysconfdir)

AC_MSG_CHECKING([what udev configuration directory to use])
AS_IF([test xguess == "x$with_udev_confdir"],
  [udev_sysconfdir=/etc/udev],
  [udev_sysconfdir=$with_udev_confdir])
AC_MSG_RESULT([$udev_sysconfdir])
AS_IF([test xno == x$cross_compiling],
  [dnl  When cross-compiling, you cannot access the target system's
   dnl  file system.  File existence checks are guaranteed to fail.
   AC_CHECK_FILE([$udev_sysconfdir/rules.d],
     [],
     [udev_sysconfdir=
      enable_udev_config=no
      AC_MSG_WARN([disabling udev integration])
     ])
  ])
AM_CONDITIONAL([enable_udev_config], [test xyes = "x$enable_udev_config"])
AC_SUBST(UDEV_SYSCONFDIR, $udev_sysconfdir)

AC_CHECK_PROGS([UNCRUSTIFY], [uncrustify])
AM_CONDITIONAL([enable_uncrustify_test], [test x != "x$UNCRUSTIFY"])

dnl  checks for header files

dnl  checks for typedefs, structures, and compiler characteristics

AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_CHECK_SIZEOF(unsigned long long)
AX_C___ATTRIBUTE__

dnl  checks for library functions

AC_CHECK_FUNCS([ \
  nanosleep \
  poll \
  sleep \
  usleep \
])

AC_CHECK_LIB([m], [sqrt],
  [AC_SUBST([LIBM_LIBS], [-lm])
   AC_SUBST([LIBM_CFLAGS], [])
  ],
  [AC_MSG_ERROR([math library required but not found])
  ])

dnl  The JPEG library does not come with .pc files :-(
dnl  We emulate the results of PKG_CHECK_MODULES here with a simple
dnl  test for a required library API entry.
AS_IF([test xno != "x$with_jpeg"],
  [AC_CHECK_LIB([jpeg], [jpeg_start_compress],
    [AC_DEFINE([HAVE_LIBJPEG], [1])
     AC_SUBST([LIBJPEG_LIBS], [-ljpeg])
     AC_SUBST([LIBJPEG_CFLAGS], [])
    ],
    [AS_IF([test xcheck != "x$with_jpeg"],
      [AC_MSG_ERROR([JPEG library required but not found])])
    ])
  ])
AM_CONDITIONAL([have_libjpeg], [test x != "x$LIBJPEG_LIBS"])

dnl  The TIFF library does not come with .pc files either :-((
AS_IF([test xno != "x$with_tiff"],
  [AC_CHECK_LIB([tiff], [TIFFOpen],
    [AC_DEFINE([HAVE_LIBTIFF], [1])
     AC_SUBST([LIBTIFF_LIBS], [-ltiff])
     AC_SUBST([LIBTIFF_CFLAGS], [])
    ],
    [AS_IF([test xcheck != "x$with_tiff"],
      [AC_MSG_ERROR([TIFF library required but not found])])
    ])
  ])
AM_CONDITIONAL([have_libtiff], [test x != "x$LIBTIFF_LIBS"])

dnl  We only use libmagic in our test suite and can just disable any
dnl  affected tests
AC_CHECK_LIB([magic], [magic_open],
  [AC_DEFINE([HAVE_LIBMAGIC], [1])
   AC_SUBST([LIBMAGIC_LIBS], [-lmagic])
   AC_SUBST([LIBMAGIC_CFLAGS], [])
  ])
AM_CONDITIONAL([have_libmagic], [test x != "x$LIBMAGIC_LIBS"])

AS_IF([test xno != "x$with_usb"],
  [save_CFLAGS="$CFLAGS"
   save_LDFLAGS="$LDFLAGS"
   CFLAGS="$CFLAGS $LIBUSB_CFLAGS"
   LDFLAGS="$LDFLAGS $LIBUSB_LIBS"
   AC_CHECK_LIB([usb-1.0], [libusb_error_name],
     [AC_DEFINE([HAVE_LIBUSB_ERROR_NAME], [1])])
   AC_CHECK_LIB([usb-1.0], [libusb_get_port_number],
     [AC_DEFINE([HAVE_LIBUSB_GET_PORT_NUMBER], [1])])
   LDFLAGS="$save_LDFLAGS"
   CFLAGS="$save_CFLAGS"],
  [true])

AC_CHECK_FUNCS_ONCE(sigaction)

dnl  appendix

AC_SUBST(LIBUTSUSHI_LIBS)
LIBUTSUSHI_LIBS="\$(BOOST_LDFLAGS)"
LIBUTSUSHI_LIBS="$LIBUTSUSHI_LIBS \$(BOOST_FILESYSTEM_LIB)"
LIBUTSUSHI_LIBS="$LIBUTSUSHI_LIBS \$(BOOST_PROGRAM_OPTIONS_LIB)"
LIBUTSUSHI_LIBS="$LIBUTSUSHI_LIBS \$(BOOST_REGEX_LIB)"
LIBUTSUSHI_LIBS="$LIBUTSUSHI_LIBS \$(BOOST_THREAD_LIB)"

env_var_prefix=AS_TR_CPP([${PACKAGE_TARNAME}_])
AC_SUBST([PACKAGE_ENV_VAR_PREFIX], [$env_var_prefix])
AC_DEFINE_UNQUOTED([PACKAGE_ENV_VAR_PREFIX], ["$env_var_prefix"],
  [Define to a suitable environment variable prefix.])

AM_CONDITIONAL([disable_shared], [test xno = x$enable_shared])

#  Make sure that we can always start from a default AM_*FLAGS value
#  for all compilation and linking steps for programs and libraries.

AS_IF([test "`cd $srcdir && pwd`" != "`pwd`"],
  [AC_SUBST([AM_CPPFLAGS],
     ['-I$(srcdir) -I$(top_builddir) -I$(top_srcdir) $(BOOST_CPPFLAGS)'])],
  [AC_SUBST([AM_CPPFLAGS],
     ['-I$(top_builddir) $(BOOST_CPPFLAGS)'])])

#  Check if we can assume whether All Warnings Are Really Errors
AS_IF([test xyes = x$ac_cv_cxx_compiler_gnu],
  [gcc_AWARE=-Werror])

AC_SUBST([AM_CXXFLAGS], ["-Wall $gcc_AWARE $code_coverage_cxxflags"])
AC_SUBST([AM_LDFLAGS], [""])

AS_IF([test x = "x$with_download_dir"],
  [with_download_dir=$am_aux_dir/downloads])
AC_SUBST(DOWNLOAD_DIR, $with_download_dir)

AC_CONFIG_FILES([
  Makefile
  connexions/Makefile
  doc/Makefile
  doc/tests/Makefile
  drivers/esci/Makefile
  drivers/esci/tests/Makefile
  filters/Makefile
  filters/tests/Makefile
  gtkmm/Makefile
  gtkmm/about.xml
  lib/Makefile
  lib/tests/Makefile
  sane/Makefile
  sane/tests/Makefile
  src/Makefile
  src/tests/Makefile
  tests/Makefile
  ])
AC_CONFIG_FILES([
  po/Makefile.in
  upstream/ltdl/Makefile
  ])

AC_OUTPUT
