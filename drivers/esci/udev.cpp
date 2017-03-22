//  udev.cpp -- convenience API
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
//
//  License: BSL-1.0
//  Author : EPSON AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This file is distributed under the terms of the Boost Software
//  License, Version 1.0.
//
//  You ought to have received a copy of the Boost Software License
//  along along with this package.
//  If not, see <http://www.boost.org/LICENSE_1_0.txt>.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "udev.hpp"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>

#if __cplusplus < 201103L
#define nullptr 0
#endif

static void
get_sysattr (struct udev_device *device,
             const std::string& name, int& value, int radix = 16);

static struct udev *udev_ = nullptr;

udev_info::udev_info (const char *path)
{
  device_ = nullptr;

  if (path && 0 < strlen (path))
    {
      if (!udev_)
        {
          udev_ = udev_new ();
          if (!udev_)
            {
              std::clog << "udev_info: cannot initialize libudev\n";
            }
        }
      else
        {
          udev_ref (udev_);
        }

      if (udev_)
        {
          device_ = udev_device_new_from_syspath (udev_, path);
        }

      if (!device_)
        {
          std::clog << "udev_info: " << strerror (ENODEV) << "\n";
        }
    }
}

udev_info::~udev_info ()
{
  if (device_)
    {
      udev_device_unref (device_);
    }
  /*! \todo Release context if possible but this requires version 183
   *        or later which changed the return value of udev_unref().
   *        Versions before 183, i.e. before libudev1, return nothing.
   *
   *  udev_ = udev_unref (udev_);
   */
}

uint16_t
udev_info::usb_vendor_id () const
{
  int id = 0;
  get_sysattr (device_, "idVendor", id);
  return id;
}

uint16_t
udev_info::usb_product_id () const
{
  int id = 0;
  get_sysattr (device_, "idProduct", id);
  return id;
}

uint8_t
udev_info::usb_configuration () const
{
  int i = 1;
  get_sysattr (device_, "bConfigurationValue", i);
  return i;
}

uint8_t
udev_info::usb_interface () const
{
  int i = 0;
  get_sysattr (device_, "bInterfaceNumber", i);
  return i;
}

uint8_t
udev_info::usb_bus_number () const
{
  int i = 0;
  get_sysattr (device_, "busnum", i, 10);
  return i;
}

uint8_t
udev_info::usb_device_address () const
{
  int i = 0;
  get_sysattr (device_, "devnum", i, 10);
  return i;
}

uint8_t
udev_info::usb_port_number () const
{
  int i = 0;
  get_sysattr (device_, "devpath", i, 10);
  return i;
}

static void
get_sysattr (struct udev_device *device,
             const std::string& name, int& value, int radix)
{
  if (!device || name.empty ())
  {
    return;
  }

  if (2 != radix && 8 != radix && 10 != radix && 16 != radix)
  {
    return;
  }

  struct udev_device *p = device;
  const char *val = nullptr;

  while (p)
    {
      val = udev_device_get_sysattr_value (p, name.c_str ());
      if (val)
        {
          break;
        }
      p  = udev_device_get_parent (p);
    }

  if (!val)
    {
      return;
    }

  if ("devpath" == name)
    {
      const char *tmp;
      tmp = strrchr (val, '-');
      if (tmp)
        {
          val = tmp + 1;
        }
      tmp = strrchr (val, '.');
      if (tmp)
        {
          val = tmp + 1;
        }
    }

  value = strtol (val, nullptr, radix);
}
