//  scan-cli.cpp -- command-line interface based scan utility
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_SIGACTION
#include <signal.h>
#endif

#include <csignal>
#include <cstdlib>

#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/device.hpp>
#include <utsushi/file.hpp>
#include <utsushi/format.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/monitor.hpp>
#include <utsushi/option.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/scanner.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/value.hpp>

#include "../connexions/hexdump.hpp"
#include "../filters/g3fax.hpp"
#include "../filters/jpeg.hpp"
#include "../filters/padding.hpp"
#include "../filters/pdf.hpp"
#include "../filters/pnm.hpp"
#include "../outputs/tiff.hpp"

namespace po = boost::program_options;

using namespace utsushi;

using std::invalid_argument;
using std::logic_error;
using std::runtime_error;

static sig_atomic_t do_cancel = false;

static void
request_cancellation (int sig)
{
  do_cancel = true;
}

//! Wrap signal registration platform dependencies
/*! \todo Improve error reporting to include signal and handler names
 */
static void
set_signal (int sig, void (*handler) (int))
{
  const string msg_failed
    ("cannot set signal handler (%1%)");
  const string msg_revert
    ("restoring default signal ignore behaviour (%1)");

#if HAVE_SIGACTION

  struct sigaction sa;
  sa.sa_handler = handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset (&sa.sa_mask);

  struct sigaction rv;

  if (0 != sigaction (sig, &sa, &rv))
    {
      log::error (msg_failed) % sig;
    }
  if (SIG_IGN == rv.sa_handler)
    {
      log::brief (msg_revert) % sig;
      sigaction (sig, &rv, 0);
    }

#else

  void (*rv) (int) = std::signal (sig, handler);

  if (SIG_ERR == rv)
    {
      log::error (msg_failed) % sig;
    }
  if (SIG_IGN == rv)
    {
      log::brief (msg_revert) % sig;
      std::signal (sig, rv);
    }

#endif  /* HAVE_SIGACTION */
}

//! Turn a \a udi into a scanner supported by a driver
/*! If \a debug functionality is requested, the device I/O connexion
 *  will be wrapped in a \c hexdump logger.
 */
static
scanner::ptr
create (const std::string& udi, bool debug)
{
  monitor mon;
  monitor::const_iterator it;

  if (!udi.empty ())
    {
      it = mon.find (udi);
      if (it != mon.end () && !it->has_driver ())
        {
          BOOST_THROW_EXCEPTION
            (runtime_error (_("device found but has no driver")));
        }
    }
  else
    {
      it = std::find_if (mon.begin (), mon.end (),
                         boost::bind (&scanner::id::has_driver, _1));
    }

  if (it == mon.end ())
    {
      std::string msg;

      if (!udi.empty ())
        {
          format fmt (_("cannot find '%1%'"));
          msg = (fmt % udi).str ();
        }
      else
        {
          msg = _("no devices available");
        }
      BOOST_THROW_EXCEPTION (runtime_error (msg));
    }

  connexion::ptr cnx (connexion::create (it->iftype (), it->path ()));

  if (debug)
    cnx = connexion::ptr (new _cnx_::hexdump (cnx));

  scanner::ptr rv = scanner::create (cnx, *it);

  if (rv) return rv;

  BOOST_THROW_EXCEPTION
    (runtime_error ((format (_("%1%: not supported")) % udi).str ()));
}

//! Convert a utsushi::option object into a Boost.Program_option
class option_visitor
  : public value::visitor<>
{
public:
  option_visitor (po::options_description& desc, const option& opt)
    : desc_(desc)
    , opt_(opt)
  {}

  template< typename T >
  void operator() (const T& t) const;

protected:
  po::options_description& desc_;
  const option& opt_;
};

template< typename T >
void
option_visitor::operator() (const T& t) const
{
  std::string documentation;
  std::string description;

  if (opt_.text ())             // only translate non-empty strings
    description = _(opt_.text ());

  if (opt_.constraint ())
    {
      if (!description.empty ())
        {
          documentation = (format (_("%1%\n"
                                     "Allowed values: %2%"))
                           % description
                           % *opt_.constraint ()).str ();
        }
      else
        {
          documentation = (format (_("Allowed values: %1%"))
                           % *opt_.constraint ()).str ();
        }
    }
  else
    {
      documentation = description;
    }

  desc_.add_options ()
    (opt_.key ().c_str (), po::value< T > ()->default_value (t),
     documentation.c_str ());
}

/*! \todo Add --foo / --no-foo support.  Boost.Program_options has
 *        info on supporting this kind of idiom.
 *
 *  \todo Take toggle value into account, if true --no-foo should be
 *        shown in the documentation, --foo otherwise.  We cannot just
 *        assume that all toggles start life with a value of false.
 */
template<>
void
option_visitor::operator() (const toggle& t) const
{
  std::string description;

  if (opt_.text ())             // only translate non-empty strings
    description = _(opt_.text ());

  desc_.add_options ()
    (opt_.key ().c_str (), po::bool_switch (), description.c_str ());
}

//! Dispatch visitation based upon utsushi::value
/*! When converting an utsushi::option::map, we cannot directly
 *  dispatch a visitor because we iterate over its options.  We need
 *  to pry out the option's value and pass the option as well as the
 *  options_descriptor to the visitor so it can do what it has to when
 *  applied to the value.
 *
 *  This function object does exactly that.
 */
class visit
{
public:
  visit (po::options_description& desc)
    : desc_(desc)
  {}

  void operator() (const option& opt) const
  {
    value val (opt);
    option_visitor v (desc_, opt);

    val.apply (v);
  }

protected:
  po::options_description& desc_;
};

//! Collect command-line arguments so they can be assigned all at once
/*! Setting option values one at a time may be fraught with constraint
 *  violations.  For that reason, it is safer to try setting all the
 *  values in one fell swoop.  This function object collects all the
 *  options it sees during its life-time in a value::map and tries to
 *  assign that when it goes out of scope.
 */
class assign
{
public:
  assign (option::map::ptr opts)
    : opts_(opts)
  {}

  ~assign ()
  {
    opts_->assign (vm_);
  }

  void
  operator() (const po::variables_map::value_type& kv)
  {
    /**/ if (boost::any_cast< quantity > (&kv.second.value ()))
      {
        vm_[kv.first] = kv.second.as< quantity > ();
      }
    else if (boost::any_cast< string > (&kv.second.value ()))
      {
        vm_[kv.first] = kv.second.as< string > ();
      }
    else if (boost::any_cast< bool > (&kv.second.value ()))
      {
        vm_[kv.first] = toggle (kv.second.as< bool > ());
      }
    else
      {
        format fmt (_("option parser internal inconsistency\n"
                      "  key = %1%"));
        BOOST_THROW_EXCEPTION (logic_error ((fmt % kv.first).str ()));
      }
  }

protected:
  option::map::ptr opts_;
  value::map vm_;
};

//! Reset all options after the first one that was not recognized
/*! Boost.Program_options is a bit of an eager beaver when you
 *  allow_unregistered() options.  This helper lets you reset those
 *  options that were prematurely recognized so that later passes will
 *  see them again.
 *
 *  \todo Find a more elegant kludge
 *  \note This code was copied from run-time.cpp
 */
struct unrecognize
{
  bool found_first_;

  unrecognize ()
    : found_first_(false)
  {}

  po::option
  operator() (po::option& item)
  {
    found_first_ |= item.string_key.empty ();
    found_first_ |= item.unregistered;
    item.unregistered = found_first_;

    return item;
  }
};

class cancellable_idevice
  : public decorator< idevice >
{
public:
  cancellable_idevice (idevice::ptr instance)
    : decorator< idevice > (instance)
  {}

  streamsize
  read (octet *data, streamsize n)
  {
    if (do_cancel)
      {
        instance_->cancel ();
        do_cancel = false;
      }

    return instance_->read (data, n);
  }
};

int
main (int argc, char *argv[])
{
  try
    {
      run_time rt (argc, argv, i18n);

      if (rt.count ("version"))
        {
          std::cout << rt.version ();
          return EXIT_SUCCESS;
        }

      // Positional arguments disguised as (undocumented) options
      //
      // Note that both positional arguments are optional.  This may
      // introduce a minor ambiguity if the first is not given on the
      // command-line.  The first positional argument is supposed to
      // be the device and hence should correspond to a valid UDI.  If
      // this is not the case and only a single positional argument is
      // specified, we will assume it is the output destination.

      std::string udi;
      std::string uri;

      po::options_description cmd_pos_opts;
      cmd_pos_opts
        .add_options ()
        ("UDI", (po::value< std::string > (&udi)),
         _("image acquisition device to use"))
        ("URI", (po::value< std::string > (&uri)),
         _("output destination to use"))
        ;

      po::positional_options_description cmd_pos_args;
      cmd_pos_args
        .add ("UDI", 1)
        .add ("URI", 1)
        ;

      // Self-documenting command options

      std::string fmt ("PNM");

      po::variables_map cmd_vm;
      po::options_description cmd_opts (_("Utility options"));
      cmd_opts
        .add_options ()
        ("debug", _("log device I/O in hexdump format"))
        ("image-format", (po::value< std::string > (&fmt)
                          -> default_value (fmt)),
         _("output image format\n"
           "PNM, JPEG, PDF, TIFF "
           "or one of the device supported transfer-formats"))
        ;

      po::options_description cmd_line;
      cmd_line
        .add (cmd_opts)
        .add (cmd_pos_opts)
        ;

      po::parsed_options cmd (po::command_line_parser (rt.arguments ())
                              .options (cmd_line)
                              .positional (cmd_pos_args)
                              .allow_unregistered ()
                              .run ());

      std::transform (cmd.options.begin (), cmd.options.end (),
                      cmd.options.begin (),
                      unrecognize ());

      po::store (cmd, cmd_vm);
      po::notify (cmd_vm);

      if (uri.empty () && !scanner::id::is_valid (udi))
        {
          uri = udi;
          udi.clear ();
        }

      if (rt.count ("help"))
        {
          std::cout << "\n"
                    << cmd_opts
                    << "\n";

          monitor mon;

          if (udi.empty () && mon.empty ())
            return EXIT_SUCCESS;
        }

      scanner::ptr device (create (udi, cmd_vm.count ("debug")));

      // Self-documenting device options

      po::variables_map dev_vm;
      po::options_description dev_opts (_("Device options"));
      std::for_each (device->options ()->begin (),
                     device->options ()->end (),
                     visit (dev_opts));

      if (rt.count ("help"))
        {
          std::cout << dev_opts
                    << "\n"
                    <<
            // FIXME: use word-wrapping instead of hard-coded newlines
            (_("Note: device options may be ignored if their prerequisites"
               " are not satisfied.\nA '--duplex' option may be ignored if"
               " you do not select the ADF, for example.\n"));

          return EXIT_SUCCESS;
        }

      run_time::sequence_type dev_argv;
      dev_argv = po::collect_unrecognized (cmd.options,
                                           po::exclude_positional);

      po::parsed_options dev (po::command_line_parser (dev_argv)
                              .options (dev_opts)
                              .run ());

      dev_argv = po::collect_unrecognized (dev.options,
                                           po::include_positional);

      if (uri.empty () && !dev_argv.empty ())
        {
          uri = dev_argv[0];
        }

      po::store (dev, dev_vm);
      po::notify (dev_vm);

      std::for_each (dev_vm.begin (), dev_vm.end (),
                     assign (device->options ()));

      // Infer desired image format from file extension

      if (!uri.empty () && cmd_vm["image-format"].defaulted ())
        {
          fs::path path (uri);

          /**/ if (".pnm"  == path.extension ()) fmt = "PNM";
          else if (".jpg"  == path.extension ()) fmt = "JPEG";
          else if (".jpeg" == path.extension ()) fmt = "JPEG";
          else if (".pdf"  == path.extension ()) fmt = "PDF";
          else if (".tif"  == path.extension ()) fmt = "TIFF";
          else if (".tiff" == path.extension ()) fmt = "TIFF";
          else
            log::brief
              ("cannot infer image format from destination: '%1%'") % path;
        }

      // Check whether we need to use raw image data transfer

      option::map& om (*device->options ());

      bool use_raw_transfer = false;
      use_raw_transfer |= (om["image-type"] == "Gray (1 bit)");
      use_raw_transfer |= ("PNM"  == fmt);
      use_raw_transfer |= ("TIFF" == fmt);

      if (use_raw_transfer)
        {
          om["transfer-format"] = "RAW";
        }

      // Configure the filter chain

      ostream ostr;

      /**/ if ("PNM" == fmt)
        {
          ostr.push (ofilter::ptr (new _flt_::padding));
          ostr.push (ofilter::ptr (new _flt_::pnm));
        }
      else if ("JPEG" == fmt)
        {
          if (om["image-type"] == "Gray (1 bit)")
            {
              BOOST_THROW_EXCEPTION
                (logic_error
                 (_("JPEG does not support bi-level imagery")));
            }

          if (use_raw_transfer)
            {
              ostr.push (ofilter::ptr (new _flt_::padding));
              _flt_::jpeg::ptr jpeg (new _flt_::jpeg);
              try
                {
                  (*jpeg->options ())["quality"]
                    = value (om["device/jpeg-quality"]);
                }
              catch (...)
                {}
              ostr.push (ofilter::ptr (jpeg));
            }
          else
            {
              om["transfer-format"] = "JPEG";
            }
        }
      else if ("PDF" == fmt)
        {
          if (use_raw_transfer)
            {
              ostr.push (ofilter::ptr (new _flt_::padding));
              if (om["image-type"] == "Gray (1 bit)")
                {
                  ostr.push (ofilter::ptr (new _flt_::g3fax));
                }
              else
                {
                  _flt_::jpeg::ptr jpeg (new _flt_::jpeg);
                  try
                    {
                      (*jpeg->options ())["quality"]
                        = value (om["device/jpeg-quality"]);
                    }
                  catch (...)
                    {}
                  ostr.push (ofilter::ptr (jpeg));
                }
            }
          else
            {
              om["transfer-format"] = "JPEG";
            }
          ostr.push (ofilter::ptr (new _flt_::pdf));
        }
      else if ("TIFF" == fmt)
        {
          ostr.push (ofilter::ptr (new _flt_::padding));
        }
      else
        {
          BOOST_THROW_EXCEPTION
            (invalid_argument
             (_("unsupported image-format requested")));
        }

      // Create an output device

      if (device->is_single_image ()
          || uri.empty ()
          || "PDF"  == fmt
          || "TIFF" == fmt)
        {
          if (uri.empty ()) uri = "/dev/stdout";
          if ("TIFF" == fmt)
            {
              ostr.push (odevice::ptr (new _out_::tiff_odevice (uri)));
            }
          else
            {
              ostr.push (odevice::ptr (new file_odevice (uri)));
            }
        }
      else
        {
          fs::path path (uri);
          path_generator gen (!path.parent_path ().empty ()
                              ? path.parent_path () / path.stem ()
                              : path.stem (), path.extension ().native ());

          ostr.push (odevice::ptr (new file_odevice (gen)));
        }

      idevice::ptr cancellable_device (new cancellable_idevice (device));

      istream istr;
      istr.push (cancellable_device);

      set_signal (SIGTERM, request_cancellation);
      set_signal (SIGINT , request_cancellation);

      streamsize rv = istr | ostr;

      if (traits::eof () == rv)
        log::alert ("acquisition was cancelled");
    }
  catch (std::exception& e)
    {
      std::cerr << e.what () << "\n";
      return EXIT_FAILURE;
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }

  exit (EXIT_SUCCESS);
}
