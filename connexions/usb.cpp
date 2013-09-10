//  usb.cpp -- shuttle messages between software and USB device
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
//  Copyright (C) 2011  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#include <iostream>
#include <stdexcept>

#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>

#include "usb.hpp"

namespace utsushi {
namespace _cnx_ {

#if !HAVE_LIBUSB_ERROR_NAME
#include <sstream>
#include <string>

static std::string
libusb_error_name (int err)
{
  std::ostringstream os;
  os << err;
  return os.str ();
}
#endif  /* !HAVE_LIBUSB_ERROR_NAME */

extern "C" {

  connexion::ptr
  libcnx_usb_LTX_factory (const std::string& iftype, const std::string& path)
  {
    device_info::ptr dev = device_info::create (iftype, path);

    if (!dev) return connexion::ptr ();

#if HAVE_LIBUSB
    return make_shared< usb > (dev);
#elif HAVE_LIBUSB_LEGACY
    return make_shared< usb_legacy > (dev);
#else
    log::alert ("USB support disabled at compile time");
    return connexion::ptr ();
#endif
  }
}

using std::runtime_error;

#if HAVE_LIBUSB

  bool usb::is_initialised_  = false;
  int  usb::default_timeout_ = 30000; // milliseconds
  libusb_context *usb::ctx_  = 0;
  int usb::connexion_count_  = 0;

  usb::usb (const device_info::ptr& device)
    : handle_(0), cfg_(-1), if_(-1), ep_bulk_i_(-1), ep_bulk_o_(-1)
  {
    if (!is_initialised_)
      {
        int err = libusb_init (&ctx_);
        is_initialised_ = !err;

        if (err)
          {
            ctx_ = 0;
            log::error (libusb_error_name (err));
            BOOST_THROW_EXCEPTION
              (runtime_error (_("unable to initialise USB support")));
          }
        libusb_set_debug (ctx_, 3);
      }

    libusb_device **haystack;
    ssize_t cnt = libusb_get_device_list (ctx_, &haystack);

    for (ssize_t i = 0; !handle_ && i < cnt; i++)
      {
        handle_ = usable_match_(device, haystack[i]);
      }

    libusb_free_device_list (haystack, 1);

    if (!handle_)
      BOOST_THROW_EXCEPTION
        (runtime_error (_("no usable, matching device")));

    libusb_clear_halt (handle_, ep_bulk_o_);
    libusb_clear_halt (handle_, ep_bulk_i_);

    ++connexion_count_;
  }

  usb::~usb (void)
  {
    libusb_clear_halt (handle_, ep_bulk_o_);
    libusb_clear_halt (handle_, ep_bulk_i_);
    libusb_release_interface (handle_, if_);
    libusb_close (handle_);

    if (0 == --connexion_count_)
      {
        libusb_exit (ctx_);
        ctx_ = 0;
        is_initialised_ = false;
      }
  }

  void
  usb::send (const octet *message, streamsize size)
  {
    unsigned char *buf = reinterpret_cast<unsigned char *>
      (const_cast<octet *> (message));

    int transferred;
    int err = libusb_bulk_transfer (handle_, ep_bulk_o_, buf, size,
                                    &transferred, default_timeout_);

    if (LIBUSB_ERROR_PIPE == err)
      err = libusb_clear_halt (handle_, ep_bulk_o_);

    if (err)
      {
        log::error (libusb_error_name (err));
        BOOST_THROW_EXCEPTION
          (runtime_error (libusb_error_name (err)));
      }
  }

  void
  usb::recv (      octet *message, streamsize size)
  {
    unsigned char *buf = reinterpret_cast<unsigned char *> (message);

    int transferred;
    int err = libusb_bulk_transfer (handle_, ep_bulk_i_, buf, size,
                                    &transferred, default_timeout_);

    if (LIBUSB_ERROR_PIPE == err)
      err = libusb_clear_halt (handle_, ep_bulk_i_);

    if (err)
      {
        log::error (libusb_error_name (err));
        BOOST_THROW_EXCEPTION
          (runtime_error (libusb_error_name (err)));
      }
  }

  libusb_device_handle *
  usb::usable_match_(const device_info::ptr& device, libusb_device *dev)
  {
    struct libusb_device_descriptor descriptor;

    int err = libusb_get_device_descriptor (dev, &descriptor);

    if (err
        || device->usb_vendor_id ()  != descriptor.idVendor
        || device->usb_product_id () != descriptor.idProduct)
      return NULL;

    err = libusb_open (dev, &handle_);
    if (err)
      {
        log::error ("%1%: open: %2%")
          % __func__
          % libusb_error_name (err);
        return NULL;
      }

    if (!serial_number_matches_(descriptor.iSerialNumber,
                                device->usb_serial ()))
      {
        libusb_close (handle_);
        handle_ = NULL;
        return NULL;
      }

    int current_cfg;
    err = libusb_get_configuration (handle_, &current_cfg);
    if (err)
      {
        log::error ("%1%: get configuration: %2%")
          % __func__
          % libusb_error_name (err);

        libusb_close (handle_);
        handle_ = NULL;
        return NULL;
      }

    cfg_ = device->usb_configuration ();
    if (current_cfg != cfg_)
      {
        err = libusb_set_configuration (handle_, cfg_);
        if (err)
          {
            log::error ("%1%: set configuration: %2%")
              % __func__
              % libusb_error_name (err);

            libusb_close (handle_);
            handle_ = NULL;
            return NULL;
          }
      }

    if_  = device->usb_interface ();
    err = libusb_claim_interface (handle_, if_);
    if (err)
      {
        log::error ("%1%: claim interface: %2%")
          %  __func__
          % libusb_error_name (err);

        if_ = -1;
        libusb_close (handle_);
        handle_ = NULL;
        return NULL;
      }

    err = libusb_get_configuration (handle_, &current_cfg);
    if (err)
      {
        log::error ("%1%: chk configuration: %2%")
          % __func__
          % libusb_error_name (err);

        libusb_release_interface (handle_, if_);
        if_ = -1;
        libusb_close (handle_);
        handle_ = NULL;
        return NULL;
      }

    if (current_cfg == cfg_)
      {
        if (set_bulk_endpoints_(dev))
          return handle_;   // we got a usable match!
      }
    else
      {
        log::error ("%1%: interface has wrong configuration: %2%")
          % __func__
          % cfg_;
      }

    libusb_release_interface (handle_, if_);
    if_ = -1;
    libusb_close (handle_);
    handle_ = NULL;
    return NULL;
  }

  bool
  usb::serial_number_matches_(uint8_t index,
                              const std::string& serial_number) const
  {
    if (!index || 0 == serial_number.size ()) return true;

    char buf[serial_number.size () + 1];
    buf[0] = '\0';

    unsigned char *ubuf = reinterpret_cast<unsigned char *> (buf);

    int length = libusb_get_string_descriptor_ascii
      (handle_, index, ubuf, serial_number.size() + 1);
    if (0 > length)             // it's really an error!
      log::error ("%1%: get_string_descr: %2%")
        % __func__
        % libusb_error_name (length);

    return (0 < length && buf == serial_number);
  }

  bool
  usb::set_bulk_endpoints_(libusb_device *dev)
  {
    if (!dev) return false;

    struct libusb_config_descriptor *config;

    int err = libusb_get_config_descriptor_by_value (dev, cfg_, &config);
    if (err)
      {
        return false;
      }

    for (int a = 0; a < config->interface[if_].num_altsetting; ++a)
      {
        const struct libusb_interface_descriptor *id
          = &config->interface[if_].altsetting[a];

        for (int n = 0; n < id->bNumEndpoints; ++n)
          {
            const struct libusb_endpoint_descriptor *ep = &id->endpoint[n];

            if (LIBUSB_TRANSFER_TYPE_BULK
                == (LIBUSB_TRANSFER_TYPE_MASK & ep->bmAttributes))
              {
                if (LIBUSB_ENDPOINT_DIR_MASK & ep->bEndpointAddress)
                  ep_bulk_i_ = ep->bEndpointAddress;
                else
                  ep_bulk_o_ = ep->bEndpointAddress;
              }
          }
      }
    libusb_free_config_descriptor (config);

    return (-1 != ep_bulk_i_ && -1 != ep_bulk_o_);
  }

#endif  /* HAVE_LIBUSB */

#if HAVE_LIBUSB_LEGACY

  bool usb_legacy::is_initialised_  = false;
  int  usb_legacy::default_timeout_ = 30000; // milliseconds

  usb_legacy::usb_legacy (const device_info::ptr& device)
    : handle_(NULL), cfg_(-1), if_(-1), ep_bulk_i_(-1), ep_bulk_o_(-1)
  {
    if (!is_initialised_)
      {
        usb_init ();
        is_initialised_ = true;
      }

    if (0 > usb_find_busses () || 0 > usb_find_devices ())
      {
        BOOST_THROW_EXCEPTION (runtime_error (usb_strerror ()));
      }

    struct usb_bus *busses = usb_get_busses ();

    struct usb_bus *bus = busses;
    while (!handle_ && bus)
      {
        struct usb_device *dev = bus->devices;
        while (!handle_ && dev)
          {
            handle_ = usable_match_(device, dev);
            dev = dev->next;
          }
        bus = bus->next;
      }
    if (!handle_)
      {
        BOOST_THROW_EXCEPTION
          (runtime_error (_("no usable, matching device")));
      }

    usb_clear_halt (handle_, ep_bulk_o_);
    usb_clear_halt (handle_, ep_bulk_i_);
  }

  usb_legacy::~usb_legacy (void)
  {
    usb_clear_halt (handle_, ep_bulk_o_);
    usb_clear_halt (handle_, ep_bulk_i_);
    usb_release_interface (handle_, if_);
    usb_close (handle_);
  }

  void
  usb_legacy::send (const octet *message, streamsize size)
  {
    char *buf = reinterpret_cast<char *> (const_cast<octet *> (message));

    if (0 > usb_bulk_write (handle_, ep_bulk_o_, buf, size, default_timeout_))
      {
        BOOST_THROW_EXCEPTION (runtime_error (usb_strerror ()));
      }
  }

  void
  usb_legacy::recv (      octet *message, streamsize size)
  {
    char *buf = reinterpret_cast<char *> (message);

    if (0 > usb_bulk_read (handle_, ep_bulk_i_, buf, size, default_timeout_))
      {
        BOOST_THROW_EXCEPTION (runtime_error (usb_strerror ()));
      }
  }

  usb_dev_handle *
  usb_legacy::usable_match_(const device_info::ptr& device,
                            struct usb_device *dev)
  {
    if (   device->usb_vendor_id ()  != dev->descriptor.idVendor
        || device->usb_product_id () != dev->descriptor.idProduct)
      return NULL;

    handle_ = usb_open (dev);
    if (!handle_)
      {
        log::error (usb_strerror ());
        return NULL;
      }

    std::string serial = device->usb_serial ();
    char   buf[serial.size () + 1];

    usb_get_string_simple (handle_, dev->descriptor.iSerialNumber, buf,
                           serial.size () + 1);

    if (buf == serial)
      {
        cfg_ = device->usb_configuration ();
        if_  = device->usb_interface ();

        if (0 > usb_set_configuration (handle_, cfg_))
          log::error (usb_strerror ());

        if (0 == usb_claim_interface (handle_, if_))
          {
            if (set_bulk_endpoints_(dev))
              return handle_;   // we got a usable match!

            usb_release_interface (handle_, if_);
          }

        log::error (usb_strerror ());
        if_ = -1;
      }

    if (0 > usb_close (handle_))
      log::error (usb_strerror ());
    handle_ = NULL;

    return NULL;
  }

  bool
  usb_legacy::set_bulk_endpoints_(struct usb_device *dev)
  {
    if (!dev || !dev->config) return false;

    int i = 0;
    while (i < USB_MAXCONFIG && cfg_ != dev->config[i].bConfigurationValue)
      ++i;

    if (USB_MAXCONFIG == i) return false;

    for (int a = 0; a < dev->config[i].interface[if_].num_altsetting; ++a)
      {
        struct usb_interface_descriptor *id
          = &dev->config[i].interface[if_].altsetting[a];

        for (int n = 0; n < id->bNumEndpoints; ++n)
          {
            struct usb_endpoint_descriptor *ep = &id->endpoint[n];

            if (USB_ENDPOINT_TYPE_BULK
                == (USB_ENDPOINT_TYPE_MASK & ep->bmAttributes))
              {
                int addr = USB_ENDPOINT_ADDRESS_MASK & ep->bEndpointAddress;

                if (USB_ENDPOINT_DIR_MASK & ep->bEndpointAddress)
                  ep_bulk_i_ = addr;
                else
                  ep_bulk_o_ = addr;
              }
          }
      }
    return (-1 != ep_bulk_i_ && -1 != ep_bulk_o_);
  }

#endif  /* HAVE_LIBUSB_LEGACY */

} // namespace _cnx_
} // namespace utsushi
