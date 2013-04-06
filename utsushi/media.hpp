//  media.hpp -- related properties
//  Copyright (C) 2013  SEIKO EPSON CORPORATION
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

#ifndef utsushi_media_hpp_
#define utsushi_media_hpp_

#include <list>
#include <string>

namespace utsushi {

  class   key;
  class   quantity;
  typedef quantity length;

//! Properties of some well-known media
class media
{
public:
  media (const length& width, const length& height);

  length width () const;
  length height () const;

  static media lookup (const std::string& name);
  static std::list< std::string > within (const length& width,
                                          const length& height);
private:
  struct impl;
  impl *pimpl_;
};

}       // namespace utsushi

#endif  /* utsushi_media_hpp_ */
