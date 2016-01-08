//  run-time.hpp -- information for a program
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

#ifndef utsushi_run_time_hpp_
#define utsushi_run_time_hpp_

#include <string>
#include <vector>

#include <boost/program_options/variables_map.hpp>

namespace utsushi {

//! Singleton for access to a program's run-time information
/*! During its execution, a program normally needs various pieces of
 *  information scattered throughout the code that tries to get the
 *  program's job done.  The run_time singleton provides access to a
 *  part of that information.  The following are available:
 *
 *  - command-line arguments, including the program name
 *  - values of program specific environment variables
 *  - possible locations for installed plugins and data files
 *
 *  In addition to this information, the run_time also encapsulates
 *  the logic needed to locate and execute @PACKAGE_NAME@ commands.
 */
class run_time
{
public:
  //! An implementation dependent forward iterable container
  typedef std::vector< std::string > sequence_type;
  typedef boost::program_options::variable_value value_type;
  typedef std::map< std::string, value_type >::size_type size_type;

  //! Initialise program run-time environmental information
  /*! A program's \c main() should create a run_time instance using
   *  this constructor, passing all the command-line arguments that
   *  are available.  The constructor will \e not modify any of the
   *  information passed and silently ignore anything it does not
   *  recognise.
   *
   *  An optional third argument may be passed (\sa utsushi::i18n) to
   *  set up localisation support.  This is primarily meant for use by
   *  the free-standing command implementations that come bundled with
   *  @PACKAGE_NAME@.  If requested, the environment's locale settings
   *  will take effect and message translation will use the @PACKAGE@
   *  text domain by default.
   *
   *  This constructor can only be used once.  Any additional use will
   *  throw a std::logic_error exception.  Use the default run_time()
   *  constructor instead.
   */
  run_time (int argc, const char *const argv[], bool configure_i18n = false);

  //! Get access to run-time environmental information
  /*! After initial instantiation, further access to the singleton is
   *  provided via the default constructor.
   *
   *  Use of this constructor before an initialising multi-argument
   *  constructor will result in a std::logic_error exception.
   */
  run_time ();

  //! Retrieve the canonical program name
  std::string
  program () const;

  //! Obtain the command used in the command-line invocation
  /*! If no command was entered on the command-line, an empty string
   *  will be returned.
   */
  std::string
  command () const;

  //! Unprocessed command-line arguments
  const sequence_type&
  arguments () const;

  //! Find the whereabouts of a \a command implementation
  /*! A non-empty return value can be used to build a \c shell_command
   *  one may want to execute().
   */
  std::string
  locate (const std::string& command) const;

  //! Replace the current process with a \a shell_command
  /*! This is primarily useful for the user-space program when it
   *  needs to dispatch to a free-standing command implementation.
   */
  void
  execute (const std::string& shell_command) const;

  //! Number of times an \a option was encountered
  /*! Options may be given on the command-line, set via environment
   *  variables and specified in configuration files.
   */
  size_type
  count (const std::string& option) const;

  const value_type&
  operator[] (const std::string& option) const;

  std::string
  help (const std::string& summary = std::string ()) const;

  std::string
  version (const std::string& legalese   = std::string (),
           const std::string& disclaimer = std::string ()) const;

  //! Limit the extent of file system locations to be searched
  enum scope {
    pkg,                        //!< package specific locations
    sys,                        //!< normal system locations
    usr,                        //!< user's home directory
  };

  sequence_type
  load_dirs (scope s, const std::string& component) const;

  std::string
  data_file (scope s, const std::string& name) const;

  std::string
  conf_file (scope s, const std::string& name) const;

  std::string
  exec_file (scope s, const std::string& name) const;

  bool
  running_in_place () const;

  class impl;
};

} // namespace utsushi

#endif /* utsushi_run_time_hpp_ */
