//  start-scan.hpp -- to acquire image data
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008  Olaf Meeuwissen
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

#ifndef drivers_esci_start_scan_hpp_
#define drivers_esci_start_scan_hpp_

#include "chunk.hpp"
#include "command.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Acquiring image data.
    /*!  The common API bits for start scan commands is implemented by
         this class.  Unlike the bulk of the implemented commands, the
         handshake is split over \e two member functions.  This allows
         for repeatedly fetching of image data chunks.

         The implementations ensure that replies upon receipt of image
         data are sent when necessary.  They also deal with the timing
         issues of scan cancellation.
     */
    class start_scan : public command
    {
    public:
      //!  Initiates image data acquisition.
      /*!  This override merely sends the command bytes to the device.
           In order to correctly complete the "handshake" one should
           keep calling operator++() until it returns an empty chunk.
       */
      virtual void operator>> (connexion& cnx) = 0;

      //!  Acquires and returns the next chunk of image data.
      /*!  This member implements the remainder of the "handshake".
           It will correctly acknowledge receipt of image data and
           cancel() when that has been requested.

           The function returns an empty chunk when all image data has
           been acquired or the acquisition process was successfully
           cancelled.
       */
      virtual chunk operator++ (void) = 0;

      //!  Tells whether the device detected a fatal error.
      /*!  When this function returns \c true something has gone very
           wrong.  The get_extended_status API may be useful in trying
           to find out more precisely what went wrong.  For a device
           that supports_extended_commands() get_scanner_status may be
           a better choice though.

           \sa buf_getter::detected_fatal_error()
       */
      virtual bool detected_fatal_error (void) const = 0;

      //!  Tells whether the device is ready to start a scan.
      /*!  A device that is not ready to start a scan is normally in
           use by someone else.  For example, somebody may be making
           copies on a multi-function device or a device is accessed
           via its network interface.

           \sa buf_getter::is_ready()
       */
      virtual bool is_ready (void) const = 0;

      //!  Requests cancellation of a scan.
      /*!  This member function only signals the request.  It does \e
           not cause the scan to cancel.  The scan will cancel at the
           next convenient moment during an operator++() call.

           The optional \a at_area_end argument may be used to request
           cancellation at end-of-medium detection if supported.
       */
      virtual void cancel (bool at_area_end = false) = 0;

    protected:
      start_scan (bool pedantic = false)
        : pedantic_(pedantic)
        , cnx_(NULL), do_cancel_(false), cancelled_(false)
      {}

      bool pedantic_;           //!<  checking of replies or not

      connexion *cnx_;          //!<  where to get image data from
      bool do_cancel_;          //!<  should acquisition be aborted
      bool cancelled_;          //!<  has acquisition been aborted
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_start_scan_hpp_ */
