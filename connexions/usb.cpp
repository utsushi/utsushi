//  usb.cpp -- shuttle messages between software and USB device
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2011, 2015  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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

#if  HAVE_LIBUSB
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
#endif  /*  HAVE_LIBUSB */

extern "C" {

  void
  libcnx_usb_LTX_factory (connexion::ptr& cnx,
                          const std::string& iftype, const std::string& path)
  {
    device_info::ptr dev = device_info::create (iftype, path);

    if (!dev) return;

#if HAVE_LIBUSB
    cnx = make_shared< usb > (dev);
#else
    log::alert ("USB support disabled at compile time");
#endif
  }
}

using std::runtime_error;

#if HAVE_LIBUSB

  const int milliseconds =    1;
  const int seconds      = 1000 * milliseconds;
  const int minutes      =   60 * seconds;

  bool usb::is_initialised_  = false;
  int  usb::default_timeout_ = 5 * minutes;
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
              (runtime_error ("unable to initialise USB support"));
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
        (runtime_error ("no usable, matching device"));

    ++connexion_count_;
  }

  usb::~usb (void)
  {
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
    return send (message, size, 0.001 * default_timeout_);
  }

  void
  usb::send (const octet *message, streamsize size, double timeout)
  {
    unsigned char *buf = reinterpret_cast<unsigned char *>
      (const_cast<octet *> (message));

    int transferred;
    int err = libusb_bulk_transfer (handle_, ep_bulk_o_, buf, size,
                                    &transferred, 1000 * timeout);

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
  usb::recv (octet *message, streamsize size)
  {
    return recv (message, size, 0.001 * default_timeout_);
  }

  void
  usb::recv (octet *message, streamsize size, double timeout)
  {
    unsigned char *buf = reinterpret_cast<unsigned char *> (message);

    int transferred;
    int err = libusb_bulk_transfer (handle_, ep_bulk_i_, buf, size,
                                    &transferred, 1000 * timeout);

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
    if (device->usb_bus_number () != libusb_get_bus_number (dev))
      return NULL;

#if HAVE_LIBUSB_GET_PORT_NUMBER
    if (libusb_get_port_number (dev)
        && device->usb_port_number () != libusb_get_port_number (dev))
      return NULL;
#endif

    if (device->usb_device_address () != libusb_get_device_address (dev))
      return NULL;

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

} // namespace _cnx_
} // namespace utsushi
