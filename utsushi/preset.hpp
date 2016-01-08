//  preset.hpp -- a collection of targetted scan settings
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_preset_hpp_
#define utsushi_preset_hpp_

#include <string>

namespace utsushi {

class preset
{
public:
  preset (const std::string& name, const std::string& text = "")
    : name_(name), text_(text)
  {}

  std::string name () const;
  std::string text () const;

private:
  std::string name_;
  std::string text_;
};

}       // namespace utsushi

#endif  /* utsushi_preset_hpp_ */
