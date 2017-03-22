//  scanner.hpp -- interface and support classes
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

  class info;
  static ptr create (const scanner::info& info);

protected:
  scanner (connexion::ptr cnx);

  shared_ptr< connexion > cnx_;
};

//! @PACKAGE_NAME@ Device Information
/*! @PACKAGE_NAME@ device information combines a URI inspired syntax
 *  to uniquely pinpoint the device with configurable bits of mostly
 *  user-oriented information about the device.
 *
 *  The user-oriented parts include model() and vendor() names as well
 *  as a nick name() and a place for some descriptive text().  This is
 *  the kind of informal information that may help users decide which
 *  device they want to use.  Any programs require a much more formal
 *  piece of information to create a unique communication channel with
 *  a device.  @PACKAGE_NAME@ has resorted to the URI syntax for that
 *  purpose and the remainder of this section will deal with how it is
 *  used by @PACKAGE_NAME@.
 *
 *  The syntax for @PACKAGE_NAME@'s unique device identifiers (UDI's)
 *  stays as close as possible to the URI specification but there is
 *  one important difference.  Rather than using a single \em scheme,
 *  UDI's use \em two.  This comes about because access to a typical
 *  scanner device involves both a device protocol as well as a wire
 *  protocol.  Using Augmented Backus-Naur Form (ABNF), one could
 *  define:
 *  \verbatim
  UDI = scheme ":" URI
  \endverbatim
 *  with \c URI defined as per RFC 3986.  However this would introduce
 *  ambiguity as it is no longer clear which of the two schemes refers
 *  to what protocol.  In addition, information available via the wire
 *  protocol may be used to infer a device protocol, so the latter can
 *  be optional.  In rare cases, a driver may be able to establish the
 *  communication channel itself so the need for a wire protocol spec
 *  becomes optional as well.  With these considerations in mind, the
 *  UDI definition becomes:
 *  \verbatim
  UDI       = protocols ":" hier-part [ "?" query ] [ "#" fragment ]
  protocols = driver ":" connexion
            / driver ":"
            / ":" connexion
  connexion = scheme
  driver    = scheme
  \endverbatim
 *  where the \c driver and \c connexion parts refer to the device and
 *  wire protocols, respectively.
 *
 *  \sa http://tools.ietf.org/html/rfc3986
 */
class scanner::info
{
public:
  //! Create an instance from a given \a udi
  info (const std::string& udi);

  //! \name User-Oriented Information
  //! @{
  std::string name () const;
  std::string text () const;

  void name (const std::string& name);
  void text (const std::string& text);

  std::string type () const;
  std::string model () const;
  std::string vendor () const;

  void type (const std::string& type);
  void model (const std::string& model);
  void vendor (const std::string& vendor);
  //! @}

  //! \name Interface-Specific Information
  //! @{
  uint16_t usb_vendor_id () const;
  uint16_t usb_product_id () const;

  void usb_vendor_id (const uint16_t& vid);
  void usb_product_id (const uint16_t& pid);
  //! @}

  //! \name UDI Component Information
  //! @{
  std::string driver () const;
  std::string connexion () const;

  void driver (const std::string& driver);
  void connexion (const std::string& connexion);

  std::string host () const;
  std::string port () const;
  std::string path () const;
  std::string query () const;
  std::string fragment () const;
  //! @}

  //! Obtain a unique device identifier string
  std::string udi () const;

  //! Check whether the device protocol part has been specified
  /*! Without a \c driver specified there is no way the application
   *  can intelligently communicate with the device.
   */
  bool is_driver_set () const;

  //! See if the device is attached to the machine we are running on
  /*! Returns \c true if the \c hier-part starts with two slashes.  A
   *  valid UDI with such a \c hier-part has a non-empty \c authority
   *  part and therefore a non-empty \c host specification.
   *
   *  \todo Decide whether \c localhost UDI's should return true.
   *  \todo Decide what to require for drivers that establish the
   *        communication channel themselves.
   */
  bool is_local () const;

  //! check same usb device
  bool is_same_usb_device (const uint16_t& vid, const uint16_t& pid) const;

  //! set debug mode
  void enable_debug (const bool debug);
  //! check debug mode
  bool enable_debug () const;

  //! Check whether a given \a udi is syntactically valid
  /*! \todo Implement parsing of the part that follows \c protocols
   */
  static bool is_valid (const std::string& udi);

  //! Character used to separate \c protocols and \c hier-part
  static const char separator;

  bool operator== (const scanner::info& rhs) const;

private:

  std::string udi_;

  std::string name_;
  std::string text_;

  std::string type_;
  std::string model_;
  std::string vendor_;

  uint16_t usb_vendor_id_;
  uint16_t usb_product_id_;

  bool dump_connexion_;
};

}       // namespace utsushi

namespace std {

template <>
struct less<utsushi::scanner::info>
{
  bool operator() (const utsushi::scanner::info& x,
                   const utsushi::scanner::info& y) const
  {
    return x.udi () < y.udi ();
  }
};

}       // namespace std

#endif  /* utsushi_scanner_hpp_ */
