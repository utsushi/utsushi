//  file.hpp -- based output devices
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

#ifndef utsushi_file_hpp_
#define utsushi_file_hpp_

#include "device.hpp"

#include <cstddef>
#include <fstream>
#include <string>

namespace utsushi {

//!  Create path names following a simple pattern
class path_generator
{
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

  //!  Creates a \c %i formatter \a pattern based instance
  /*!  The formatter may be a simple \c %i or contain a field width
   *   specifier, similar to the printf() version.  A `0` flag is
   *   allowed but not required.  Fields are always zero filled.
   *
   *   If \a pattern does not contain a \c %i formatter, a default
   *   constructed instance will be created.
   */
  explicit path_generator (const std::string& pattern);

  operator bool () const;

  std::string operator() ();

private:
  std::string parent_;
  std::string format_;
  unsigned    offset_;
};

//!  Load an image data sequence from file(s)
class file_idevice
  : public idevice
{
public:
  //!  Creates a device that loads an image from file
  file_idevice (const std::string& filename);

  //!  Create a device that loads images from multiple files
  /*!  Path names are provided by a \a generator.  The first path name
   *   for which no corresponding file exists triggers an end of image
   *   data sequence condition.
   */
  file_idevice (const path_generator& generator);

  ~file_idevice ();

protected:
  bool is_consecutive () const;

  bool obtain_media ();
  bool set_up_image ();
  void finish_image ();

  streamsize sgetn (octet *data, streamsize n);

private:
  std::string    filename_;
  path_generator generator_;

  std::basic_filebuf< octet > file_;
  bool used_;
};

//!  Save an image data sequence to one or more files
class file_odevice
  : public odevice
{
public:
  //!  Creates a device that saves all image data in a single file
  /*!  \note  The file will not be opened until the sequence of scans
   *          begins.
   */
  file_odevice (const std::string& filename);

  //!  Creates a device that saves images in separate files
  /*!  Path names are provided by a \a generator.
   *   \note  Files are not opened until the start of an image.
   */
  file_odevice (const path_generator& generator);

  ~file_odevice ();

  streamsize write (const octet *data, streamsize n);

protected:
  virtual void open ();
  virtual void close ();

  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eos (const context& ctx);
  void eof (const context& ctx);

  std::string    filename_;
  path_generator generator_;

  int fd_;
  int fd_flags_;

  size_t count_;
};

}       // namespace utsushi

#endif  /* utsushi_file_hpp_ */
