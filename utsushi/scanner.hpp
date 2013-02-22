//  scanner.hpp -- interface and support classes
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


#ifndef utsushi_scanner_hpp_
#define utsushi_scanner_hpp_

#include <string>

#include "utsushi/connexion.hpp"
#include "utsushi/device.hpp"
#include "utsushi/option.hpp"

namespace utsushi {

class scanner
  : public idevice
  , protected option::map       // FIXME idevice provides configurable
{
public:
  typedef shared_ptr< scanner > ptr;

  class id;
  static ptr create (connexion::ptr cnx, const scanner::id& id);

protected:
  scanner (connexion::ptr cnx)
    : cnx_(cnx)
  {
    option_.reset (static_cast< option::map * > (this),
                   null_deleter ());
  }

  shared_ptr< connexion > cnx_;
};

class scanner::id
{
  std::string udi_;
  std::string nick_;
  std::string text_;

  std::string model_;
  std::string vendor_;
  std::string type_;

  bool invalid_(const std::string& fragment) const;
  bool promise_(bool nothrow = false) const;

public:
  id (const std::string& udi);
  id (const std::string& model, const std::string& vendor,
      const std::string& udi);
  id (const std::string& model, const std::string& vendor,
      const std::string& path, const std::string& iftype,
      const std::string& driver = "");

  std::string path () const;
  std::string name () const;
  std::string text () const;
  std::string model () const;
  std::string vendor () const;
  std::string type () const;

  std::string driver () const;
  std::string iftype () const;

  std::string udi () const;

  void name (const std::string& name);
  void text (const std::string& text);
  void driver (const std::string& driver);

  bool is_configured () const;
  bool is_local () const;

  bool has_driver () const;

  static bool is_valid (const std::string& udi);

  static const char separator;

private:
  id (const std::string& udi, bool nocheck);
};

}       // namespace utsushi

namespace std {

template <>
struct less<utsushi::scanner::id>
{
  bool operator() (const utsushi::scanner::id& x,
                   const utsushi::scanner::id& y) const
  {
    return x.udi () < y.udi ();
  }
};

}       // namespace std

#endif  /* utsushi_scanner_hpp_ */
