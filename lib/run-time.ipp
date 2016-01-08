//  run-time.ipp -- implementation details
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef lib_run_time_ipp_
#define lib_run_time_ipp_

/*! \file
 *  \brief Expose enough detail to make run_time testable
 *
 *  Unit tests for the run_time API need to be able to "reset" the
 *  singleton between tests.  The public API does not provide for this
 *  (on purpose).  This header file exposes the implementation details
 *  for use by unit tests so they can get their job done.
 *
 *  Typical use would be in a test fixture destructor where the single
 *  instance_ can be deleted and reinitialised to \c NULL.
 */

#include <boost/filesystem/path.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "utsushi/run-time.hpp"

namespace utsushi {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

//! Internal API to the run-time state information
/*! The public run_time API does not expose any state information in
 *  the shape of member variables.  All member variables are "hidden"
 *  in this implementation class.
 */
class run_time::impl
{
public:
  //! Initialise the instance_ based on the command-line arguments
  /*! Parses the content of \a argv and handles the standard options
   *  it encounters.  Supported standard options not included on the
   *  command-line will be handled in default fashion.  Environment
   *  variables and configuration files will be dealt with as well.
   */
  impl (int argc, const char *const argv[]);
  ~impl ();

  static impl *instance_;

  run_time::sequence_type args_;

  fs::path    argzero_;
  std::string command_;

  po::variables_map vm_;
  po::options_description gnu_opts_;
  po::options_description std_opts_;

  run_time::sequence_type cmd_args_;

  std::string shell_;

  fs::path top_builddir_;
  fs::path top_srcdir_;
  bool running_in_place_() const
  {
    return !top_srcdir_.empty ();
  }

  struct unrecognize;
  struct env_var_mapper;

  static const std::string libexec_prefix_;
  static const std::string libtool_prefix_;
};

} // namespace utsushi

#endif /* lib_run_time_ipp_ */
