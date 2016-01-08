//  backend.cpp -- implementation of the SANE utsushi backend
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2013  Olaf Meeuwissen
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*! \file
 *  \brief  Implements the SANE 'utsushi' backend
 *
 *  \mainpage
 *
 *  The SANE 'utsushi' backend ...
 *
 *  \todo  Expand the 'utsushi' backend's main page.
 */

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <locale>
#include <new>
#include <set>
#include <stdexcept>
#include <string>

#include <boost/preprocessor/stringize.hpp>
#include <boost/static_assert.hpp>

#include <utsushi/exception.hpp>
#include <utsushi/monitor.hpp>
#include "../lib/run-time.ipp"  /*! \todo Get rid of this kludge */

#define BACKEND_CREATE_FORWARDERS
#include "backend.hpp"
#include "version.hpp"

#include "guard.hpp"
#include "handle.hpp"
#include "device.hpp"
#include "log.hpp"

using namespace utsushi;
using boost::format;

//! Communicates backend constructor check results to sane_init()
static bool backend_is_usable = true;

//! Tracks the devices currently in use by the application
/*! The pointer's value also serves as a flag to track the backend's
 *  initialization status.
 */
static std::set< SANE_Handle > *backend = NULL;
static std::set< SANE_Handle > *expired = NULL;

//! Remembers the authorization callback passed to sane_init()
static SANE_Auth_Callback auth_cb = NULL;

//! Sets up function scope C++ exception handling aspect
/*! With the SANE API being a C API we cannot expect SANE frontends to
 *  be doing anything particularly useful with any C++ exceptions that
 *  this backend may throw.  Together with cxx_exception_aspect_footer,
 *  this macro tries to remedy that.
 *
 *  This macro provides a \c status variable at function scope that is
 *  used to communicate back an appropriate SANE_Status in the catch()
 *  clauses of cxx_exception_aspect_footer.  SANE API functions should
 *  use this variable rather than declare their own return \c status.
 *  The function identifier \a f is used for logging purposes and made
 *  available at function scope through the \c fn_name variable.  This
 *  variable acts and behaves as a C-style string.
 */
#define cxx_exception_aspect_header(f)                           \
  static const char fn_name[] = BOOST_PP_STRINGIZE (f);          \
  SANE_Status status __attribute__((unused)) = SANE_STATUS_GOOD; \
                                                                 \
  log::quark ();                                                 \
                                                                 \
  try                                                            \
    /**/

//! Completes function scope C++ exception handling aspect
/*! Complementing cxx_exception_aspect_header, this macro implements
 *  the core of our C++ exception handling aspect.  At a minimum, it
 *  logs any unhandled exception that comes its way in as much detail
 *  as possible.
 *
 *  The macro takes an \e optional sane::handle \a h that gets passed
 *  on to the cxx_exception_aspect_handler() called internally.  This
 *  allows for differentiation between uncaught exceptions that occur
 *  at SANE_Handle level from those that happen at the SANE backend
 *  level.
 */
#define cxx_exception_aspect_footer(h)                          \
  catch (const std::exception& e)                               \
    {                                                           \
      status = failure_status_;                                 \
      log::fatal                                                \
        ("%1%: unhandled exception\n"                           \
         "%2%")                                                 \
        % fn_name                                               \
        % e.what ()                                             \
        ;                                                       \
      cxx_exception_aspect_handler (h);                         \
    }                                                           \
  catch (...)                                                   \
    {                                                           \
      status = failure_status_;                                 \
      log::fatal                                                \
        ("%1%: unhandled exception")                            \
        % fn_name                                               \
        ;                                                       \
      cxx_exception_aspect_handler (h);                         \
    }                                                           \
                                                                \
  if (SANE_STATUS_GOOD != status)                               \
    {                                                           \
      log::error                                                \
        ("%1%: %2%")                                            \
        % fn_name                                               \
        % sane_strstatus (status)                               \
        ;                                                       \
    }                                                           \
  /**/

//! Implements the common C++ exception aspect part for a handle
/*! This function is mainly provided to allow the catch() clauses of
 *  cxx_exception_aspect_footer to be consistent when exiting scope.
 *
 *  \remark  Expects a \e known sane::handle \a h.
 */
static void
cxx_exception_aspect_handler (sane::handle *h)
{
  return_unless (h);
  return_if (expired->count (h));       // prevent recursion

  expired->insert (h);

  log::fatal
    ("closing handle for '%1%'")
    % h->name ()
    ;

  sane_close (h);               // may trigger unhandled exceptions!
  if (backend->erase (h))
    {
      expired->erase (h);
      delete h;
    }
}

//! Implements the common C++ exception aspect part for the backend
/*! This function is mainly provided to allow the catch() clauses of
 *  cxx_exception_aspect_footer to be consistent when exiting scope.
 *
 *  Rather than calling \c abort() directly, the implementation uses
 *  \c std::terminate().  This means that C++ client code can still
 *  customize the exception aspect if so desired.
 */
static void
cxx_exception_aspect_handler ()
{
  static sig_atomic_t backend_is_terminating = false;

  return_if (backend_is_terminating);

  backend_is_terminating = true;

  log::fatal
    ("exiting SANE '%1%' backend")
    % BOOST_PP_STRINGIZE (BACKEND_NAME)
    ;

  if (backend)                  // forcefully close all handles
    {
      std::set< SANE_Handle >::iterator it;
      for (it = backend->begin (); it != backend->end (); ++it)
        {
          cxx_exception_aspect_handler (static_cast< sane::handle * > (*it));
        }
    }

  sane::device::release ();
  delete backend;
  backend = NULL;

  //! \todo  Add bit flipping support to log::category
  // log::matching &= ~log::SANE_BACKEND;

  delete run_time::impl::instance_;
  run_time::impl::instance_ = 0;

  backend_is_terminating = false;

  std::terminate ();
}

static std::string
frame_to_string (const SANE_Frame& f)
{
  switch (f)
    {
    case SANE_FRAME_GRAY : return "GRAY";
    case SANE_FRAME_RGB  : return "RGB";
    case SANE_FRAME_RED  : return "RED";
    case SANE_FRAME_GREEN: return "GREEN";
    case SANE_FRAME_BLUE : return "BLUE";
    }
  return "(unknown)";
}

//! Map system_error objects to a corresponding SANE_Status
static SANE_Status
exception_to_sane_status (const system_error& e)
{
  log::error ("system_error: %1%") % e.what ();

  if (system_error::media_out  == e.code ()) return SANE_STATUS_NO_DOCS;
  if (system_error::media_jam  == e.code ()) return SANE_STATUS_JAMMED;
  if (system_error::cover_open == e.code ()) return SANE_STATUS_COVER_OPEN;
  if (system_error::permission_denied == e.code ())
    return SANE_STATUS_ACCESS_DENIED;

  return SANE_STATUS_IO_ERROR;
}

using std::runtime_error;

//! Map runtime_error objects to a corresponding SANE_Status
/*! \warn  The implementation relies heavily on the implementation of
 *         the driver in terms of the message used.
 *  \note  This overload is used as a fallback for situations where
 *         the exception handling fails to recognize a system_error
 *         and would end up in the cxx_exception_aspect_footer().
 *         This odd behaviour has been observed on Ubuntu 10.04 LTS
 *         and is believed to be due to an issue in the C++ runtime
 *         (4.4.3-4ubuntu5.1).  The very same binary code worked as
 *         intended on Ubuntu 12.04 LTS.
 */
static SANE_Status
exception_to_sane_status (const runtime_error& e)
{
  log::error ("runtime_error: %1%") % e.what ();

  std::string msg (_(e.what ()));

  if (0 == msg.compare
      (SEC_("Please load the document(s) into the Automatic Document"
            " Feeder.")))
    return SANE_STATUS_NO_DOCS;

  if (0 == msg.compare
      (SEC_("A paper jam occurred.\n"
            "Open the Automatic Document Feeder and remove any paper.\n"
            "If there are any documents loaded in the ADF, remove them"
            " and load them again.")))
    return SANE_STATUS_JAMMED;

  if (0 == msg.compare
      (SEC_("A multi page feed occurred in the auto document feeder. "
            "Open the cover, remove the documents, and then try again."
            " If documents remain on the tray, remove them and then"
            " reload them.")))
    return SANE_STATUS_JAMMED;

  if (0 == msg.compare
      (SEC_("The Automatic Document Feeder is open.\n"
            "Please close it.")))
    return SANE_STATUS_COVER_OPEN;

  if (0 == msg.compare ("Device initiated cancellation."))
    return SANE_STATUS_CANCELLED;

  return SANE_STATUS_IO_ERROR;
}

#define not_initialized_message                                 \
  (format ("The '%1%' backend is currently not initialized")    \
   % BOOST_PP_STRINGIZE (BACKEND_NAME)).str ()                  \
  /**/
#define not_known_message                                               \
  (format ("Memory at %1% was not acquired by the '%2%' backend")       \
   % handle % BOOST_PP_STRINGIZE (BACKEND_NAME)).str ()                 \
  /**/

//  Readability macros used for SANE frontend API usage compliance
//  checking and argument screening.

#define return_unless_initialized(backend)                              \
  return_verbosely_unless (backend, not_initialized_message)            \
  /**/
#define return_failure_unless_initialized(backend)                      \
  return_failure_verbosely_unless (backend, not_initialized_message)    \
  /**/
#define return_value_unless_initialized(backend,value)                  \
  return_value_verbosely_unless (backend, not_initialized_message, value) \
  /**/

#define return_unless_known(handle)                                     \
  return_unless_initialized (backend);                                  \
  return_verbosely_if (0 == backend->count (handle),                    \
                       not_known_message)                               \
  /**/
#define return_failure_unless_known(handle)                             \
  return_failure_unless_initialized (backend);                          \
  return_failure_verbosely_if (0 == backend->count (handle),            \
                               not_known_message)                       \
  /**/
#define return_value_unless_known(handle,value)                         \
  return_value_unless_initialized (backend, value);                     \
  return_value_verbosely_if (0 == backend->count (handle),              \
                             not_known_message, value)                  \
  /**/

extern "C" {

/*! \defgroup SANE_API  SANE API Entry Points
 *
 *  The SANE API entry points make up the \e full API available to the
 *  SANE frontend application programmer.  Users of this API should be
 *  careful \e never to assume \e anything about a backend's behaviour
 *  beyond what is required by the SANE standard.  The standard can be
 *  retrieved via http://sane.alioth.debian.org/docs.html on the SANE
 *  project's web site.
 *
 *  Whatever documentation may be provided here serves to document the
 *  implementation, if anything.  In case of discrepancy with the SANE
 *  specification, the SANE specification is correct.
 *
 *  @{
 */

//! Prepares the backend for use by a SANE frontend
/*! \remarks
 *  This function \e must be called before any other SANE API entry is
 *  called.  It is the only SANE function that may be called after the
 *  sane_exit() function has been called.
 */
SANE_Status
sane_init (SANE_Int *version_code, SANE_Auth_Callback authorize)
{
  return_value_unless (backend_is_usable, failure_status_);

  cxx_exception_aspect_header (sane_init)
    {
      //! \todo  Add bit flipping support to log::category
      // log::matching |= log::SANE_BACKEND;

      log::brief
        ("%1%: SANE '%2%' backend (%3%.%4%.%5%), a part of %6%")
        % fn_name
        % BOOST_PP_STRINGIZE (BACKEND_NAME)
        % BACKEND_MAJOR % BACKEND_MINOR % BACKEND_BUILD
        % BACKEND_SOURCE
        ;

      /*! \todo  Decide whether to return a version code and/or reset the
       *         authorization callback when called without an intervening
       *         call to sane_exit().
       */

      if (version_code)
        {
          *version_code = SANE_VERSION_CODE (BACKEND_MAJOR, BACKEND_MINOR,
                                             BACKEND_BUILD);
        }

      auth_cb = authorize;

      return_value_if (backend, SANE_STATUS_GOOD);

      try
        {
          expired = new std::set< SANE_Handle >;
          backend = new std::set< SANE_Handle >;
          sane::device::pool = new std::vector< sane::device >;
        }
      catch (const std::bad_alloc& e)
        {
          log::error ("%1%: %2%") % fn_name % e.what ();
          sane_exit ();
          status = SANE_STATUS_NO_MEM;
        }
    }
  cxx_exception_aspect_footer ();

  return status;
}

//! Releases all resources held by the backend
/*! \remarks
 *  Applications \e must call this function to terminate use of the
 *  backend.  After it has been called, sane_init() has to be called
 *  before other SANE API can be used.  The function needs to close
 *  any open handles.
 *
 *  \remarks
 *  The implementation must be able to deal properly with a partially
 *  initialised backend so that sane_init() can use this function for
 *  its error recovery.
 */
void
sane_exit (void)
{
  cxx_exception_aspect_header (sane_exit)
    {
      return_unless_initialized (backend);

      sane::device::release ();
      delete sane::device::pool;

      if (backend)
        {
          log::trace ("%1%: closing open handles") % fn_name;
          std::for_each (backend->begin (), backend->end (), sane_close);
        }
      delete backend;
      backend = NULL;
      delete expired;
      expired = NULL;

      //! \todo  Add bit flipping support to log::category
      // log::matching &= ~log::SANE_BACKEND;
    }
  cxx_exception_aspect_footer ();

  return;
}

//! Creates a list of devices available through the backend
/*! \remarks
 *  The returned \a device_list \e must remain unchanged and valid
 *  until this function is called again or sane_exit() is called.
 *
 *  \remarks
 *  When returning successfully, the \a device_list points to a \c
 *  NULL terminated list of \c SANE_Device pointers.
 *
 *  \remarks
 *  Applications are \e not required to call this function before
 *  they call sane_open().
 */
SANE_Status
sane_get_devices (const SANE_Device ***device_list, SANE_Bool local_only)
{
  cxx_exception_aspect_header (sane_get_devices)
    {
      return_failure_unless_initialized (backend);
      return_invalid_unless (device_list);

      sane::device::release ();
      log::trace ("%1%: invalidated SANE_Device pointers") % fn_name;

      try
        {
          monitor mon;
          monitor::const_iterator it = mon.begin ();

          for (; mon.end () != it; ++it)
            {
              if (!it->is_driver_set ())          continue;
              if (local_only && !it->is_local ()) continue;

              sane::device::pool->push_back (sane::device (*it));
              log::debug
                ("%1%: added %2% to device pool")
                % fn_name
                % it->udi ();
            }

          sane::device::list
            = new const SANE_Device * [sane::device::pool->size () + 1];

          std::vector< sane::device >::size_type i;
          for (i = 0; i < sane::device::pool->size (); ++i)
            {
              sane::device::list[i] = &sane::device::pool->at (i);
            }
          sane::device::list[i] = NULL;
        }
      catch (const std::bad_alloc& e)
        {
          log::error ("%1%: %2%") % fn_name % e.what ();
          status = SANE_STATUS_NO_MEM;
        }

      *device_list = sane::device::list;
   }
  cxx_exception_aspect_footer ();

  return status;
}

//! Establishes a connection to a named device
/*! \remarks
 *  Applications are allowed to call this function directly, without a
 *  call to sane_get_devices() first.  An empty string may be used for
 *  the \a device_name to request the first available device.
 *
 *  \remarks
 *  The SANE specification says nothing about required behaviour when
 *  the frontend passes a \c NULL \a device_name, only when the empty
 *  string is passed do we have to do something special.  We degrade
 *  gracefully anyway and treat \c NULL as if it were an empty string.
 */
SANE_Status
sane_open (SANE_String_Const device_name, SANE_Handle *handle)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_open)
    {
      return_failure_unless_initialized (backend);
      return_invalid_unless (handle);

      if (!device_name)
        {
          log::brief
            ("%1%: assuming frontend meant to pass an empty string")
            % fn_name;
        }

      std::string udi (device_name ? device_name : "");
      monitor mon;

      if (udi.empty ())
        {
          udi = mon.default_device ();
        }

      monitor::const_iterator it (mon.find (udi));

      if (it == mon.end ())
        {
          if (!udi.empty ())
            {
              log::error ("%1%: '%2%' not found") % fn_name % udi;
            }
          else
            {
              log::error ("%1%: no usable devices available") % fn_name;
            }
          return SANE_STATUS_INVAL;
        }

      if (!it->is_driver_set ())
        {
          log::alert ("%1%: '%2%' found but has no driver") % fn_name % udi;
          return SANE_STATUS_UNSUPPORTED;
        }

      try
        {
          log::trace
            ("%1%: creating SANE_Handle for %2%")
            % fn_name
            % it->udi ();
          h = new sane::handle (*it);
          backend->insert (h);

          *handle = h;
        }
      catch (const std::bad_alloc& e)
        {
          log::error ("%1%: %2%") % fn_name % e.what ();
          status = SANE_STATUS_NO_MEM;
        }
    }
  cxx_exception_aspect_footer (h);

  return status;
}

//! Terminates the association of a handle with a device
/*! \remarks
 *  A call to sane_cancel() will be issued if the device is active.
 */
void
sane_close (SANE_Handle handle)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_close)
    {
      return_unless_known (handle);

      h = static_cast< sane::handle * > (handle);

      sane_cancel (h);

      backend->erase (h);
      delete h;
    }
  cxx_exception_aspect_footer (h);

  return;
}

//! Provides information about an indexed device option
/*! \remarks
 *  Option descriptors returned \e must remain valid and at the \e
 *  same address until the \a handle is closed.  A descriptor for an
 *  \a index of zero must exist.  It describes the option count (the
 *  number of options that is available for a \a handle).
 *
 *  \remarks
 *  The SANE specification states, in "4.4 Code Flow", that the number
 *  of options for a given \a handle is \e fixed.  Options may become
 *  active or inactive as the result of setting other options but the
 *  option count remains constant.
 */
const SANE_Option_Descriptor *
sane_get_option_descriptor (SANE_Handle handle, SANE_Int index)
{
  const SANE_Option_Descriptor *desc = NULL;
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_get_option_descriptor)
    {
      return_value_unless_known (handle, NULL);

      h = static_cast< sane::handle * > (handle);

      return_value_unless (0 <= index && index < h->size (), NULL);

      desc = h->descriptor (index);
    }
  cxx_exception_aspect_footer (h);

  return desc;
}

//! Queries or modifies an indexed device option
/*! \remarks
 *  Modifying an option does \e not guarantee that it gets set to the
 *  exact \a value that was passed.
 *
 *  \remarks
 *  After sane_start() has been called, none of the scan parameters
 *  are supposed to change until the completion of a scan.  This is
 *  typically until sane_cancel() or sane_close() is called.  While
 *  setting options in this time frame is not forbidden, it sure is
 *  rather strange to do so.
 *
 *  \remarks
 *  The specification explicitly mentions that when invoked with an \a
 *  action of \c SANE_ACTION_SET_AUTO the \a value is to be completely
 *  ignored and may be \c NULL.
 */
SANE_Status
sane_control_option (SANE_Handle handle, SANE_Int index, SANE_Action action,
                     void *value, SANE_Word *info)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_control_option)
    {
      return_failure_unless_known (handle);
      return_invalid_if (SANE_ACTION_GET_VALUE == action && !value);
      return_invalid_if (SANE_ACTION_SET_VALUE == action && !value);

      h = static_cast< sane::handle * > (handle);

      return_invalid_unless (0 <= index && index < h->size ());
      //! \todo Review control of inactive options
      return_invalid_unless (h->is_active (index));

      //! \todo Review control of group options
      return_invalid_if (h->is_group (index));

      /**/ if (SANE_ACTION_GET_VALUE == action)
        {
          status = h->get (index, value);
        }
      else if (SANE_ACTION_SET_VALUE == action)
        {
          return_invalid_unless (h->is_settable (index));
          status = h->set (index, value, info);
        }
      else if (SANE_ACTION_SET_AUTO  == action)
        {
          return_invalid_unless (h->is_automatic (index));
          status = h->set (index, info);
        }
      else
        {
          log::error ("%1%: invalid action") % fn_name;
          status = SANE_STATUS_INVAL;
        }
    }
  cxx_exception_aspect_footer (h);

  return status;
}

//! Obtains the current scan parameters for a device
/*! \remarks
 *  The parameters are only guaranteed to be accurate between a call
 *  to sane_start() and the completion of that request.  Outside of
 *  that scope the parameters are a best effort only and the backend
 *  is at liberty to change them.
 */
SANE_Status
sane_get_parameters (SANE_Handle handle, SANE_Parameters *parameters)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_get_parameters)
    {
      return_failure_unless_known (handle);
      return_invalid_unless (parameters);

      h = static_cast< sane::handle * > (handle);

      const context& ctx = h->get_context ();

      parameters->format = (3 == ctx.comps ()
                            ? SANE_FRAME_RGB
                            : SANE_FRAME_GRAY);
      parameters->last_frame = SANE_TRUE;
      parameters->lines = (context::unknown_size != ctx.lines_per_image ()
                           ? ctx.lines_per_image ()
                           : -1);
      parameters->depth = ctx.depth ();
      parameters->pixels_per_line = (context::unknown_size != ctx.width ()
                                     ? ctx.width ()
                                     : 0);
      parameters->bytes_per_line = ctx.octets_per_line ();

      log::brief
        ("%1%: %2% frame") % fn_name % frame_to_string (parameters->format);
      log::brief
        ("%1%: %2% lines") % fn_name % parameters->lines;
      log::brief
        ("%1%: %2% pixels/line") % fn_name % parameters->pixels_per_line;
      log::brief
        ("%1%: %2% bytes/line (%3% padding)")
        %  fn_name
        %  parameters->bytes_per_line
        % (parameters->bytes_per_line - ctx.scan_width ());
      log::brief
        ("%1%: %2% bits/sample") % fn_name % parameters->depth;
      log::brief
        ("%1%: last frame: %2%") % fn_name % (parameters->last_frame
                                              ? "yes" : "no");
    }
  cxx_exception_aspect_footer (h);

  return status;
}

//! Initiates acquisition of image data for a single frame
/*! \remarks
 *  The SANE API leaves the start of the \e physical data acquisition
 *  to the discretion of the backend implementation.  It can be done
 *  in this function or in sane_read().  However, given the fact that
 *  sane_set_io_mode() and sane_get_select_fd() can only be called \e
 *  after calling sane_start(), postponing the start of physical data
 *  acquisition to the first sane_read() invocation is probably to be
 *  preferred (unless physical data acquistion itself is non-blocking
 *  to begin with).
 */
SANE_Status
sane_start (SANE_Handle handle)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_start)
    {
      return_failure_unless_known (handle);

      h = static_cast< sane::handle * > (handle);

      streamsize rv = h->start ();

      if (traits::boi () != rv)
        {
          status = SANE_STATUS_INVAL;
          if (traits::eos () == rv) status = SANE_STATUS_NO_DOCS;
          if (traits::eoi () == rv) status = SANE_STATUS_EOF;
          if (traits::eof () == rv) status = SANE_STATUS_CANCELLED;
        }
    }
  catch (const system_error& e)
    {
      status = exception_to_sane_status (e);
    }
  catch (const runtime_error& e)
    {
      status = exception_to_sane_status (e);
    }
  cxx_exception_aspect_footer (h);

  return status;
}

//! Acquires up to \a max_length bytes of new image data
/*! \remarks
 *  The \a length is guaranteed to be zero in case of an unsuccessful
 *  request.
 */
SANE_Status
sane_read (SANE_Handle handle, SANE_Byte *buffer, SANE_Int max_length,
           SANE_Int *length)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_read)
    {
      if (length) *length = 0;

      return_failure_unless_known (handle);
      return_invalid_unless (buffer && length && (0 < max_length));

      h = static_cast< sane::handle * > (handle);

      BOOST_STATIC_ASSERT (sizeof (octet) == sizeof (SANE_Byte));

      *length = h->read (reinterpret_cast< octet * > (buffer),
                         max_length);

      if (traits::is_marker (*length))
        {
          status = SANE_STATUS_IO_ERROR;
          if (traits::eos () == *length) status = SANE_STATUS_NO_DOCS;
          if (traits::eoi () == *length) status = SANE_STATUS_EOF;
          if (traits::eof () == *length) status = SANE_STATUS_CANCELLED;
        }

      if (SANE_STATUS_GOOD != status)
        {
          *length = 0;
        }

      // The SANE specification follows the PNM specification for its
      // SANE_FRAME_GRAY images.  Assume that the Utsushi devices and
      // streams produce light oriented values and correct here.
      // Note, this uses "experimental" context API.

      if (   1 == h->get_context ().depth ()
          && 1 == h->get_context ().comps ())
        {
          for (SANE_Int i = 0; i < *length; ++i, ++buffer)
            {
              *buffer = ~*buffer;

              if (HAVE_GRAPHICS_MAGICK)
                {
                  octet v = *buffer;

                  v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
                  v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
                  v = ((v >> 4) & 0x0F) | ((v & 0x0F) << 4);

                  *buffer = v;
                }
            }
        }

      log::brief
        ("%1%: %2% bytes (of %3% requested)")
        % fn_name
        % *length
        % max_length;
    }
  catch (const system_error& e)
    {
      status = exception_to_sane_status (e);
    }
  catch (const runtime_error& e)
    {
      status = exception_to_sane_status (e);
    }
  cxx_exception_aspect_footer (h);

  return status;
}

//! Initiates cancellation of the currently pending operation
/*! \remarks
 *  As per "4.4 Code Flow", this function \e must be called when all
 *  frames or images have been acquired.  If a SANE frontend expects
 *  additional frames \e or images the function should not be called
 *  until the last frame or image has been acquired.
 *
 *  \remarks
 *  It is safe to call this function asynchronously (e.g. from signal
 *  handlers).  Its completion only guarantees that cancellation of a
 *  long-running operation has been initiated, not that cancellation
 *  of that operation has completed.  The long-running operation will
 *  typically be acquisition of an image but other operations such as
 *  initiating a scan with sane_start() or performing calibration via
 *  a call to sane_control_option() for a \c SANE_TYPE_BUTTON option
 *  could be subject to cancellation as well.  Note, though, that only
 *  sane_read() lends itself to straightforward cancellation support
 *  (due to its intended use in a loop construct).
 */
void
sane_cancel (SANE_Handle handle)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_cancel)
    {
      return_unless_known (handle);

      h = static_cast< sane::handle * > (handle);

      h->cancel ();
    }
  cxx_exception_aspect_footer (h);

  return;
}

//! Controls whether device I/O is (non-)blocking
/*! \remarks
 *  Blocking I/O is the default I/O mode and \e must be supported.
 *  Support for non-blocking I/O is optional.
 *
 *  \remarks
 *  This function may only be called after a call to sane_start().
 */
SANE_Status
sane_set_io_mode (SANE_Handle handle, SANE_Bool non_blocking)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_set_io_mode)
    {
      return_failure_unless_known (handle);

      h = static_cast< sane::handle * > (handle);

      return_invalid_unless (h->is_scanning ());

      status = (!non_blocking
                ? SANE_STATUS_GOOD
                : SANE_STATUS_UNSUPPORTED);
    }
  cxx_exception_aspect_footer (h);

  return status;
}

//! Obtains a file descriptor if image data is available
/*! \remarks
 *  Support for file descriptors is optional.  The file descriptor is
 *  guaranteed to remain valid for the duration of the current image
 *  acquisition.  That is, until sane_read() returns SANE_STATUS_EOF
 *  or the frontend calls one of sane_cancel() or sane_start().
 *
 *  \remarks
 *  This function may only be called after a call to sane_start().
 */
SANE_Status
sane_get_select_fd (SANE_Handle handle, SANE_Int *fdp)
{
  sane::handle *h = NULL;

  cxx_exception_aspect_header (sane_get_select_fd)
    {
      return_failure_unless_known (handle);
      return_invalid_unless (fdp);

      h = static_cast< sane::handle * > (handle);

      return_invalid_unless (h->is_scanning ());

      status = SANE_STATUS_UNSUPPORTED;
    }
  cxx_exception_aspect_footer (h);

  return status;
}

/*! @} */

/*  Hooks into the dynamic loading API that allow for initialization
 *  before the backend's sane_init() and cleanup after the backend's
 *  sane_exit().  The hooks are backend specific and \em not part of
 *  the SANE API.
 */
static void
API_ENTRY (BACKEND_NAME, ctor) (void) __attribute__((constructor));
static void
API_ENTRY (BACKEND_NAME, dtor) (void) __attribute__((destructor));

//! Performs usability checks at first time backend loading
/*! This is a hook into the library loading mechanism that carries out
 *  whatever is necessary to make sure that the backend will be really
 *  usable at run-time.
 *
 *  The main inspiration for this approach comes from an issue with an
 *  upstream OpenOffice.org (3.2.1) package that shipped with its own
 *  copy of the standard C++ library.  That library did not appear to
 *  have any localization support activated and caused loading of the
 *  backend to throw an exception \em before sane_init().  The locale
 *  used at that time was \c en_US.UTF-8, i.e. a regular Linux locale
 *  specification, with the en-US OpenOffice.org package.
 */
static void
API_ENTRY (BACKEND_NAME, ctor) (void)
{
  try
    {
      std::locale ("");
    }
  catch (const std::runtime_error& e)
    {
      fprintf (stderr, "%s\n", CCB_(
"The current locale settings are not supported by the standard C++"
" library used by this application.  This is most likely caused by a"
" misconfigured locale but may also be due to use of a C++ library"
" without localization support.  You can work around this issue by"
" starting the application in a \"C\" locale, but you really should"
" check your locale configuration and the locale support of the C++"
" library used by the application."
                                    ));
      backend_is_usable = false;
    }

  try
    {
      const char *argv[] = { "SANE Backend" };

      run_time (1, argv);
    }
  catch (const std::exception& e)
    {
      fprintf (stderr, "%s\n", CCB_(e.what ()));
      backend_is_usable = false;
    }
  catch (...)
    {
      fprintf (stderr, "%s\n", CCB_("library initialization failed"));
      backend_is_usable = false;
    }
}

//! Undoes the effects of the backend constructor
static void
API_ENTRY (BACKEND_NAME, dtor) (void)
{
  delete run_time::impl::instance_;
  run_time::impl::instance_ = 0;

  backend_is_usable = true;
}

}       // extern "C"
