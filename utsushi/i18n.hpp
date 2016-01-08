//  i18n.hpp -- C++ wrappers for libintl.h functionality
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

#ifndef utsushi_i18n_hpp_
#define utsushi_i18n_hpp_

/*! \file
 *  \brief C++ wrappers for \c libintl.h functionality
 *
 *  The \c libintl.h header file provides a C API to the \c gettext
 *  family of functions.  This file provides convenience wrappers and
 *  overloads that exploit the capabilities of the C++ language.  The
 *  \c gettext, \c dgettext and \c dcgettext C API has been collected
 *  in a set of gettext() overloads.  Similarly, the \c ngettext, \c
 *  dngettext and \c dcngettext C API is available through ngettext()
 *  overloads.  In addition, a full set of overloads for the C++ \c
 *  std::string class is provided.
 *
 *  This header file honours the \c ENABLE_NLS preprocessor macro.
 *  Define it to a \c true value before including this file to enable
 *  national language support.  For @PACKAGE_NAME@ this is achieved by
 *  simply including the \c config.h file.
 *  By default \c ENABLE_NLS evaluates to \c false.
 *
 *  A \c DEFAULT_TEXT_DOMAIN preprocessor macro may be defined to make
 *  gettext() behave as if a \a domainname had been given.  When not
 *  defined, gettext() behaves as if no such name has been specified.
 *  For @PACKAGE_NAME@, the \c DEFAULT_TEXT_DOMAIN should be set via
 *  inclusion of \c config.h.
 *
 *  Finally, the common markup keywords _() and N_() are supported.
 */

#include <libintl.h>

#include <string>

#ifndef ENABLE_NLS
#define ENABLE_NLS 0
#endif

#ifndef DEFAULT_TEXT_DOMAIN
#define DEFAULT_TEXT_DOMAIN NULL
#endif

namespace utsushi {

static const char *default_text_domain (DEFAULT_TEXT_DOMAIN);

enum { i18n = 1 };

inline
const char *
gettext (const char *domainname, const char *msgid, int category)
{
  return (ENABLE_NLS
          ? ::dcgettext (domainname, msgid, category)
          : msgid);
}

inline
const char *
gettext (const char *domainname, const char *msgid)
{
  return (ENABLE_NLS
          ? ::dgettext (domainname, msgid)
          : msgid);
}

inline
const char *
gettext (const char *msgid)
{
  return (ENABLE_NLS
          ? gettext (default_text_domain, msgid)
          : msgid);
}

inline
const char *
ngettext (const char *domainname, const char *msgid, const char *msgid_plural,
          unsigned long int n, int category)
{
  return (ENABLE_NLS
          ? ::dcngettext (domainname, msgid, msgid_plural, n, category)
          : (1 == n
             ? msgid
             : msgid_plural));
}

inline
const char *
ngettext (const char *domainname, const char *msgid, const char *msgid_plural,
          unsigned long int n)
{
  return (ENABLE_NLS
          ? ::dngettext (domainname, msgid, msgid_plural, n)
          : (1 == n
             ? msgid
             : msgid_plural));
}

inline
const char *
ngettext (const char *msgid, const char *msgid_plural,
          unsigned long int n)
{
  return (ENABLE_NLS
          ? ngettext (default_text_domain, msgid, msgid_plural, n)
          : (1 == n
             ? msgid
             : msgid_plural));
}

inline
const char *
textdomain (const char *domainname)
{
  return (ENABLE_NLS
          ? ::textdomain (domainname)
          : domainname);
}

inline
const char *
bindtextdomain (const char *domainname, const char *dirname)
{
  return (ENABLE_NLS
          ? ::bindtextdomain (domainname, dirname)
          : dirname);
}

inline
const char *
bind_textdomain_codeset (const char *domainname, const char *codeset)
{
  return (ENABLE_NLS
          ? ::bind_textdomain_codeset (domainname, codeset)
          : codeset);
}

// C++ std::string overloads

inline
const char *
gettext (const std::string& domainname, const std::string& msgid,
         int category)
{
  return gettext (domainname.c_str (), msgid.c_str (), category);
}

inline
const char *
gettext (const std::string& domainname, const char *msgid,
         int category)
{
  return gettext (domainname.c_str (), msgid, category);
}

inline
const char *
gettext (const char *domainname, const std::string& msgid,
         int category)
{
  return gettext (domainname, msgid.c_str (), category);
}

inline
const char *
gettext (const std::string& domainname, const std::string& msgid)
{
  return gettext (domainname.c_str (), msgid.c_str ());
}

inline
const char *
gettext (const std::string& domainname, const char *msgid)
{
  return gettext (domainname.c_str (), msgid);
}

inline
const char *
gettext (const char *domainname, const std::string& msgid)
{
  return gettext (domainname, msgid.c_str ());
}

inline
const char *
gettext (const std::string& msgid)
{
  return gettext (msgid.c_str ());
}

inline
const char *
ngettext (const std::string& domainname,
          const std::string& msgid, const std::string& msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname.c_str (), msgid.c_str (), msgid_plural.c_str (),
                   n, category);
}

inline
const char *
ngettext (const std::string& domainname,
          const std::string& msgid, const char *msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname.c_str (), msgid.c_str (), msgid_plural,
                   n, category);
}

inline
const char *
ngettext (const std::string& domainname,
          const char *msgid, const std::string& msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname.c_str (), msgid, msgid_plural.c_str (),
                   n, category);
}

inline
const char *
ngettext (const std::string& domainname,
          const char *msgid, const char *msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname.c_str (), msgid, msgid_plural,
                   n, category);
}

inline
const char *
ngettext (const char *domainname,
          const std::string& msgid, const std::string& msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname, msgid.c_str (), msgid_plural.c_str (),
                   n, category);
}

inline
const char *
ngettext (const char *domainname,
          const std::string& msgid, const char *msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname, msgid.c_str (), msgid_plural,
                   n, category);
}

inline
const char *
ngettext (const char *domainname,
          const char *msgid, const std::string& msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (domainname, msgid, msgid_plural.c_str (),
                   n, category);
}

inline
const char *
ngettext (const std::string& domainname,
          const std::string& msgid, const std::string& msgid_plural,
          unsigned long int n)
{
  return ngettext (domainname.c_str (), msgid.c_str (), msgid_plural.c_str (),
                   n);
}

inline
const char *
ngettext (const std::string& domainname,
          const std::string& msgid, const char *msgid_plural,
          unsigned long int n)
{
  return ngettext (domainname.c_str (), msgid.c_str (), msgid_plural,
                   n);
}

inline
const char *
ngettext (const std::string& domainname,
          const char *msgid, const std::string& msgid_plural,
          unsigned long int n)
{
  return ngettext (domainname.c_str (), msgid, msgid_plural.c_str (),
                   n);
}

inline
const char *
ngettext (const char *domainname,
          const std::string& msgid, const std::string& msgid_plural,
          unsigned long int n)
{
  return ngettext (domainname, msgid.c_str (), msgid_plural.c_str (),
                   n);
}

inline
const char *
ngettext (const char *domainname,
          const std::string& msgid, const char *msgid_plural,
          unsigned long int n)
{
  return ngettext (domainname, msgid.c_str (), msgid_plural,
                   n);
}

inline
const char *
ngettext (const char *domainname,
          const char *msgid, const std::string& msgid_plural,
          unsigned long int n)
{
  return ngettext (domainname, msgid, msgid_plural.c_str (),
                   n);
}

inline
const char *
ngettext (const std::string& msgid, const std::string& msgid_plural,
          unsigned long int n)
{
  return ngettext (msgid.c_str (), msgid_plural.c_str (), n);
}

inline
const char *
ngettext (const std::string& msgid, const char *msgid_plural,
          unsigned long int n)
{
  return ngettext (msgid.c_str (), msgid_plural, n);
}

inline
const char *
ngettext (const char *msgid, const std::string& msgid_plural,
          unsigned long int n)
{
  return ngettext (msgid, msgid_plural.c_str (), n);
}

inline
const char *
textdomain (const std::string& domainname)
{
  return textdomain (domainname.c_str ());
}

inline
const char *
bindtextdomain (const std::string& domainname, const std::string& dirname)
{
  return bindtextdomain (domainname.c_str (), dirname.c_str ());
}

inline
const char *
bindtextdomain (const std::string& domainname, const char *dirname)
{
  return bindtextdomain (domainname.c_str (), dirname);
}

inline
const char *
bindtextdomain (const char *domainname, const std::string& dirname)
{
  return bindtextdomain (domainname, dirname.c_str ());
}

inline
const char *
bind_textdomain_codeset (const std::string& domainname,
                         const std::string& codeset)
{
  return bind_textdomain_codeset (domainname.c_str (), codeset.c_str ());
}

inline
const char *
bind_textdomain_codeset (const std::string& domainname, const char *codeset)
{
  return bind_textdomain_codeset (domainname.c_str (), codeset);
}

inline
const char *
bind_textdomain_codeset (const char *domainname, const std::string& codeset)
{
  return bind_textdomain_codeset (domainname, codeset.c_str ());
}

// additional default_text_domain overloads

inline
const char *
gettext (const std::string& msgid, int category)
{
  return gettext (default_text_domain, msgid.c_str (), category);
}

inline
const char *
gettext (const char *msgid, int category)
{
  return gettext (default_text_domain, msgid, category);
}

inline
const char *
ngettext (const std::string& msgid, const std::string& msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (default_text_domain, msgid.c_str (), msgid_plural.c_str (),
                   n, category);
}

inline
const char *
ngettext (const std::string& msgid, const char *msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (default_text_domain, msgid.c_str (), msgid_plural,
                   n, category);
}

inline
const char *
ngettext (const char *msgid, const std::string& msgid_plural,
          unsigned long int n, int category)
{
  return ngettext (default_text_domain, msgid, msgid_plural.c_str (),
                   n, category);
}

inline
const char *
textdomain ()
{
  return textdomain (default_text_domain);
}

inline
const char *
bindtextdomain (const std::string& dirname)
{
  return bindtextdomain (default_text_domain, dirname.c_str ());
}

inline
const char *
bindtextdomain (const char *dirname)
{
  return bindtextdomain (default_text_domain, dirname);
}

inline
const char *
bind_textdomain_codeset (const std::string& codeset)
{
  return bind_textdomain_codeset (default_text_domain, codeset.c_str ());
}

inline
const char *
bind_textdomain_codeset (const char *codeset)
{
  return bind_textdomain_codeset (default_text_domain, codeset);
}

// Common marker keyword overloads

inline
const char *
_(const std::string& msgid)
{
  return gettext (msgid.c_str ());
}

inline
const char *
_(const char *msgid)
{
  return gettext (msgid);
}

inline
const char *
N_(const std::string& msgid)
{
  return msgid.c_str ();
}

inline
const char *
N_(const char *msgid)
{
  return msgid;
}

// Translation "responsibility" scope defines

#define SEC_     _              /* SEIKO EPSON CORPORATION */
#define SEC_N_  N_
#define CCB_     _              /* Community Code Base */
#define CCB_N_  N_

}       // namespace utsushi

#endif  /* utsushi_i18n_hpp_ */
