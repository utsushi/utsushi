//  usb.cpp -- device I/O API
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

#include "usb.hpp"

#include "interpreter.hpp"
#include "udev.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <libusb.h>
#if !HAVE_LIBUSB_ERROR_NAME
#include <sstream>
#include <string>

static std::string
libusb_error_name (int err)
{
  std::ostringstream os;
  os << "err" << err;
  return os.str ();
}
#endif

#define require assert
#define promise assert

#if __cplusplus < 201103L
#define nullptr 0
#define noexcept
#endif

extern int (*interpreter_ctor_) (callback *, callback *);
extern int (*interpreter_dtor_) ();

static const int timeout_ = 0;        // never

static libusb_context       *context_ = nullptr;
static libusb_device_handle *handle_  = nullptr;

static int interface_       =  0;
static int bulk_endpoint_i_ = -1;
static int bulk_endpoint_o_ = -1;

static const char *prefix = "usb:";
static const char  sep    = ':';

uint16_t
get_vid (const std::string& udi)
{
  if (0 != udi.find (prefix)) return 0;

  std::string::size_type pos1 = udi.find (sep) + 1;
  std::string::size_type pos2 = udi.find (sep, pos1);
  if (std::string::npos != pos2) pos2 -= pos1;

  char         *end = nullptr;
  unsigned long vid = strtoul (udi.substr (pos1, pos2).c_str (), &end, 16);

  if (end && '\0' == *end && vid <= 0xFFFF)
    return vid;

  return 0;
}

uint16_t
get_pid (const std::string& udi)
{
  if (0 != udi.find (prefix)) return 0;

  std::string::size_type pos1 = udi.find (sep) + 1;
  std::string::size_type pos2 = udi.find (sep, pos1);

  if (std::string::npos == pos2) return 0; // no PID

  pos1 = pos2 + 1;
  pos2 = udi.find (sep, pos1);
  if (std::string::npos != pos2) pos2 -= pos1;

  char         *end = nullptr;
  unsigned long pid = strtoul (udi.substr (pos1, pos2).c_str (), &end, 16);

  if (end && '\0' == *end && pid <= 0xFFFF)
    return pid;

  return 0;
}

static bool
is_vid_pid (const std::string& udi)
{
  return (get_vid (udi) && get_pid (udi));
}

static std::string
get_syspath (const std::string& udi)
{
  if (0 != udi.find (prefix)) return std::string ();

  std::string::size_type pos = udi.find (sep) + 1;
  // TODO cross check udi.substr (pos) with libudev?
  return udi.substr (pos);
}

bool
is_syspath (const std::string& udi)
{
  return !get_syspath (udi).empty ();
}

static bool
is_valid (const std::string& udi)
{
  return (is_vid_pid (udi) || is_syspath (udi));
}

static libusb_device_handle *
match (const udev_info& info, libusb_device *dev) noexcept
{
  require (dev);

  uint8_t bus  = libusb_get_bus_number (dev);
  uint8_t addr = libusb_get_device_address (dev);
  uint8_t port = 0;
#if HAVE_LIBUSB_GET_PORT_NUMBER
  port = libusb_get_port_number (dev);
#endif

  if (   info.usb_bus_number ()           != bus
      || info.usb_device_address ()       != addr
      || (port && info.usb_port_number () != port))
    return nullptr;

  struct libusb_device_descriptor desc;
  int err = libusb_get_device_descriptor (dev, &desc);

  if (err
      || info.usb_vendor_id ()  != desc.idVendor
      || info.usb_product_id () != desc.idProduct)
    return nullptr;

  libusb_device_handle *handle;
  err = libusb_open (dev, &handle);
  if (err)
    {
      std::clog << "libusb_open: "
                << libusb_error_name (err) << "\n";
      return nullptr;
    }

  int cur_cfg;
  err = libusb_get_configuration (handle, &cur_cfg);
  if (err)
    {
      std::clog << "libusb_get_configuration: "
                << libusb_error_name (err) << "\n";

      libusb_close (handle);
      return nullptr;
    }

  int cfg = info.usb_configuration ();
  if (cur_cfg != cfg)
    {
      err = libusb_set_configuration (handle, cfg);
      if (err)
        {
          std::clog << "libusb_set_configuration: "
                    << libusb_error_name (err) << "\n";

          libusb_close (handle);
          return nullptr;
        }
    }

  int iface = info.usb_interface ();
  err = libusb_claim_interface (handle, iface);
  if (err)
    {
      std::clog << "libusb_claim_interface: "
                << libusb_error_name (err) << "\n";

      libusb_close (handle);
      return nullptr;
    }

  err = libusb_get_configuration (handle, &cur_cfg);
  if (err)
    {
      std::clog << "libusb check configuration: "
                << libusb_error_name (err) << "\n";

      libusb_release_interface (handle, iface);
      libusb_close (handle);
      return nullptr;
      }

  libusb_release_interface (handle, iface);

  if (cur_cfg != cfg)
    {
      std::clog << "libusb wrong configuration\n";
      libusb_close (handle);
      return nullptr;
    }

  return handle;
}

libusb_device_handle *
open_device_with_syspath (libusb_context *ctx, const char *path)
{
  require (ctx && path);

  libusb_device_handle *handle = nullptr;

  libusb_device **devices;
  ssize_t         cnt = libusb_get_device_list (ctx, &devices);
  udev_info       info (path);

  for (ssize_t i = 0; !handle && i < cnt; ++i)
    {
      handle = match (info, devices[i]);
    }
  libusb_free_device_list (devices, 1);

  return handle;
}

int
set_bulk_endpoints (libusb_device_handle *handle)
{
  require (handle);
  assert  (-1 == bulk_endpoint_i_ && -1 == bulk_endpoint_o_);

  libusb_device *dev = libusb_get_device (handle);

  struct libusb_config_descriptor *cfg;
  int err = libusb_get_active_config_descriptor (dev, &cfg);
  if (0 != err)
  {
    std::clog << "libusb_get_active_config_descriptor: "
              << libusb_error_name (err) << "\n";
    return err;
  }

  const struct libusb_interface_descriptor *id = nullptr;
  const struct libusb_endpoint_descriptor *ep = nullptr;
  for (int alt = 0; alt < cfg->interface[interface_].num_altsetting; ++alt)
    {
      id = &cfg->interface[interface_].altsetting[alt];

      for (int n = 0; n < id->bNumEndpoints; ++n)
        {
          ep = &id->endpoint[n];

          if (LIBUSB_TRANSFER_TYPE_BULK
              == (LIBUSB_TRANSFER_TYPE_MASK & ep->bmAttributes))
            {
              if (LIBUSB_ENDPOINT_IN
                  == (LIBUSB_ENDPOINT_DIR_MASK & ep->bEndpointAddress))
                bulk_endpoint_i_ = ep->bEndpointAddress;
              else
                bulk_endpoint_o_ = ep->bEndpointAddress;
            }
        }
    }
  libusb_free_config_descriptor (cfg);

  if (-1 != bulk_endpoint_i_ && -1 != bulk_endpoint_o_)
    return 0;

  bulk_endpoint_i_ = -1;
  bulk_endpoint_o_ = -1;

  return LIBUSB_ERROR_NOT_FOUND;
}

static int
usb_transfer (const char *func, int endpoint, void *buffer, int length)
{
  unsigned char *buf = reinterpret_cast< unsigned char *> (buffer);

  int transferred;
  int err = libusb_bulk_transfer (handle_, endpoint, buf, length,
                                  &transferred, timeout_);

  if (transferred != length)
    {
      std::clog << func << ": transferred " << transferred
                << " of " << length << " bytes\n";
    }

  if (LIBUSB_ERROR_PIPE == err)
    {
      std::clog << "clearing halt: " << libusb_error_name (err) << "\n";
      err = libusb_clear_halt (handle_, endpoint);
    }

  if (err)
    {
      std::clog << "usb_transfer: " << libusb_error_name (err) << "\n";
    }

  return transferred;
}

static int
usb_reader (void *buffer, int length)
{
  return usb_transfer (__func__, bulk_endpoint_i_, buffer, length);
}

static int
usb_writer (void *buffer, int length)
{
  return usb_transfer (__func__, bulk_endpoint_o_, buffer, length);
}

static void
usb_teardown ()
{
  if (handle_ && interface_)
    {
      libusb_release_interface (handle_, interface_);
      interface_       =  0;
      bulk_endpoint_i_ = -1;
      bulk_endpoint_o_ = -1;
    }
  if (handle_)
    {
      libusb_close (handle_);
      handle_ = nullptr;
    }
  if (context_)
    {
      libusb_exit (context_);
      context_ = nullptr;
    }
}

usb_handle::usb_handle (const std::string& udi)
{
  if (!is_valid (udi))
    throw std::invalid_argument
      (std::string ("machine: invalid UDI '") + udi + "'");

  if (context_)
    throw std::runtime_error
      ("multiple, simultaneous devices not supported");

  int err = libusb_init (&context_);
  if (err)
    {
      context_ = nullptr;
      throw std::runtime_error
        (std::string ("libusb_init: ") + libusb_error_name (err));
    }

  if (is_vid_pid (udi))
    {
      uint16_t vid = get_vid (udi);
      uint16_t pid = get_pid (udi);

      assert (vid && pid);

      handle_ = libusb_open_device_with_vid_pid (context_, vid, pid);
      // If we got a handle_ here, device configuration and default
      // interface_ value assumed to be correct.
    }
  else
    {
      std::string syspath = get_syspath (udi);

      assert (!syspath.empty ());

      handle_ = open_device_with_syspath (context_, syspath.c_str ());
      // If we got a handle_ here, configuration and interface_ have
      // been properly configured.
    }

  if (handle_)
    {
      err = libusb_claim_interface (handle_, interface_);
      if (!err)
        {
          err = set_bulk_endpoints (handle_);
          if (err)
            libusb_release_interface (handle_, interface_);
        }
      if (err)
        {
          std::clog << libusb_error_name (err) << "\n";
          interface_ = 0;
          libusb_close (handle_);
          handle_ = nullptr;
        }
    }

  if (!handle_)
    {
      libusb_exit (context_);
      context_ = nullptr;
      throw std::runtime_error
        ("no matching device: " + udi);
    }

  if (interpreter_ctor_(usb_reader, usb_writer))
    {
      promise (handle_);
    }
  else
    {
      usb_teardown ();
      throw std::runtime_error
        ("failed to initialize interpreter");
    }
}

usb_handle::~usb_handle ()
{
  interpreter_dtor_();
  usb_teardown ();
}
