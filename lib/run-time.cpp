//  run-time.cpp -- information for a program
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#include <cstdlib>
#include <ltdl.h>

#include <algorithm>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>

#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"
#include "utsushi/regex.hpp"

#include "run-time.ipp"

#define DEFAULT_SHELL "/bin/sh"

namespace utsushi {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using std::logic_error;
using std::runtime_error;

run_time::impl *run_time::impl::instance_(0);

const std::string run_time::impl::libexec_prefix_(PACKAGE_TARNAME "-");
const std::string run_time::impl::libtool_prefix_("lt-");

run_time::run_time (int argc, const char *const argv[], bool configure_i18n)
{
  if (impl::instance_)
    BOOST_THROW_EXCEPTION
      (logic_error ("run_time has been initialized already"));

  if (configure_i18n)
    {
      const char *dirname = getenv (PACKAGE_ENV_VAR_PREFIX "LOCALEDIR");

      if (!dirname) dirname = LOCALEDIR;

      setlocale (LC_ALL, "");
      bindtextdomain (dirname);
      textdomain ();
    }

  impl::instance_ = new impl (argc, argv);
}

run_time::run_time ()
{
  if (!impl::instance_)
    BOOST_THROW_EXCEPTION
      (logic_error ("run_time has not been initialized yet"));
}

std::string
run_time::program () const
{
  return PACKAGE_TARNAME;
}

std::string
run_time::command () const
{
  return impl::instance_->command_;
}

const run_time::sequence_type&
run_time::arguments () const
{
  return impl::instance_->cmd_args_;
}

std::string
run_time::locate (const std::string& command) const
{
  fs::path rv;

  if (!running_in_place ())
    {
      rv = fs::path (PKGLIBEXECDIR) / impl::libexec_prefix_;
      rv = rv.native () + command;
    }
  else
    {
      fs::path path (impl::instance_->argzero_.parent_path ());

      if (fs::path (LT_OBJDIR).parent_path () == path.filename ())
        {
          path = path.parent_path ();
        }
      rv = path / command;
    }
  rv = rv.native () + impl::instance_->argzero_.extension ().native ();

  if (!fs::exists (rv))
    log::trace ("%1%: no such file") % rv.string ();

  return rv.string ();
}

void
run_time::execute (const std::string& shell_command) const
{
  execl (impl::instance_->shell_.c_str (),
         impl::instance_->shell_.c_str (),
         "-c",
         shell_command.c_str (),
         NULL);

  int err_code = errno;
  BOOST_THROW_EXCEPTION (runtime_error (strerror (err_code)));
}

po::variables_map::size_type
run_time::count (const po::variables_map::key_type& option) const
{
  return impl::instance_->vm_.count (option);
}

const po::variable_value&
run_time::operator[] (const std::string& option) const
{
  return impl::instance_->vm_[option];
}

std::string
run_time::help (const std::string& summary) const
{
  format fmt (!command ().empty ()
              ? "%1% %2% -- %3%\n"
              : "%1% -- %3%\n");
  return (fmt
          % program ()
          % command ()
          % summary).str ();
}

std::string
run_time::version (const std::string& legalese,
                   const std::string& disclaimer) const
{
  // This string should NOT be translated
  static const std::string default_legalese
    ("Copyright (C) 2012-2015  SEIKO EPSON CORPORATION\n"
     "License: GPL-3.0+");

  format fmt (!command ().empty ()
              ? "%1% %2% (%3%) %4%\n%5%\n%6%\n"
              : "%1% (%3%) %4%\n%5%\n%6%\n");
  return (fmt
          % program ()
          % command ()
          % PACKAGE_NAME
          % PACKAGE_VERSION
          % (legalese.empty ()
             ? default_legalese
             : legalese)
          % disclaimer).str ();
}

run_time::sequence_type
run_time::load_dirs (scope s, const std::string& component) const
{
  sequence_type rv;

  if (!running_in_place ())
    {
      /**/ if (pkg == s)
        {
          rv.push_back (fs::path (PKGLIBDIR).string ());
        }
      else
        {
          log::alert ("unsupported scope: %1%") % s;
        }
    }
  else
    {
      /**/ if ("driver" == component)
        {
          rv.push_back ((impl::instance_->top_builddir_
                         / "drivers").string ());
          rv.push_back ((impl::instance_->top_builddir_
                         / "drivers" / "esci").string ());
        }
      else
        {
          log::alert ("unsupported component: %1%") % component;
        }
    }

  return rv;
}

std::string
run_time::data_file (scope s, const std::string& name) const
{
  fs::path rv;

  if (!running_in_place ())
    {
      /**/ if (pkg == s)
        {
          rv = fs::path (PKGDATADIR) / name;
        }
      else
        {
          log::alert ("unsupported scope: %1%") % s;
        }
    }
  else
    {
      rv = impl::instance_->top_srcdir_ / name;
    }

  if (!fs::exists (rv))
    log::trace ("%1%: no such file") % rv.string ();

  return rv.string ();
}

std::string
run_time::conf_file (scope s, const std::string& name) const
{
  fs::path rv;

  if (!running_in_place ())
    {
      /**/ if (pkg == s || sys == s)
        {
          rv = fs::path (PKGSYSCONFDIR) / name;
        }
      else
        {
          log::alert ("unsupported scope: %1%") % s;
        }
    }
  else
    {
      rv = impl::instance_->top_srcdir_ / "lib" / name;
      if (!fs::exists (rv))
        rv = impl::instance_->top_srcdir_ / name;
    }

  if (!fs::exists (rv))
    log::trace ("%1%: no such file") % rv.string ();

  return rv.string ();
}

std::string
run_time::exec_file (scope s, const std::string& name) const
{
  fs::path rv;

  if (!running_in_place ())
    {
      /**/ if (pkg == s)
        {
          rv = fs::path (PKGLIBEXECDIR) / name;
        }
      else
        {
          log::alert ("unsupported scope: %1%") % s;
        }
    }
  else
    {
      rv = impl::instance_->top_srcdir_ / "filters" / name;
    }

  if (!fs::exists (rv))
    log::trace ("%1%: no such file") % rv.string ();

  return rv.string ();
}

bool
run_time::running_in_place () const
{
  return impl::instance_->running_in_place_();
}

static
bool
is_option (const std::string& s)
{
  return (0 == s.find ("-"));
}

struct run_time::impl::unrecognize
{
  bool found_first_;

  unrecognize (const std::vector< po::option >::iterator& it)
    : found_first_(false)
  {
    if (std::vector< po::option >::iterator () != it)
      operator() (*it);
  }

  po::option
  operator() (po::option& item)
  {
    found_first_ |= item.string_key.empty ();
    found_first_ |= item.unregistered;
    item.unregistered = found_first_;

    return item;
  }
};

struct run_time::impl::env_var_mapper
{
  po::options_description opts_;

  enum { approx = true, exact = false };

  env_var_mapper (const po::options_description& opts)
    : opts_(opts)
  {}

  std::string
  operator() (const std::string& env_var)
  {
    static regex re (PACKAGE_ENV_VAR_PREFIX "(.*)");
    smatch option;

    if (regex_match (env_var, option, re)
        && opts_.find_nothrow (option[1], exact))
      return option[1];

    return std::string ();
  }
};

run_time::impl::impl (int argc, const char *const argv[])
  : gnu_opts_(CCB_("GNU standard options"))
  , std_opts_(CCB_("Standard options"))
  , shell_(DEFAULT_SHELL)
{
  lt_dlinit ();

  const char *srcdir (getenv ("srcdir"));
  if (srcdir)
    {
      // Set up run-in-place support
      // This support requires knowledge of where the top source and
      // build directories can be found.  We search for known source
      // and object files from suitable starting points in the file
      // system tree and walk up that tree until a match is found.

      fs::path src (fs::absolute (srcdir));

      while (!src.empty ()
             && !fs::exists (src / "lib" / "tests" / "run-time.cpp"))
        {
          src = src.parent_path ();
        }
      top_srcdir_ = src;

      if (top_srcdir_.empty ())
        {
          log::alert ("not in a source tree: %1%") % srcdir;
        }

      fs::path obj (fs::absolute ("."));

      while (!obj.empty ()
             && !fs::exists (obj / "lib" / "tests" / ".deps" / "run-time.Po"))
        {
          obj = obj.parent_path ();
        }
      top_builddir_ = obj;

      if (top_builddir_.empty ())
        {
          log::alert ("not in a build tree");
        }
    }

  argzero_ = argv[0];

  args_.resize (argc - 1);
  std::copy (argv + 1, argv + argc, args_.begin ());

  gnu_opts_
    .add_options ()
    ("help"   , CCB_("display this help and exit"))
    ("version", CCB_("output version information and exit"))
    ;

  std_opts_
    .add_options ()
    ;

  po::options_description cli_args;
  cli_args
    .add (gnu_opts_)
    .add (std_opts_)
    ;

  po::options_description env_args;
  env_args
    .add_options ()
    ("SHELL", (po::value< std::string > (&shell_)
               -> default_value (DEFAULT_SHELL)))
    ;

  po::parsed_options cmd_line (po::command_line_parser (args_)
                               .options (cli_args)
                               .allow_unregistered ()
                               .run ());

  std::transform (cmd_line.options.begin (), cmd_line.options.end (),
                  cmd_line.options.begin (),
                  unrecognize (cmd_line.options.begin ()));

  po::store (cmd_line, vm_);
  po::store (po::parse_environment (env_args, env_var_mapper (env_args)), vm_);
  po::notify (vm_);

  cmd_args_ = po::collect_unrecognized (cmd_line.options,
                                        po::include_positional);

  std::string cmd_name (argzero_.stem ().string ());
  std::string prefix (!running_in_place_()
                      ? libexec_prefix_
                      : libtool_prefix_);

  if (0 == cmd_name.find (prefix))
    cmd_name.erase (0, prefix.length ());
  if (!(PACKAGE_TARNAME == cmd_name || "main" == cmd_name))
    command_ = cmd_name;

  if (command_.empty ())
    {
      if (!cmd_args_.empty ()
          && !is_option (cmd_args_.front ()))
        {
          command_ = cmd_args_.front ();
          cmd_args_.erase (cmd_args_.begin ());
        }
    }
}

run_time::impl::~impl ()
{
  lt_dlexit ();
}

} // namespace utsushi
