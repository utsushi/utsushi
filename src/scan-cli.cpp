//  scan-cli.cpp -- command-line interface based scan utility
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2013, 2015  Olaf Meeuwissen
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
#include <utility>
#include <vector>
#include <set>

#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/device.hpp>
#include <utsushi/file.hpp>
#include <utsushi/format.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/memory.hpp>
#include <utsushi/monitor.hpp>
#include <utsushi/option.hpp>
#include <utsushi/pump.hpp>
#include <utsushi/range.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/scanner.hpp>
#include <utsushi/store.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/value.hpp>

#include "../filters/autocrop.hpp"
#include "../filters/deskew.hpp"
#include "../filters/g3fax.hpp"
#include "../filters/image-skip.hpp"
#if HAVE_LIBJPEG
#include "../filters/jpeg.hpp"
#endif
#include "../filters/padding.hpp"
#include "../filters/pdf.hpp"
#include "../filters/pnm.hpp"
#include "../filters/magick.hpp"
#include "../filters/reorient.hpp"
#if HAVE_LIBTIFF
#include "../outputs/tiff.hpp"
#endif

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace utsushi;
using namespace _flt_;

using std::invalid_argument;
using std::logic_error;
using std::runtime_error;

static int status = EXIT_SUCCESS;

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
  monitor::const_iterator it (mon.find (udi));

  if (it == mon.end ())
    {
      if (!udi.empty ())
        {
          BOOST_THROW_EXCEPTION
            (runtime_error ((format (CCB_("%1%: not found")) % udi).str ()));
        }
      else
        {
          BOOST_THROW_EXCEPTION
            (runtime_error (CCB_("no usable devices available")));
        }
    }

  if (!it->is_driver_set ())
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ((format (CCB_("%1%: found but has no driver"))
                         % udi).str ()));
    }

  scanner::info info (*it);
  info.enable_debug (debug);

  scanner::ptr rv = scanner::create (info);

  if (rv) return rv;

  BOOST_THROW_EXCEPTION
    (runtime_error ((format (CCB_("%1%: not supported")) % udi).str ()));
}

//! Convert a utsushi::option object into a Boost.Program_option
class option_visitor
  : public value::visitor<>
{
public:
  option_visitor (po::options_description& desc, const option& opt,
                  const std::set<std::string>& option_blacklist,
                  boost::optional< toggle > resampling = boost::none)
    : desc_(desc)
    , opt_(opt)
    , resampling_(resampling)
    , option_blacklist_(option_blacklist)
  {}

  template< typename T >
  void operator() (const T& t) const;

protected:
  po::options_description& desc_;
  const option& opt_;
  boost::optional< toggle > resampling_;
  const std::set<std::string>& option_blacklist_;
};

template< typename T >
void
option_visitor::operator() (const T& t) const
{
  std::string documentation;
  std::string description;
  std::string key (opt_.key ());

  if (resampling_ && *resampling_)
    {
      if (key.substr (0,10) == "resolution") return;
      if (key.substr (0, 3) == "sw-") key.erase (0, 3);
    }

  if (option_blacklist_.count (key)) return;

  if (opt_.text ())             // only translate non-empty strings
    description = _(opt_.text ());

  if (opt_.constraint ())
    {
      if (!description.empty ())
        {
          documentation = (format (CCB_("%1%\n"
                                        "Allowed values: %2%"))
                           % description
                           % *opt_.constraint ()).str ();
        }
      else
        {
          documentation = (format (CCB_("Allowed values: %1%"))
                           % *opt_.constraint ()).str ();
        }
    }
  else
    {
      documentation = description;
    }

  desc_.add_options ()
    (key.c_str (), po::value< T > ()->default_value (t),
     documentation.c_str ());
}

template<>
void
option_visitor::operator() (const toggle& t) const
{
  std::string description;
  std::string key (opt_.key ());

  if (resampling_ && *resampling_)
    {
      if (key.substr (0,10) == "resolution") return;
      if (key.substr (0, 3) == "sw-") key.erase (0, 3);
    }

  if (option_blacklist_.count (key)) return;

  if (opt_.text ())             // only translate non-empty strings
    description = _(opt_.text ());

  if (t) key.insert (0, "no-");

  desc_.add_options ()
    (key.c_str (), po::bool_switch (), description.c_str ());
}

template<>
void
option_visitor::operator() (const value::none&) const
{
  std::string description;
  std::string key (opt_.key ());

  if (opt_.text ())             // only translate non-empty strings
    description = _(opt_.text ());

  desc_.add_options ()
    (key.c_str (), description.c_str ());
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
  visit (po::options_description& desc,
         const std::set<std::string>& option_blacklist = std::set<std::string>(),
         boost::optional< toggle > resampling = boost::none)
    : desc_(desc)
    , resampling_(resampling)
    , option_blacklist_(option_blacklist)
  {}

  void operator() (const option& opt) const
  {
    if (opt.is_read_only ())
      return;

    value val (opt);
    option_visitor v (desc_, opt, option_blacklist_, resampling_);

    val.apply (v);
  }

protected:
  po::options_description& desc_;
  boost::optional< toggle > resampling_;
  const std::set<std::string>& option_blacklist_;
};

class run
{
public:
  run (option::map::ptr acts)
    : acts_(acts)
  {}

  void
  operator() (const po::variables_map::value_type& kv)
  {
    if ("dont-scan" == kv.first) return;

    result_code rc = (*acts_)[kv.first].run ();
    if (rc)
      {
        std::cerr << rc.message () << "\n";
        status = EXIT_FAILURE;
      }
  }

  option::map::ptr acts_;
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
    if (opts_->count ("enable-resampling"))
      {
        toggle t = value ((*opts_)["enable-resampling"]);
        if (vm_.count ("enable-resampling"))
          t = vm_["enable-resampling"];

        if (t)
          {
            replace ("resolution");
            replace ("resolution-x");
            replace ("resolution-y");
            replace ("resolution-bind");
          }
      }

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
    else if (boost::any_cast< std::string > (&kv.second.value ()))
      {
        vm_[kv.first] = kv.second.as< std::string > ();
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
        format fmt ("option parser internal inconsistency\n"
                    "  key = %1%");
        BOOST_THROW_EXCEPTION (logic_error ((fmt % kv.first).str ()));
      }
  }

protected:
  void replace (const std::string& k)
  {
    value::map::iterator it (vm_.find (k));

    if (vm_.end () == it) return;

    vm_["sw-" + k] = it->second;
    vm_.erase (it);
  }

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
         CCB_("image acquisition device to use"))
        ("URI", (po::value< std::string > (&uri)),
         CCB_("output destination to use"))
        ;

      po::positional_options_description cmd_pos_args;
      cmd_pos_args
        .add ("UDI", 1)
        .add ("URI", 1)
        ;

      // Self-documenting command options

      std::string fmt;

      po::variables_map cmd_vm;
      po::options_description cmd_opts (CCB_("Utility options"));
      cmd_opts
        .add_options ()
        ("debug", CCB_("log device I/O in hexdump format"))
        ("image-format", (po::value< std::string > (&fmt)
                          ->default_value ("PNM")),
         CCB_("output image format\n"
              "PNM, PNG, JPEG, PDF, TIFF "
              "or one of the device supported transfer-formats.  "
              "The explicitly mentioned types are normally inferred from"
              " the output file name.  Some require additional libraries"
              " at build-time in order to be available."))
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

      if (udi.empty ())
        {
          monitor mon;
          udi = mon.default_device ();
        }

      if (rt.count ("help"))
        {
          // FIXME clarify the command-line API
          // FIXME explain %-escape pattern usage
          std::cout << "\n"
                    << cmd_opts
                    << "\n";

          if (udi.empty ())
            return EXIT_SUCCESS;
        }

      if (udi.empty ())
        {
          std::cerr << CCB_("no usable devices available");
          return EXIT_FAILURE;
        }

      scanner::ptr device (create (udi, cmd_vm.count ("debug")));

      // Self-documenting device and add-on options

      po::variables_map act_vm;
      po::options_description dev_acts (CCB_("Device actions"));

      std::for_each (device->actions ()->begin (),
                     device->actions ()->end (),
                     visit (dev_acts));

      if (!device->actions ()->empty ())
        {
          dev_acts
            .add_options ()
            ("dont-scan", po::bool_switch (),
             CCB_("Only perform the actions given on the command-line."
                  "  Do not perform image acquisition."))
            ;
        }

      po::variables_map dev_vm;
      po::options_description dev_opts (CCB_("Device options"));
      po::variables_map add_vm;
      po::options_description add_opts (CCB_("Add-on options"));

      boost::optional< toggle > resampling;
      if (device->options ()->count ("enable-resampling"))
        {
          toggle t = value ((*device->options ())["enable-resampling"]);
          resampling = t;
        }

      bool emulating_automatic_scan_area = false;

      if (HAVE_MAGICK_PP
          && device->options ()->count ("lo-threshold")
          && device->options ()->count ("hi-threshold"))
        {
          if (device->options ()->count ("scan-area"))
            {
              constraint::ptr c ((*device->options ())["scan-area"]
                                 .constraint ());
              if (value ("Auto Detect") != (*c) (value ("Auto Detect")))
                {
                  dynamic_pointer_cast< utsushi::store >
                    (c)->alternative ("Auto Detect");
                  emulating_automatic_scan_area = true;
                }
            }

          if (!device->options ()->count ("deskew"))
            {
              add_opts
                .add_options ()
                ("deskew", po::bool_switch (),
                 SEC_N_("Deskew"))
                ;
            }
        }

      filter::ptr magick;
      std::set<std::string> option_blacklist;
      if (HAVE_MAGICK)
        {
          magick = make_shared< _flt_::magick > ();
        }

      filter::ptr reorient;
      if (magick)
        {
          visit v (add_opts);
          option::map om;

          om.add_options ()
            ("image-type", (from< utsushi::store > ()
                            -> alternative (SEC_N_("Monochrome"))
                            -> alternative (SEC_N_("Grayscale"))
                            -> default_value (SEC_N_("Color"))),
             attributes (tag::general)(level::standard),
             SEC_N_("Image Type"))
            ;
          v (om ["image-type"]);
          option_blacklist.insert ("image-type");

          v ((*magick->options ())["threshold"]);
          option_blacklist.insert ("threshold");
          v ((*magick->options ())["brightness"]);
          v ((*magick->options ())["contrast"]);

          if (magick->options ()->count ("auto-orient"))
            {
              reorient = make_shared< _flt_::reorient > ();
              std::for_each (reorient->options ()->begin (),
                             reorient->options ()->end (),
                             visit (add_opts));
            }
        }

      filter::ptr blank_skip = make_shared< _flt_::image_skip > ();
      std::for_each (blank_skip->options ()->begin (),
                     blank_skip->options ()->end (),
                     visit (add_opts));

      std::for_each (device->options ()->begin (),
                     device->options ()->end (),
                     visit (dev_opts, option_blacklist, resampling));

      if (rt.count ("help"))
        {
          if (!device->actions ()->empty ())
            std::cout << dev_acts;

          std::cout << dev_opts
                    << add_opts
                    << "\n"
                    <<
            // FIXME: use word-wrapping instead of hard-coded newlines
            (CCB_("Note: device options may be ignored if their prerequisites"
                  " are not satisfied.\nA '--duplex' option may be ignored if"
                  " you do not select the ADF, for example.\n"));

          return EXIT_SUCCESS;
        }

      run_time::sequence_type dev_argv;
      dev_argv = po::collect_unrecognized (cmd.options,
                                           po::exclude_positional);

      po::parsed_options act (po::command_line_parser (dev_argv)
                              .options (dev_acts)
                              .allow_unregistered ()
                              .run ());

      dev_argv = po::collect_unrecognized (act.options,
                                           po::include_positional);

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

      po::store (act, act_vm);
      po::notify (act_vm);

      po::store (dev, dev_vm);
      po::notify (dev_vm);

      po::store (add, add_vm);
      po::notify (add_vm);

      // ...
      if (dev_vm.count ("long-paper-mode")
          && add_vm.count ("deskew"))
        {
          if (dev_vm["long-paper-mode"].as < bool > ()
              && add_vm["deskew"].as < bool > ())
            {
              BOOST_THROW_EXCEPTION
                (constraint::violation ("value combination not acceptable"));
            }
        }
      // Pick off those options and option values that need special
      // handling

      bool long_paper_mode (dev_vm.count ("long-paper-mode")
                            && dev_vm["long-paper-mode"].as < bool > ());

      if (dev_vm.count ("doc-source"))
        {
          long_paper_mode &=
            (dev_vm["doc-source"].as< string > () == "ADF");
        }

      filter::ptr autocrop;
      if (HAVE_MAGICK_PP
          && (emulating_automatic_scan_area || long_paper_mode)
          && (dev_vm["scan-area"].as< string > () == "Auto Detect"))
        {
          autocrop = make_shared< _flt_::autocrop > ();
          dev_vm.erase ("scan-area");
          po::variable_value v (std::string ("Maximum"), false);
          dev_vm.insert (std::make_pair ("scan-area", v));

          if (device->options ()->count ("auto-kludge"))
            (*device->options ())["auto-kludge"] = toggle (long_paper_mode);

          if (dev_vm.count ("overscan"))
            {
              dev_vm.erase ("overscan");
              po::variable_value v (true, false);
              dev_vm.insert (std::make_pair ("overscan", v));
            }
        }

      filter::ptr deskew;
      if (HAVE_MAGICK_PP
          && add_vm.count ("deskew"))
        {
          if (!autocrop         // autocrop already deskews
              && !long_paper_mode
              && add_vm["deskew"].as< bool > ())
            deskew = make_shared< _flt_::deskew > ();
          add_vm.erase ("deskew");
        }

      if (HAVE_MAGICK
          && add_vm.count ("rotate"))
        {
          value angle = add_vm["rotate"].as< string > ();
          (*reorient->options ())["rotate"] = angle;
          add_vm.erase ("rotate");
        }

      quantity threshold;
      quantity brightness;
      quantity contrast;
      bool bilevel = false;
      if (magick)
        {
          threshold = add_vm["threshold"].as< quantity > ();
          add_vm.erase ("threshold");
          brightness = add_vm["brightness"].as< quantity > ();
          add_vm.erase ("brightness");
          contrast   = add_vm["contrast"].as< quantity > ();
          add_vm.erase ("contrast");

          std::string type (add_vm["image-type"].as< string > ());
          bilevel = (type == "Monochrome");
          if (bilevel)              // use software thresholding
            {
              type = "Grayscale";
            }
          po::variable_value v (type, false);
          dev_vm.insert (std::make_pair ("image-type", v));
          add_vm.erase ("image-type");
        }
      else
        {
          if (dev_vm.count ("image-type"))
            {
              bilevel = (dev_vm["image-type"].as< string > () == "Monochrome");
            }
        }

      // Push all options to their respective providers

      std::for_each (act_vm.begin (), act_vm.end (),
                     run (device->actions ()));

      std::for_each (dev_vm.begin (), dev_vm.end (),
                     assign (device->options ()));

      std::for_each (add_vm.begin (), add_vm.end (),
                     assign (blank_skip->options ()));

      if (act_vm.count ("dont-scan")
          && !act_vm["dont-scan"].defaulted ())
        return status;

      // Determine the requested image format

      if (cmd_vm["image-format"].defaulted () && !uri.empty ())
        {
          fs::path path (uri);

          /**/ if (".pnm"  == path.extension ()) fmt = "PNM";
          else if (HAVE_MAGICK  && ".png"  == path.extension ()) fmt = "PNG";
          else if (HAVE_LIBJPEG && ".jpg"  == path.extension ()) fmt = "JPEG";
          else if (HAVE_LIBJPEG && ".jpeg" == path.extension ()) fmt = "JPEG";
          else if (".pdf"  == path.extension ()) fmt = "PDF";
          else if (HAVE_LIBTIFF && ".tif"  == path.extension ()) fmt = "TIFF";
          else if (HAVE_LIBTIFF && ".tiff" == path.extension ()) fmt = "TIFF";
          else
            {
              std::cerr <<
                format (CCB_("cannot infer image format from file"
                             " extension: '%1%'"))
                % path.extension ()
                ;
              return EXIT_FAILURE;
            }
        }

      // Check whether the requested image format is supported

      fs::path ext;

      /**/ if ("PNM"  == fmt) ext = ".pnm";
      else if (HAVE_MAGICK  && "PNG"  == fmt) ext = ".png";
      else if (HAVE_LIBJPEG && "JPEG" == fmt) ext = ".jpeg";
      else if ("PDF"  == fmt) ext = ".pdf";
      else if (HAVE_LIBTIFF && "TIFF" == fmt) ext = ".tiff";
      else if ("ASIS" == fmt) ;        // for troubleshooting purposes
      else
        {
          std::cerr <<
            format (CCB_("unsupported image format: '%1%'"))
            % fmt
            ;
          return EXIT_FAILURE;
        }

      if (!uri.empty () && !ext.empty ())
        {
          fs::path path (uri);

          if (ext != path.extension ())
            {
              /**/ if (HAVE_LIBJPEG && "JPEG" == fmt && ".jpg" == path.extension ());
              else if (HAVE_LIBTIFF && "TIFF" == fmt && ".tif" == path.extension ());
              else
                {
                  log::alert
                    ("uncommon file extension for %1% image format: '%2%'")
                    % fmt
                    % ext
                    ;
                }
            }
        }

      const std::string stdout ("/dev/stdout");

      if (uri.empty ()) uri = stdout;

      path_generator gen (uri);

      // TODO add (optional) overwrite checking

      // Create an output device

      odevice::ptr odev;

      if (!gen)                 // single file (or standard output)
        {
#if HAVE_LIBTIFF
          /**/ if ("TIFF" == fmt)
            {
              odev = make_shared< _out_::tiff_odevice > (uri);
            }
          else
#endif
          /**/ if ("PDF" == fmt
                   || stdout == uri
                   || device->is_single_image ())
            {
              odev = make_shared< file_odevice > (uri);
            }
          else
            {
              std::cerr <<
                format (CCB_("%1% does not support multi-image files"))
                % fmt
                ;
              return EXIT_FAILURE;
            }
        }
      else                      // file per image
        {
#if HAVE_LIBTIFF
          if ("TIFF" == fmt)
            {
              odev = make_shared< _out_::tiff_odevice > (gen);
            }
          else
#endif
            {
              odev = make_shared< file_odevice > (gen);
            }
        }

      // Configure the filter chain

      option::map& om (*device->options ());
      stream::ptr str = make_shared< stream > ();

      const std::string xfer_raw = "image/x-raster";
      const std::string xfer_jpg = "image/jpeg";
      std::string xfer_fmt = device->get_context ().content_type ();

      toggle force_extent = true;
      quantity width  = -1.0;
      quantity height = -1.0;
      try
        {
          force_extent = value (om["force-extent"]);
          width   = value (om["br-x"]);
          width  -= value (om["tl-x"]);
          height  = value (om["br-y"]);
          height -= value (om["tl-y"]);
        }
      catch (const std::out_of_range&)
        {
          force_extent = false;
          width  = -1.0;
          height = -1.0;
        }
      if (force_extent) force_extent = (width > 0 || height > 0);

      /* autocrop has been instantiated earlier if necessary
       * It is not done here due to command-line option handling.
       */
      if (autocrop)     force_extent = false;

      if (autocrop)
        {
          if (long_paper_mode)
            {
              (*autocrop->options ())["trim"] = toggle (long_paper_mode);
            }
          else
            {
              (*autocrop->options ())["lo-threshold"]
                = value (om["lo-threshold"]);
              (*autocrop->options ())["hi-threshold"]
                = value (om["hi-threshold"]);
            }
        }

      /* deskew has been instantiated earlier if necessary
       * It is not done here due to command-line option handling.
       */

      if (deskew)
        {
          (*deskew->options ())["lo-threshold"] = value (om["lo-threshold"]);
          (*deskew->options ())["hi-threshold"] = value (om["hi-threshold"]);
        }

      toggle resample = false;
      if (om.count ("enable-resampling"))
        resample = value (om["enable-resampling"]);

      if (magick)
        {
          if (reorient)
            {
              (*magick->options ())["auto-orient"] = toggle (true);
            }
        }

      if (magick)
        {
          toggle bound = true;
          quantity res_x  = -1.0;
          quantity res_y  = -1.0;

          std::string sw (resample ? "sw-" : "");
          if (om.count (sw + "resolution-x"))
            {
              res_x = value (om[sw + "resolution-x"]);
              res_y = value (om[sw + "resolution-y"]);
            }
          if (om.count (sw + "resolution-bind"))
            bound = value (om[sw + "resolution-bind"]);

          if (bound)
            {
              res_x = value (om[sw + "resolution"]);
              res_y = value (om[sw + "resolution"]);
            }

          (*magick->options ())["resolution-x"] = res_x;
          (*magick->options ())["resolution-y"] = res_y;
          (*magick->options ())["force-extent"] = force_extent;
          (*magick->options ())["width"]  = width;
          (*magick->options ())["height"] = height;

          (*magick->options ())["bilevel"] = toggle (bilevel);

          (*magick->options ())["threshold"] = threshold;
          (*magick->options ())["brightness"] = brightness;
          (*magick->options ())["contrast"]   = contrast;

          if ("ASIS" != fmt)
            (*magick->options ())["image-format"] = fmt;
      }

      {
        toggle sw_color_correction = false;
        if (om.count ("sw-color-correction"))
          {
            sw_color_correction = value (om["sw-color-correction"]);
            for (int i = 1; sw_color_correction && i <= 9; ++i)
              {
                key k ("cct-" + boost::lexical_cast< std::string > (i));
                (*magick->options ())[k] = value (om[k]);
              }
          }
        (*magick->options ())["color-correction"] = sw_color_correction;
      }

      toggle skip_blank = !bilevel; // \todo fix filter limitation
      quantity skip_thresh = -1.0;
      /* blank_skip generated earlier due to command-line handling issues */
      try
        {
          /* blank_skip options already set thru command-line handling */
          skip_thresh = value ((*blank_skip->options ())["blank-threshold"]);
        }
      catch (const std::out_of_range&)
        {
          skip_blank = false;
          log::error ("Disabling blank skip functionality");
        }
      // Don't even try skipping of completely white images.  We are
      // extremely unlikely to encounter any of those.
      skip_blank = (skip_blank
                    && (quantity (0.) < skip_thresh));


      /**/ if ("ASIS" != fmt)
        {

          /**/ if (xfer_raw == xfer_fmt)
            {
              str->push (make_shared< padding > ());
            }
#if HAVE_LIBJPEG
          else if (xfer_jpg == xfer_fmt)
            {
              str->push (make_shared< jpeg::decompressor > ());
            }
#endif
          else
            {
              log::alert
                ("unsupported transfer format: '%1%'") % xfer_fmt;

              BOOST_THROW_EXCEPTION
                (runtime_error
                 ((format (SEC_("conversion from %1% to %2% is not supported"))
                   % xfer_fmt
                   % fmt)
                  .str ()));
            }

          if (skip_blank)  str->push (blank_skip);
          str->push (make_shared< pnm > ());
          if (autocrop)    str->push (autocrop);
          if (deskew)      str->push (deskew);
          if (reorient)    str->push (reorient);
          if (magick)      str->push (magick);

          if ("PDF" == fmt)
            {
              if (bilevel) str->push (make_shared< g3fax > ());
              str->push (make_shared< pdf > (gen));
            }
        }
      else
        {
          log::brief
            ("as-is image format requested, not applying any filters");
        }

      str->push (odev);
      pump p (device);
      pptr = &p;                // for use in request_cancellation

      set_signal (SIGTERM, request_cancellation);
      set_signal (SIGINT , request_cancellation);
      set_signal (SIGPIPE, request_cancellation);
      set_signal (SIGHUP , request_cancellation);

      p.connect (on_notify);
      p.start (str);
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
