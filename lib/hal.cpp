//  hal.cpp -- OO wrapper around bits and pieces of the HAL API
//  Copyright (C) 2012, 2014  SEIKO EPSON CORPORATION
//  Copyright (C) 2009  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : Olaf Meeuwissen
//  Origin : FreeRISCI
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

#include <cstdlib>
#include <stdexcept>

#include <boost/throw_exception.hpp>

#include "hal.hpp"

namespace HAL {

  using namespace std;

    static LibHalContext *
    hal_ctx_init (void)
    {
      LibHalContext *ctx = libhal_ctx_new ();
      if (!ctx) BOOST_THROW_EXCEPTION (bad_alloc ());

      DBusError error;
      dbus_error_init (&error);

      DBusConnection *cnx = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

      bool all_is_well = !dbus_error_is_set (&error);

      if (all_is_well)
        all_is_well = libhal_ctx_set_dbus_connection (ctx, cnx);
      if (all_is_well)
        all_is_well = libhal_ctx_init (ctx, &error);

      if (!all_is_well)
        {
          libhal_ctx_free (ctx);
          runtime_error rte (error.message);
          dbus_error_free (&error);
          BOOST_THROW_EXCEPTION (rte);
        }

      return ctx;
    }

    device::device (const std::string& type,
                    const std::string& path)
      : udi_(), ctx_(NULL)
    {
      ctx_ = hal_ctx_init ();

      DBusError error;
      dbus_error_init (&error);

      int    cnt = 0;
      char **udi = libhal_manager_find_device_string_match
        (ctx_, "linux.sysfs_path", path.c_str (), &cnt, &error);

      if (!udi || 1 != cnt)
        {
          libhal_free_string_array (udi);
          libhal_ctx_free (ctx_);
          runtime_error rte (error.message);
          dbus_error_free (&error);
          BOOST_THROW_EXCEPTION (rte);
        }

      udi_ = udi[0];

      libhal_free_string_array (udi);
    }

    device::device (const std::string& udi)
      : udi_(udi), ctx_(NULL)
    {
      ctx_ = hal_ctx_init ();

      DBusError error;
      dbus_error_init (&error);

      if (!libhal_device_exists (ctx_, udi_.c_str (), &error))
        {
          libhal_ctx_free (ctx_);
          runtime_error rte (error.message);
          dbus_error_free (&error);
          BOOST_THROW_EXCEPTION (rte);
        }
    }

    device::~device (void)
    {
      libhal_ctx_free (ctx_);
    }

    std::string
    device::subsystem (void) const
    {
      std::string s;
      try
        {
          get_property_("info.subsystem", s);
        }
      catch (runtime_error& e)
        {
          get_property_("info.bus", s);
        }
      return s;
    }

    uint16_t
    device::usb_vendor_id (void) const
    {
      int id;
      get_property_("usb.vendor_id", id);
      return id;
    }

    uint16_t
    device::usb_product_id (void) const
    {
      int id;
      get_property_("usb.product_id", id);
      return id;
    }

    std::string
    device::usb_serial (void) const
    {
      std::string s;
      try {
        get_property_("usb.serial", s);
      }
      catch (const runtime_error& e) {
        // assume no usb.serial property available
      }
      return s;
    }

    uint8_t
    device::usb_configuration (void) const
    {
      int i = 1;
      try {
        get_property_("usb.configuration_value", i);
      }
      catch (const runtime_error& e) {
        // use the default configuration
      }
      return i;
    }

    uint8_t
    device::usb_interface (void) const
    {
      int i = 0;
      try {
        get_property_("usb.interface.number", i);
      }
      catch (const runtime_error& e) {
        // use the default interface
      }
      return i;
    }

    uint8_t
    device::usb_bus_number (void) const
    {
      int i = 0;
      try {
        get_property_("usb.bus_number", i);
      }
      catch (const runtime_error& e) {
        // return an invalid bus number
      }
      return i;
    }

    uint8_t
    device::usb_port_number (void) const
    {
      int i = 0;
      try {
        get_property_("usb.port_number", i);
      }
      catch (const runtime_error& e) {
        // return an invalid port number
      }
      return i;
    }

    uint8_t
    device::usb_device_address (void) const
    {
      int i = 0;
      try {
        get_property_("usb.linux.device_number", i);
      }
      catch (const runtime_error& e) {
        // return an invalid device address
      }
      return i;
    }

    void
    device::get_property_(const std::string& name, int& value) const
    {
      DBusError error;
      dbus_error_init (&error);

      int val = libhal_device_get_property_int (ctx_, udi_.c_str (),
                                                name.c_str (), &error);
      if (dbus_error_is_set (&error))
        {
          runtime_error rte (error.message);
          dbus_error_free (&error);
          BOOST_THROW_EXCEPTION (rte);
        }
      value = val;
    }

    void
    device::get_property_(const std::string& name, std::string& value) const
    {
      DBusError error;
      dbus_error_init (&error);

      char *s = libhal_device_get_property_string (ctx_, udi_.c_str (),
                                                   name.c_str (), &error);
      if (dbus_error_is_set (&error))
        {
          if (s) free (s);
          runtime_error rte (error.message);
          dbus_error_free (&error);
          BOOST_THROW_EXCEPTION (rte);
        }
      value = s;
      if (s) free (s);
    }

}       // namespace HAL
