//  writer.hpp -- putting PDF objects in a file
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

#ifndef filters_pdf_writer_hpp_
#define filters_pdf_writer_hpp_

#include <map>
#include <sstream>

#include "../pdf.hpp"
#include "dictionary.hpp"
#include "object.hpp"
#include "primitive.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

/*! Writes PDF objects to a file.
 * See section 3.4 of the PDF Reference version 1.7 for details on the basic
 * file structure of a PDF file.
 *
 * There are two writing modes: object mode and stream mode.
 * The default mode is object mode. When begin_stream() is called, the current
 * mode is set to stream mode. When end_stream() is called, the mode is reset
 * to object mode.
 *
 * In object mode, PDF objects are written all at once. Stream mode allows the
 * writing of PDF stream objects which can be written incrementally.
 */
class writer
{
private:
  typedef std::map<size_t, size_t> xref;

  std::stringstream stream_;

  xref _xref;
  size_t _xref_pos;
  size_t _last_xref_pos;

  size_t octets_seen_;
  size_t _saved_pos;

  primitive* _stream_len_obj;

  typedef enum {
    object_mode,
    stream_mode
  } write_mode;

  write_mode _mode;

public:
  /*! Creates a new _pdf_::writer object
   */
  writer ();

  ~writer ();

  streamsize write (pdf::output::ptr& output);

  /*! Writes a _pdf_::object to the file as an indirect object [p 63].
   *
   * Recursively writes any child objects to the file.
   * Can only be called while in object mode, which is the default
   * mode. If this method is called between calls to begin_stream() and
   * end_stream() a std::runtime_error exception is thrown.
   */
  void write (object& object);

  /*! Initializes a PDF stream [p 60] and sets the current mode to stream mode.
   *
   *  Pass a _pdf_::dictionary object, \a dict, defining the stream properties,
   *  omitting the mandatory "Length" property which is automatically
   *  calculated and written to the file. If this method is called when already
   *  in stream mode, a std::runtime_error exception is thrown.
   */
  void begin_stream (dictionary& dict);

  /*! Writes \a n bytes from \a buf to the file as part of a PDF stream.
   *
   *  Can only be called while in stream mode.
   *  Calls to this method must be contained within calls to
   *  begin_stream() and end_stream(). If not, a std::runtime_error
   *  exception is thrown.
   */
  void write (const char* buf, size_t n);

  /*! Writes a string \a s to the file as part of a PDF stream.
   *
   *  Can only be called while in stream mode.
   *  Calls to this method must be contained within calls to
   *  begin_stream() and end_stream(). If not, a std::runtime_error
   *  exception is thrown.
   */
  void write (const std::string& s);

  /*! Finishes writing a PDF stream and sets the current mode to object mode.
   *
   *  The "Length" property of the stream is written at this point as an
   *  indirect object. Can only be called when in stream mode, if not, a
   *  std::runtime_error exception is thrown.
   */
  void end_stream ();

  /*! Writes the PDF header [p 92].
   *
   *  A std::runtime_error exception is thrown if this method is called while
   *  in stream mode.
   */
  void header ();

  /*! Writes the PDF trailer [p 96] and xref table [p 93].
   *
   * Also sets the "Prev" entry in \a trailer_dict and clears the internal
   * xref table. A std::runtime_error exception is thrown if this method is
   * called while in stream mode.
   *
   * \param trailer_dict The trailer entries to be written.
   */
  void trailer (dictionary& trailer_dict);

private:
  // Writes the cross-reference table [p 93].
  void write_xref ();

  // Writes the file trailer [p 96].
  void write_trailer (dictionary& trailer_dict);
};

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pdf_writer_hpp_ */
