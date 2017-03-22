//  combo.hpp -- API implementation for an combo driver
//  Copyright (C) 2017  SEIKO EPSON CORPORATION
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

#ifndef drivers_combo_hpp_
#define drivers_combo_hpp_

#include <list>
#include <string>

#include "utsushi/connexion.hpp"
#include "utsushi/scanner.hpp"
#include "utsushi/option.hpp"

namespace utsushi {

// TODO move to a combo driver plugin
extern "C" {
  void libdrv_combo_LTX_scanner_factory (const scanner::info&,
                                         scanner::ptr&);
}

namespace _drv_ {
namespace combo {

typedef std::list< std::pair< string, std::string > > kv_list;

class scanner
  : public utsushi::scanner
{
public:
  scanner (const kv_list& kvs, const bool debug = false);
  ~scanner ();

  connection connect_marker (const marker_signal_type::slot_type& slot) const;
  connection connect_update (const update_signal_type::slot_type& slot) const;

  streamsize read (octet *data, streamsize n);
  streamsize marker ();

  void cancel ();

  context get_context () const;
  option::map::ptr options ();
  streamsize buffer_size () const;

  bool is_single_image () const;

protected:
  void configure_options (const std::string& key, store::ptr s,
                          const std::string& name = std::string ());

  bool validate (const value::map& vm) const;
  void finalize (const value::map& vm);

private:

  std::string key;
  option::map combo_opts;
  utsushi::scanner::ptr active_scanner;
  typedef std::map< string, utsushi::scanner::ptr > map;
  map scanners;
};

}       // namespace combo
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_combo_hpp_ */
