//  pdf.cpp -- PDF image format support
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

#include <sstream>

#include <boost/assert.hpp>

#include "pdf.hpp"
#include "pdf/writer.hpp"

namespace utsushi {
namespace _flt_ {

pdf::pdf (bool multi_file)
  : _match_direction (false)
  , _doc (NULL)
  , _pages (NULL)
  , _page_list (NULL)
  , _trailer (NULL)
  , _img_height_obj (NULL)
  , _rotate_180 (false)
  , _multi_file (multi_file)
{
  _doc = new _pdf_::writer ();
}

pdf::~pdf ()
{
  delete _doc;
  delete _pages;
  delete _page_list;
  delete _trailer;
  delete _img_height_obj;
}

streamsize
pdf::write (const octet * data, streamsize n)
{
  if (!data || 0 == n) return 0;

  _doc->write (data, n);
  _doc->write (output_);

  return n;
}

void
pdf::bos (const context& ctx)
{
  _page = 0;
  _need_page_trailer = false;

  _pdf_::object::reset_object_numbers ();

  write_header ();
  _doc->write (output_);
}

// FIXME image height may be unknown at this point
//       At other places this is taken into account properly but here
//       the computation of _pdf_v_sz assumes it is known.  Not sure
//       if this can be fixed at all.
void
pdf::boi (const context& ctx)
{
  BOOST_ASSERT (   "image/jpeg"  == ctx.content_type ()
                || "image/g3fax" == ctx.content_type ());

  if (_multi_file)
    {
      size_type page (_page);
      bos (ctx);
      _page = page;
    }

  content_type_ = ctx.content_type ();
  ctx_ = ctx;
  ctx_.content_type ("application/pdf");

  // Adjust to PDF default user space coordinates (1/72 inch)
  _pdf_h_sz = (72.0 * ctx_.width ()) / ctx_.x_resolution ();
  _pdf_v_sz = (72.0 * ctx_.height ()) / ctx_.y_resolution ();

  write_page_header ();
  _doc->write (output_);
  ++_page;
}

void
pdf::eoi (const context& ctx)
{
  write_page_trailer ();
  _doc->write (output_);
  _rotate_180 = _match_direction && (_page % 2);
}

void
pdf::eos (const context& ctx)
{
  if (_need_page_trailer)
    {
      write_page_trailer ();
      _doc->write (output_);
    }
}

void
pdf::write_header ()
{
  _doc->header ();

  delete _pages;
  _pages = new _pdf_::dictionary ();

  _pdf_::dictionary info;
  info.insert ("Producer", _pdf_::primitive ("(" PACKAGE_STRING ")"));
  info.insert ("Creator", _pdf_::primitive ("(" PACKAGE_STRING ")"));
  _doc->write (info);

  _pdf_::dictionary catalog;
  catalog.insert ("Type", _pdf_::primitive ("/Catalog"));
  catalog.insert ("Pages", _pdf_::object (_pages->obj_num ()));
  _doc->write (catalog);

  delete _trailer;
  _trailer = new _pdf_::dictionary ();
  _trailer->insert ("Info", _pdf_::object (info.obj_num ()));
  _trailer->insert ("Root", _pdf_::object (catalog.obj_num ()));

  delete _page_list;
  _page_list = new _pdf_::array ();
}

void
pdf::write_page_header ()
{
  _pdf_::dictionary page;

  _page_list->insert (_pdf_::object (page.obj_num ()));

  _pages->insert ("Type", _pdf_::primitive ("/Pages"));
  _pages->insert ("Kids", _page_list);
  _pages->insert ("Count", _pdf_::primitive (_page_list->size ()));

  _doc->write (*_pages);

  _pdf_::dictionary image;
  _pdf_::dictionary contents;

  _pdf_::array mbox;
  mbox.insert (_pdf_::primitive (0));
  mbox.insert (_pdf_::primitive (0));
  mbox.insert (_pdf_::primitive (_pdf_h_sz));
  mbox.insert (_pdf_::primitive (_pdf_v_sz));

  std::stringstream ss2;
  std::string img_name;

  ss2 << PACKAGE_TARNAME "Image" << _page;
  img_name = ss2.str ();

  _pdf_::array procset;
  std::string cproc = "/ImageB";
  if (ctx_.is_rgb ()) cproc = "/ImageC";

  _pdf_::dictionary tmp;
  tmp.insert (img_name.c_str (), _pdf_::object (image.obj_num ()));

  procset.insert (_pdf_::primitive ("/PDF"));
  procset.insert (_pdf_::primitive (cproc));

  _pdf_::dictionary rsrc;
  rsrc.insert ("XObject", &tmp);
  rsrc.insert ("ProcSet", &procset);

  page.insert ("Type", _pdf_::primitive ("/Page"));
  page.insert ("Parent", _pdf_::object (_pages->obj_num ()));
  page.insert ("Resources", &rsrc);
  page.insert ("MediaBox", &mbox);
  page.insert ("Contents", _pdf_::object (contents.obj_num ()));

  _doc->write (page);

  _doc->begin_stream (contents);

  // transformation matrices must be specified in reverse order
  std::stringstream ss;
  ss << "q" << std::endl;
  ss << _pdf_h_sz << " 0 0 " << _pdf_v_sz << " 0 0 cm" << std::endl;
  if (_rotate_180)
    {
      // undo the translation below
      ss << "1 0 0 1 0.5 0.5 cm" << std::endl;

      // reflect along x and y axis
      ss << "-1 0 0 -1 0 0 cm" << std::endl;

      // translate so the image midpoint lies on the origin
      ss << "1 0 0 1 -0.5 -0.5 cm" << std::endl;
    }
  ss << "/" << img_name << " Do" << std::endl;
  ss << "Q";

  _doc->write (ss.str ());
  _doc->end_stream ();

  write_image_object (image, img_name);

  _need_page_trailer = true;
}

void
pdf::write_image_object (_pdf_::dictionary& image, std::string name)
{
  delete _img_height_obj;
  _img_height_obj = new _pdf_::primitive ();

  image.insert ("Type", _pdf_::primitive ("/XObject"));
  image.insert ("Subtype", _pdf_::primitive ("/Image"));
  image.insert ("Width", _pdf_::primitive (ctx_.width ()));
  image.insert ("Height", _pdf_::object (_img_height_obj->obj_num ()));

  _pdf_::array decode;
  std::string dev = "/DeviceGray";
  if (ctx_.is_rgb ()) dev = "/DeviceRGB";

  image.insert ("ColorSpace", _pdf_::primitive (dev));
  image.insert ("BitsPerComponent", _pdf_::primitive (ctx_.depth ()));
  image.insert ("Interpolate", _pdf_::primitive ("true"));

  _pdf_::dictionary parms;
  if ("image/jpeg" == content_type_)
    {
      image.insert ("Filter", _pdf_::primitive ("/DCTDecode"));
    }
  else if ("image/g3fax" == content_type_)
    {
      image.insert ("Filter", _pdf_::primitive ("/CCITTFaxDecode"));

      parms.insert ("Columns", _pdf_::primitive (ctx_.width ()));
      parms.insert ("Rows", _pdf_::object (_img_height_obj->obj_num ()));
      parms.insert ("EndOfBlock", _pdf_::primitive ("false"));
      parms.insert ("EndOfLine", _pdf_::primitive ("true"));
      parms.insert ("EncodedByteAlign", _pdf_::primitive ("false"));
      parms.insert ("K", _pdf_::primitive (0));   // CCITT3 1-D encoding
      image.insert ("DecodeParms", &parms);
    }

  // see PDF reference 1.7 p. 342 and p. 1107 # 53
  image.insert ("Name", _pdf_::primitive ("/" + name));

  _doc->begin_stream (image);
}

void
pdf::write_page_trailer ()
{
  _doc->end_stream ();

  *_img_height_obj = _pdf_::primitive (ctx_.height ());
  _doc->write (*_img_height_obj);

  _doc->trailer (*_trailer);

  _need_page_trailer = false;

  _pdf_h_sz = 0;
  _pdf_v_sz = 0;
}

}       // namespace _flt_
}       // namespace utsushi
