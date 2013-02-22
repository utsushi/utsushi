//  file.hpp -- based output devices
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

#ifndef utsushi_file_hpp_
#define utsushi_file_hpp_

#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "device.hpp"
#include "format.hpp"

namespace utsushi {

namespace fs = boost::filesystem;

typedef boost::filesystem::basic_filebuf<octet> file;

//!  Create path names following a simple pattern
class path_generator
{
  format   format_;
  unsigned offset_;

public:
  //!  Default constructor
  /*!  A default path_generator instance evaluates to \c false in a
   *   Boolean context.  Its operator() member function should never
   *   be invoked.
   *
   *   \internal
   *   This odd behaviour allows for a file_odevice implementation
   *   that can handle %output to a single file and multiple files
   *   transparently.
   */
  path_generator ();

  //!  Creates a regular path_generator instance
  /*!  A regular instance uses a path \a prefix and an optional \a
   *   extension to generate sequentially numbered paths.  The paths
   *   contain a \e minimal number of \a digits and start at a given
   *   non-negative \a offset.  The \a offset is zero-padded (on the
   *   left) if necessary.
   *
   *   If a non-empty \a extension does not start with an extension
   *   separator (a dot), one is automatically inserted.
   */
  path_generator (const fs::path& prefix,
                  const fs::path& extension = fs::path(),
                  unsigned digits = 3, unsigned offset = 0);

  operator bool () const;

  fs::path operator() ();
};

//!  Load an image data sequence from file(s)
class file_idevice : public idevice
{
  path_generator generator_;

  fs::path name_;
  file file_;
  bool used_;

protected:
  bool is_consecutive () const;

  bool obtain_media ();
  bool set_up_image ();
  void finish_image ();

  streamsize sgetn (octet *data, streamsize n);

public:
  //!  Creates a device that loads an image from file
  file_idevice (const fs::path& name);

  //!  Create a device that loads images from multiple files
  /*!  Path names are provided by a \a generator.  The first path name
   *   for which no corresponding file exists triggers an end of image
   *   data sequence condition.
   */
  file_idevice (const path_generator& generator);

  ~file_idevice ();
};

//!  Save an image data sequence to file
class file_odevice : public odevice
{
  fs::path name_;
  file file_;

  path_generator generator_;

public:
  //!  Creates a device that saves all image data in a single file
  /*!  \note  The file will not be opened until the sequence of scans
   *          begins.
   */
  file_odevice (const fs::path& name);

  //!  Creates a device that saves images in separate files
  /*!  Path names are provided by a \a generator.
   *   \note  Files are not opened until the start of an image.
   */
  file_odevice (const path_generator& generator);

  streamsize write (const octet *data, streamsize n);

protected:
  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eos (const context& ctx);
};

}       // namespace utsushi

#endif  /* utsushi_file_hpp_ */
