//  autocrop.hpp -- leaving only the (reoriented) scanned documents
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef filters_autocrop_hpp_
#define filters_autocrop_hpp_

#include "shell-pipe.hpp"

namespace utsushi {
namespace _flt_ {

class autocrop
  : public shell_pipe
{
public:
  autocrop ();

  void mark (traits::int_type c, const context& ctx);

protected:
  void freeze_options ();

  context estimate (const context& ctx);
  context finalize (const context& ctx);

  std::string arguments (const context& ctx);

  void checked_write (octet *data, streamsize n);

private:
  static const streamsize header_buf_size_ = 64;

  bool       header_seen_;
  octet      header_buf_[header_buf_size_];
  streamsize header_buf_used_;

  context::size_type width_;
  context::size_type height_;

  double lo_threshold_;
  double hi_threshold_;
  bool   trim_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_autocrop_hpp_ */
