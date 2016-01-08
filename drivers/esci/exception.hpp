//  exception.hpp -- classes for the ESC/I driver
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

#ifndef drivers_esci_exception_hpp_
#define drivers_esci_exception_hpp_

#include <exception>
#include <stdexcept>
#include <string>

#include <utsushi/i18n.hpp>

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    using std::domain_error;
    using std::logic_error;
    using std::range_error;
    using std::runtime_error;

    class exception : public std::exception
    {
    public:
      exception (const std::string& message = std::string ())
        : message_(message)
      {}

      ~exception () throw ()
      {}

      const char *
      what () const throw ()
      {
        return message_.c_str ();
      }

    protected:
      std::string message_;
    };

    //!  \todo Implement and document
    class invalid_parameter : public exception
    {
    public:
      invalid_parameter (const std::string& message
                         = CCB_N_("invalid parameter"))
        : exception (message)
      {}
    };

    //!  \todo Implement and document
    class unknown_reply : public exception
    {
    public:
      unknown_reply (const std::string& message
                     = CCB_N_("unknown reply"))
        : exception (message)
      {}
    };

    //!  \todo Implement and document
    class invalid_command : public exception
    {
    public:
      invalid_command (const std::string& message
                       = CCB_N_("invalid command"))
        : exception (message)
      {}
    };

    //!  \todo Implement and document
    class device_busy : public exception
    {
    public:
      device_busy (const std::string& message
                   = SEC_N_("device busy"))
        : exception (message)
      {}
    };

    //!  \todo Implement and document
    class protocol_error : public runtime_error
    {
    public:
      protocol_error (const std::string& message
                      = CCB_N_("protocol error"))
        : runtime_error (message)
      {}
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_exception_hpp_ */
