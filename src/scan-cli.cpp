//  scan-cli.cpp -- command-line interface based scan utility
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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
#include <utsushi/pump.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/scanner.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/value.hpp>

#include "../connexions/hexdump.hpp"
#include "../filters/g3fax.hpp"
#include "../filters/image-skip.hpp"
#include "../filters/jpeg.hpp"
#include "../filters/padding.hpp"
#include "../filters/pdf.hpp"
#include "../filters/pnm.hpp"
#include "../filters/threshold.hpp"
#include "../outputs/tiff.hpp"

namespace po = boost::program_options;

using namespace utsushi;
using namespace _flt_;

using std::invalid_argument;
using std::logic_error;
using std::runtime_error;

#define nullptr 0
static pump *pptr (nullptr);

static void
request_cancellation (int sig)
{
  if (pptr) pptr->cancel ();
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
    ("restoring default signal ignore behaviour (%1%)");

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
  if (SIG_IGN == rv.sa_handler && SIG_IGN != handler)
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
  if (SIG_IGN == rv && SIG_IGN != handler)
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
      if (it != mon.end () && !it->is_driver_set ())
        {
          BOOST_THROW_EXCEPTION
            (runtime_error (_("device found but has no driver")));
        }
    }
  else
    {
      it = std::find_if (mon.begin (), mon.end (),
                         boost::bind (&scanner::info::is_driver_set, _1));
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

  connexion::ptr cnx (connexion::create (it->connexion (), it->path ()));

  if (debug)
    cnx = make_shared< _cnx_::hexdump > (cnx);

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

template<>
void
option_visitor::operator() (const toggle& t) const
{
  std::string description;

  if (opt_.text ())             // only translate non-empty strings
    description = _(opt_.text ());

  std::string key (opt_.key ());
  if (t) key.insert (0, "no-");

  desc_.add_options ()
    (key.c_str (), po::bool_switch (), description.c_str ());
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
    if (opt.is_read_only ()) return;

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
    if (kv.second.defaulted ()) return;

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
        std::string key (kv.first);

        if ("no-" != key.substr (0, 3))
          {
            vm_[key] = toggle (kv.second.as< bool > ());
          }
        else
          {
            key.erase (0, 3);
            vm_[key] = toggle (!kv.second.as< bool > ());
          }
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

static int status = EXIT_SUCCESS;

void
on_notify (log::priority level, std::string message)
{
  std::cerr << message << '\n';

  if (log::ERROR >= level) status = EXIT_FAILURE;
}

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

      std::string fmt;

      po::variables_map cmd_vm;
      po::options_description cmd_opts (_("Utility options"));
      cmd_opts
        .add_options ()
        ("debug", _("log device I/O in hexdump format"))
        ("image-format", (po::value< std::string > (&fmt)),
         _("output image format\n"
           "PNM, JPEG, PDF, TIFF "
           "or one of the device supported transfer-formats.  "
           "The explicitly mentioned types are normally inferred from"
           " the output file name."))
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

      if (uri.empty () && !scanner::info::is_valid (udi))
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

      // Self-documenting device and add-on options

      po::variables_map dev_vm;
      po::options_description dev_opts (_("Device options"));
      std::for_each (device->options ()->begin (),
                     device->options ()->end (),
                     visit (dev_opts));

      po::variables_map add_vm;
      po::options_description add_opts (_("Add-on options"));
      ofilter::ptr blank_skip = make_shared< _flt_::image_skip > ();
      std::for_each (blank_skip->options ()->begin (),
                     blank_skip->options ()->end (),
                     visit (add_opts));

      if (rt.count ("help"))
        {
          std::cout << dev_opts
                    << add_opts
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
                              .allow_unregistered ()
                              .run ());

      dev_argv = po::collect_unrecognized (dev.options,
                                           po::include_positional);

      po::parsed_options add (po::command_line_parser (dev_argv)
                              .options (add_opts)
                              .run ());

      dev_argv = po::collect_unrecognized (add.options,
                                           po::include_positional);

      if (uri.empty () && !dev_argv.empty ())
        {
          uri = dev_argv[0];
        }

      po::store (dev, dev_vm);
      po::notify (dev_vm);

      std::for_each (dev_vm.begin (), dev_vm.end (),
                     assign (device->options ()));

      po::store (add, add_vm);
      po::notify (add_vm);

      std::for_each (add_vm.begin (), add_vm.end (),
                     assign (blank_skip->options ()));

      // Infer desired image format from file extension

      if (!uri.empty ())
        {
          fs::path path (uri);

          /**/ if (".pnm"  == path.extension ()) fmt = "PNM";
          else if (".jpg"  == path.extension ()) fmt = "JPEG";
          else if (".jpeg" == path.extension ()) fmt = "JPEG";
          else if (".pdf"  == path.extension ()) fmt = "PDF";
          else if (".tif"  == path.extension ()) fmt = "TIFF";
          else if (".tiff" == path.extension ()) fmt = "TIFF";
          else
            log::alert
              ("cannot infer image format from destination: '%1%'") % path;
        }

      // Configure the filter chain

      option::map& om (*device->options ());
      ostream::ptr ostr = make_shared< ostream > ();

      const std::string xfer_raw = "image/x-raster";
      const std::string xfer_jpg = "image/jpeg";
      std::string xfer_fmt = device->get_context ().content_type ();

      bool bilevel = (om["image-type"] == "Gray (1 bit)");
      ofilter::ptr threshold (make_shared< threshold > ());
      try
        {
          (*threshold->options ())["threshold"]
            = value (om["threshold"]);
        }
      catch (const std::out_of_range&)
        {
          log::error ("Falling back to default threshold value");
        }

      ofilter::ptr jpeg_compress (make_shared< jpeg::compressor > ());
      try
        {
          (*jpeg_compress->options ())["quality"]
            = value ((om)["jpeg-quality"]);
        }
      catch (const std::out_of_range&)
        {
          log::error ("Falling back to default JPEG compression quality");
        }

      toggle match_height = true;
      quantity height = -1.0;
      try
        {
          match_height = value (om["match-height"]);
          height  = value (om["br-y"]);
          height -= value (om["tl-y"]);
        }
      catch (const std::out_of_range&)
        {
          match_height = false;
          height = -1.0;
        }
      if (match_height) match_height = (height > 0);

      toggle skip_blank = (add_vm.count ("blank-threshold")
                           && !bilevel); // \todo fix filter limitation

      /**/ if (xfer_raw == xfer_fmt) {}
      else if (xfer_jpg == xfer_fmt) {}
      else
        {
          log::alert
            ("unsupported transfer format: '%1%'") % xfer_fmt;
        }

      /**/ if ("PNM" == fmt)
        {
          /**/ if (xfer_raw == xfer_fmt)
            ostr->push (make_shared< padding > ());
          else if (xfer_jpg == xfer_fmt)
            {
              ostr->push (make_shared< jpeg::decompressor > ());
              if (skip_blank) ostr->push (blank_skip);
              if (bilevel) ostr->push (threshold);
            }
          else
            BOOST_THROW_EXCEPTION
              (runtime_error
               ((format (_("conversion from %1% to %2% is not supported"))
                 % xfer_fmt
                 % fmt)
                .str ()));
          if (skip_blank) ostr->push (blank_skip);
          if (match_height)
            ostr->push (make_shared< bottom_padder > (height));
          ostr->push (make_shared< pnm > ());
        }
      else if ("JPEG" == fmt)
        {
          if (bilevel)
            BOOST_THROW_EXCEPTION
              (logic_error
               (_("JPEG does not support bi-level imagery")));

          /**/ if (xfer_raw == xfer_fmt)
            {
              ostr->push (make_shared< padding > ());
              if (skip_blank) ostr->push (blank_skip);
              if (match_height)
                ostr->push (make_shared< bottom_padder > (height));
              ostr->push (jpeg_compress);
            }
          else if (xfer_jpg == xfer_fmt)
            {
              if (match_height || skip_blank)
                {
                  ostr->push (make_shared< jpeg::decompressor > ());
                  if (skip_blank)   ostr->push (blank_skip);
                  if (match_height) ostr->push (make_shared< bottom_padder >
                                                (height));
                  ostr->push (jpeg_compress);
                }
            }
          else
            {
              BOOST_THROW_EXCEPTION
                (runtime_error
                 ((format (_("conversion from %1% to %2% is not supported"))
                   % xfer_fmt
                   % fmt)
                  .str ()));
            }
        }
      else if ("PDF" == fmt)
        {
          /**/ if (xfer_raw == xfer_fmt)
            {
              ostr->push (make_shared< padding > ());
              if (skip_blank) ostr->push (blank_skip);
              if (match_height)
                ostr->push (make_shared< bottom_padder > (height));

              if (bilevel)
                {
                  ostr->push (make_shared< g3fax > ());
                }
              else
                {
                  ostr->push (jpeg_compress);
                }
            }
          else if (xfer_jpg == xfer_fmt)
            {
              if (match_height || bilevel)
                {
                  ostr->push (make_shared< jpeg::decompressor > ());
                  if (skip_blank) ostr->push (blank_skip);
                }
              if (match_height)
                {
                  ostr->push (make_shared< bottom_padder > (height));
                }
              if (bilevel)
                {
                  ostr->push (threshold);
                  ostr->push (make_shared< g3fax > ());
                }
              else
                {
                  ostr->push (jpeg_compress);
                }
            }
          else
            {
              BOOST_THROW_EXCEPTION
                (runtime_error
                 ((format (_("conversion from %1% to %2% is not supported"))
                   % xfer_fmt
                   % fmt)
                  .str ()));
            }
          ostr->push (make_shared< pdf > ());
        }
      else if ("TIFF" == fmt)
        {
          /**/ if (xfer_raw == xfer_fmt)
            ostr->push (make_shared< padding > ());
          else if (xfer_jpg == xfer_fmt)
             {
               ostr->push (make_shared< jpeg::decompressor > ());
               if (bilevel) ostr->push (threshold);
             }
          else
            BOOST_THROW_EXCEPTION
              (runtime_error
               ((format (_("conversion from %1% to %2% is not supported"))
                 % xfer_fmt
                 % fmt)
                .str ()));
          if (skip_blank) ostr->push (blank_skip);
          if (match_height)
            ostr->push (make_shared< bottom_padder > (height));
        }
      else
        {
          log::brief
            ("unsupported image format requested, passing data as is");
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
              ostr->push (make_shared< _out_::tiff_odevice > (uri));
            }
          else
            {
              ostr->push (make_shared< file_odevice > (uri));
            }
        }
      else
        {
          fs::path path (uri);
          path_generator gen (!path.parent_path ().empty ()
                              ? path.parent_path () / path.stem ()
                              : path.stem (), path.extension ().native ());

          ostr->push (make_shared< file_odevice > (gen));
        }

      pump p (device);
      pptr = &p;                // for use in request_cancellation

      set_signal (SIGTERM, request_cancellation);
      set_signal (SIGINT , request_cancellation);

      p.connect (on_notify);
      p.start (ostr);
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

  exit (status);
}
