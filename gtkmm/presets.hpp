//  presets.hpp -- for scanner device selection and maintenance actions
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

#ifndef gtkmm_presets_hpp_
#define gtkmm_presets_hpp_

#include <gtkmm/builder.h>

#include <utsushi/preset.hpp>

#include "dropdown.hpp"

namespace utsushi {
namespace gtkmm {

class presets : public dropdown
{
  typedef dropdown base;

public:
  presets (BaseObjectType *ptr, Glib::RefPtr<Gtk::Builder>& builder);

private:
  void insert_preset (type_id type, const preset& preset);
  void insert_custom (const preset& preset);
  void insert_system (const preset& preset);
};

}       // namespace gtkmm
}       // namespace utsushi

#endif  /* gtkmm_presets_hpp_ */
