//  magick.hpp -- touches applied to your image data
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

#ifndef filters_magick_hpp_
#define filters_magick_hpp_

#include "shell-pipe.hpp"

#include <string>

namespace utsushi {
namespace _flt_ {

class magick
  : public shell_pipe
{
public:
  magick ();

protected:
  void freeze_options ();

  context estimate (const context& ctx);

  std::string arguments (const context& ctx);

  bool bilevel_;
  double threshold_;

  double x_resolution_;
  double y_resolution_;

  bool force_extent_;
  double width_;
  double height_;

  string image_format_;

  bool color_correction_;
  double cct_[9];

  double brightness_;
  double contrast_;

  bool auto_orient_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_magick_hpp_ */
