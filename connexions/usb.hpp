//  usb.hpp -- shuttle messages between software and USB device
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

#ifndef connexions_usb_hpp_
#define connexions_usb_hpp_

#if HAVE_LIBUSB
#include <libusb.h>
#endif

#if HAVE_LIBUSB_LEGACY
#include <usb.h>
#endif

#include <string>

#include <utsushi/connexion.hpp>
#include <utsushi/device-info.hpp>

namespace utsushi {

extern "C" {
  void libcnx_usb_LTX_factory (connexion::ptr& cnx,
                               const std::string& type,
                               const std::string& path);
}

namespace _cnx_ {

#if HAVE_LIBUSB

  class usb : public connexion
  {
  public:
    usb (const device_info::ptr& device);

    virtual ~usb (void);

    virtual void send (const octet *message, streamsize size);
    virtual void recv (      octet *message, streamsize size);

  private:
    libusb_device_handle * usable_match_(const device_info::ptr& device,
                                         libusb_device *dev);
    bool set_bulk_endpoints_(libusb_device *dev);
    bool serial_number_matches_(uint8_t index,
                                const std::string& serial_number) const;

    libusb_device_handle *handle_;
    int cfg_;
    int if_;
    int ep_bulk_i_;
    int ep_bulk_o_;

    static bool is_initialised_;
    static int  default_timeout_;

    static libusb_context *ctx_;
    static int connexion_count_;
  };

#endif  /* HAVE_LIBUSB */

#if HAVE_LIBUSB_LEGACY

  class usb_legacy : public connexion
  {
  public:
    usb_legacy (const device_info::ptr& device);

    virtual ~usb_legacy (void);

    virtual void send (const octet *message, streamsize size);
    virtual void recv (      octet *message, streamsize size);

  private:
    usb_dev_handle * usable_match_(const device_info::ptr& device,
                                   struct usb_device *dev);
    bool set_bulk_endpoints_(struct usb_device *dev);

    usb_dev_handle *handle_;
    int cfg_;
    int if_;
    int ep_bulk_i_;
    int ep_bulk_o_;

    static bool is_initialised_;
    static int  default_timeout_;
  };

#endif  /* HAVE_LIBUSB_LEGACY */

} // namespace _cnx_
} // namespace utsushi

#endif  /* connexions_usb_hpp_ */
