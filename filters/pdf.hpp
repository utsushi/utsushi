//  pdf.hpp -- PDF image format support
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

#ifndef filters_pdf_hpp_
#define filters_pdf_hpp_

#include <string>

#include <utsushi/filter.hpp>

#include "pdf/array.hpp"
#include "pdf/dictionary.hpp"
#include "pdf/primitive.hpp"

namespace utsushi {
namespace _flt_ {

namespace _pdf_ {
  class writer;
}

class pdf
  : public filter
{
public:
  pdf (bool multi_file = false);
  ~pdf ();

  streamsize write (const octet *data, streamsize n);

protected:
  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eos (const context& ctx);

private:
  typedef context::size_type size_type;

  std::string content_type_;
  size_type _page;              // zero-offset page count, back is odd
  bool _match_direction;
  bool _need_page_trailer;
  size_type _pdf_h_sz; // scaled size to fit on a page at 72 dpi
  size_type _pdf_v_sz; // but better to set the dpi in the PDF file,
                       // there is some way to do that, I read it in the spec!

  _pdf_::writer *_doc;
  _pdf_::dictionary *_pages;
  _pdf_::array *_page_list;
  _pdf_::dictionary *_trailer;

  _pdf_::primitive *_img_height_obj;

  bool _rotate_180;
  bool _multi_file;

  friend class _pdf_::writer;

  void write_header ();
  void write_page_header ();
  void write_page_trailer ();
  void write_image_object (_pdf_::dictionary& image, std::string name);
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pdf_hpp_ */
