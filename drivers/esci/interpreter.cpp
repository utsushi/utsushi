//  interpreter.cpp -- API entrypoints to use protocol translators
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
//
//  License: BSL-1.0
//  Author : EPSON AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This file is distributed under the terms of the Boost Software
//  License, Version 1.0.
//
//  You ought to have received a copy of the Boost Software License
//  along along with this package.
//  If not, see <http://www.boost.org/LICENSE_1_0.txt>.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "interpreter.hpp"

#include "connexion.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <sys/types.h>
#include <unistd.h>

#include <ltdl.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

static void load_interpreter (const std::string& argv0);
static void set_signal (int, void (*) (int));
extern "C" { static void request_cancel (int); }

/*! This plugin ... 
 *
 *  The plugin should be started as a child process by the application
 *  that uses the driver.  At process start up, the plugin outputs the
 *  port number it will use for inter-process communication (IPC).  It
 *   should be noted that this convention, as well as the IPC protocol
 *  itself, are subject to change and will follow the specification of
 *  the Utsushi project.
 */
int
main (int argc, const char *argv[])
{
  std::string argv0 (argv[0]);
  pid_t ppid (getppid ());

  int status = EXIT_FAILURE;
  bool output_port = false;

#if HAVE_PRCTL

  // Request for a signal to be sent when the parent dies.  It is not
  // a big deal if that fails, but it may not be possible to clean up
  // after ourselves in that case.

  if (prctl (PR_SET_PDEATHSIG, SIGHUP))
    std::clog << strerror (errno) << std::endl;

#endif  /* HAVE_PRCTL */

  set_signal (SIGTERM, request_cancel);
  set_signal (SIGINT , request_cancel);
  set_signal (SIGHUP , request_cancel);

  lt_dlinit ();
  try
    {
      std::size_t pos = argv0.find_last_of ("/");
      std::string argv0stem = argv0;
      if (std::string::npos != pos)
        argv0stem = argv0.substr (pos + 1);
      load_interpreter (argv0stem);

      connexion   cnx;
      header      hdr;
      std::string payload;

      std::cout << cnx.port () << std::endl;
      output_port = true;

      cnx.accept ();
      while (!cnx.eof ())
        {
          if (0 <= cnx.read (hdr, payload))
            {
              cnx.dispatch (hdr, payload);
            }
          if (0 != kill (ppid, 0))
            {
              std::clog << "parent process (" << ppid << ") died." << std::endl;
              break;
            }
        }
      if (cnx.eof ())
        {
          status = EXIT_SUCCESS;
        }
    }
  catch (std::exception& e)
    {
      std::cerr << e.what () << std::endl;
    }
  catch (...)
    {
      std::cerr << "caught non-standard exception" << std::endl;
    }
  if (0 != lt_dlexit ())
    std::clog << lt_dlerror () << "\n";

  if (!output_port)             // give parent an invalid port number
    std::cout << -1 << std::endl;

  return status;
}

int (*interpreter_ctor_) (callback *, callback *);
int (*interpreter_dtor_) ();
int (*interpreter_reader_) (void *, int);
int (*interpreter_writer_) (void *, int);

template< typename T >
void get (const lt_dlhandle& handle, T& symbol, const char *name)
{
  symbol = reinterpret_cast< T > (lt_dlsym (handle, name));

  if (!symbol)
    std::clog << name << ": " << lt_dlerror () << "\n";
}

#define get_sym(handle,name) \
  get (handle, interpreter_ ## name ## _, "interpreter_" #name)

static void
load_interpreter (const std::string& argv0)
{
  std::string interpreter ("libcnx-");
  interpreter += argv0;

  lt_dladdsearchdir (std::string (PKGLIBDIR).c_str ());

  lt_dlhandle handle = lt_dlopenext (interpreter.c_str ());
  if (!handle)
    throw std::runtime_error (interpreter + ": " + lt_dlerror ());

  get_sym (handle, ctor);
  get_sym (handle, dtor);
  get_sym (handle, reader);
  get_sym (handle, writer);

  if (   interpreter_ctor_
      && interpreter_dtor_
      && interpreter_reader_
      && interpreter_writer_) return;

  if (0 != lt_dlclose (handle))
    std::clog << lt_dlerror () << "\n";

  throw std::runtime_error
    ("cannot find all required interpreter API");
}


//  Wrap signal registration platform dependencies
static void
set_signal (int sig, void (*handler) (int))
{
#define msg_failed "cannot set signal handler"
#define msg_revert "restoring default ignore behaviour for signal "

#if HAVE_SIGACTION

  struct sigaction sa;
  sa.sa_handler = handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset (&sa.sa_mask);

  struct sigaction rv;

  if (0 != sigaction (sig, &sa, &rv))
    {
      std::clog << msg_failed << sig << std::endl;
    }
  if (SIG_IGN == rv.sa_handler && SIG_IGN != handler)
    {
      // std::clog << msg_revert << sig << std::endl;
      sigaction (sig, &rv, 0);
    }

#else  /* fall back to plain C++ signal support */

  void (*rv) (int) = std::signal (sig, handler);

  if (SIG_ERR == rv)
    {
      std::clog << msg_failed << sig << std::endl;
    }
  if (SIG_IGN == rv && SIG_IGN != handler)
    {
      // std::clog << msg_revert << sig << std::endl;
      std::signal (sig, rv);
    }

#endif  /* HAVE_SIGACTION */

#undef msg_failed
#undef msg_revert
}

extern "C" {                    // signal handlers shall have C linkage

  sig_atomic_t cancel_requested = false;

  static void
  request_cancel (int sig)
  {
    cancel_requested = true;
  }

}       // extern "C"
