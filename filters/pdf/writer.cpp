//  writer.cpp -- putting PDF object in a file
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>

#include "writer.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

using std::ios_base;
using std::runtime_error;
using std::string;
using std::stringstream;

writer::writer ()
  : _xref (xref ())
{
  _xref_pos = 0;
  _last_xref_pos = 0;
  octets_seen_ = 0;
  _saved_pos = 0;
  _mode = object_mode;
  _stream_len_obj = NULL;
}

writer::~writer ()
{
  delete _stream_len_obj;
  _stream_len_obj = NULL;
}

streamsize
writer::write (pdf::output::ptr& output)
{
  streamsize rv (output->write (stream_.str ().c_str (),
                                stream_.str ().size ()));

  if (string::size_type (rv) != stream_.str ().size ())
    BOOST_THROW_EXCEPTION
      (ios_base::failure ("PDF filter octet count mismatch"));

  stream_.str ("");

  return rv;
}

void
writer::write (object& obj)
{
  if (object_mode != _mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("invalid call to _pdf_::writer::write (object&)"));
    }

  _xref[obj.obj_num ()] = octets_seen_;

  ostream::pos_type mark (stream_.tellp ());
  stream_ << obj.obj_num ()
          << " 0 obj\n"
          << obj
          << "\n"
          << "endobj\n";
  octets_seen_ += stream_.tellp () - mark;
}

void
writer::begin_stream (dictionary& dict)
{
  if (stream_mode == _mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("invalid call to _pdf_::writer::begin_stream ()"));
    }
  _mode = stream_mode;

  _stream_len_obj = new primitive ();
  dict.insert ("Length", object (_stream_len_obj->obj_num ()));

  _xref[dict.obj_num ()] = octets_seen_;

  ostream::pos_type mark (stream_.tellp ());
  stream_ << dict.obj_num ()
          << " 0 obj\n"
          << dict
          << "\n"
          << "stream\n";
  octets_seen_ += stream_.tellp () - mark;
  _saved_pos = octets_seen_;
}

void
writer::write (const char *data, size_t n)
{
  if (stream_mode != _mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("invalid call to _pdf_::writer::write ()"));
    }
  stream_.write (data, sizeof (char) * n);
  octets_seen_ += sizeof (char) * n;
}

void
writer::write (const string& s)
{
  if (stream_mode != _mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("invalid call to _pdf_::writer::write ()"));
    }
  stream_ << s;
  octets_seen_ += s.size ();
}

void
writer::end_stream ()
{
  if (stream_mode != _mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("invalid call to _pdf_::writer::end_stream ()"));
    }
  _mode = object_mode;

  size_t pos = octets_seen_;
  size_t length = pos - _saved_pos;

  ostream::pos_type mark (stream_.tellp ());
  stream_ << "\n"
          << "endstream\n"
          << "endobj\n";
  octets_seen_ += stream_.tellp () - mark;

  // FIXME: overload the '=' operator in _pdf_::primitive
  *_stream_len_obj = primitive (length);

  write (*_stream_len_obj);
  delete _stream_len_obj;
  _stream_len_obj = NULL;
}

void
writer::header ()
{
  if (_mode == stream_mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("cannot write header in stream mode"));
    }
  ostream::pos_type mark (stream_.tellp ());
  stream_ << "%PDF-1.0\n";
  octets_seen_ += stream_.tellp () - mark;
}

void
writer::trailer (dictionary& trailer_dict)
{
  if (_mode == stream_mode)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("cannot write trailer in stream mode"));
    }
  write_xref ();
  write_trailer (trailer_dict);
}

// FIXME: clean up this kludge
void
writer::write_xref ()
{
  xref::const_iterator it;

  _last_xref_pos = _xref_pos;
  _xref_pos = octets_seen_;

  ostream::pos_type mark (stream_.tellp ());
  stream_ << "xref\n";

  stringstream ss;
  size_t start_obj_num = 0;
  size_t cur_obj_num = 0;
  size_t last_obj_num = 0;

  ss << "0000000000 65535 f " << endl;
  for(it = _xref.begin (); _xref.end () != it; ++it)
    {
      cur_obj_num = it->first;

      if (cur_obj_num != last_obj_num + 1)
        {
          // write out the current xref section and start a new one
          stream_ << start_obj_num
                  << " "
                  << last_obj_num + 1 - start_obj_num
                  << "\n"
                  << ss.str ();

          ss.str ("");          // flush stream
          start_obj_num = cur_obj_num;
        }

        last_obj_num = cur_obj_num;

        ss.width (10);
        ss.fill ('0');
        ss << it->second << " 00000 n " << endl;
    }

  if (!ss.str ().empty ())
    {
      stream_ << start_obj_num
              << " "
              << last_obj_num + 1 - start_obj_num
              << "\n"
              << ss.str ();
    }
  octets_seen_ += stream_.tellp () - mark;
}

void
writer::write_trailer (dictionary& trailer_dict)
{
  trailer_dict.insert ("Size", primitive (_xref.size () + 1));
  if (_last_xref_pos != 0)
    {
      trailer_dict.insert ("Prev", primitive (_last_xref_pos));
    }

  ostream::pos_type mark (stream_.tellp ());
  stream_ << "trailer\n"
          << trailer_dict
          << "\n"
          << "startxref\n"
          << _xref_pos
          << "\n"
          << "%%EOF\n";
  octets_seen_ += stream_.tellp () - mark;

  _xref.clear ();
}

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi
