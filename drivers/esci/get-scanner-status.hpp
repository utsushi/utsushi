//  get-scanner-status.hpp -- query for device status
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2009  Olaf Meeuwissen
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

#ifndef drivers_esci_get_scanner_status_hpp_
#define drivers_esci_get_scanner_status_hpp_

#include "getter.hpp"

#include <utsushi/media.hpp>

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  A more extensive status query.
    /*!  One of the extended commands, this command provides access to
         a lot of a device's status.  Unlike the get_extended_status
         command, this command is more true to its name and primarily
         limits itself to providing status.  The bulk of the device's
         capabilities can be retrieved via the get_extended_identity
         command instead.

         Most of the information is encoded in the form of bit flags
         and has been made available through boolean queries.  All of
         the status API has been designed to match the corresponding
         API in the get_extended_status class exactly.

         \sa adf_detected(), tpu_detected(), adf_enabled(), tpu_enabled()
     */
    class get_scanner_status : public getter<FS,UPPER_F,16>
    {
    public:
      //!  \copybrief getter::getter
      /*!  \copydetails getter::getter
       */
      get_scanner_status (bool pedantic = false);

      //!  \copybrief get_extended_status::device_type
      /*!  \copydetails get_extended_status::device_type
       */
      uint8_t device_type (void) const;

      //!  \copybrief get_extended_status::supports_size_detection
      /*!  \copydetail get_extended_status::supports_size_detection
       */
      bool supports_size_detection (const source_value& source) const;

      bool media_size_detected (const source_value& source) const;

      media media_size (const source_value& source) const;

      //!  \copybrief get_extended_status::media_value
      /*!  \copydetails get_extended_status::media_value
       */
      uint16_t media_value (const source_value& source) const;

      //!  \copybrief get_extended_status::fatal_error
      /*!  \copydetails get_extended_status::fatal_error
       */
      bool fatal_error (void) const;

      //!  \copybrief get_extended_status::is_ready
      /*!  \copydetails get_extended_status::is_ready
       */
      bool is_ready (void) const;

      //!  \copybrief get_extended_status::is_warming_up
      /*!  \copydetails get_extended_status::is_warming_up
       */
      bool is_warming_up (void) const;

      //!  Indicates whether lamp warming up can be cancelled
      /*!  If \c true the cancel_warming_up command may be sent to the
           device.
       */
      bool can_cancel_warming_up (void) const;

      //!  \copybrief get_extended_status::main_error
      /*!  \copydetails get_extended_status::main_error
       */
      bool main_error (void) const;

      //!  \copybrief get_extended_status::main_media_out
      /*!  \copydetails get_extended_status::main_media_out
       */
      bool main_media_out (void) const;

      //!  \copybrief get_extended_status::main_media_jam
      /*!  \copydetails get_extended_status::main_media_jam
       */
      bool main_media_jam (void) const;

      //!  \copybrief get_extended_status::main_cover_open
      /*!  \copydetails get_extended_status::main_cover_open
       */
      bool main_cover_open (void) const;

      //!  \copybrief get_extended_status::adf_detected
      /*!  \copydetails get_extended_status::adf_detected
       */
      bool adf_detected (void) const;

      //!  \copybrief get_extended_status::adf_enabled
      /*!  \copydetails get_extended_status::adf_enabled
       */
      bool adf_enabled (void) const;

      //!  \copybrief get_extended_status::adf_error
      /*!  \copydetails get_extended_status::adf_error
       */
      bool adf_error (void) const;

      //!  \copybrief get_extended_status::adf_double_feed
      /*!  \copydetail get_extended_status::adf_double_feed
       */
      bool adf_double_feed (void) const;

      //!  \copybrief get_extended_status::adf_media_out
      /*!  \copydetails get_extended_status::adf_media_out
       */
      bool adf_media_out (void) const;

      //!  \copybrief get_extended_status::adf_media_jam
      /*!  \copydetails get_extended_status::adf_media_jam
       */
      bool adf_media_jam (void) const;

      //!  \copybrief get_extended_status::adf_cover_open
      /*!  \copydetails get_extended_status::adf_cover_open
       */
      bool adf_cover_open (void) const;

      //!  Indicates whether the ADF unit's tray is open.
      /*!  If the tray is not closed, this function returns \c true.
           The tray needs to be closed before the device can scan.
       */
      bool adf_tray_open (void) const;

      //!  \copybrief get_extended_status::adf_is_duplexing
      /*!  \copydetails get_extended_status::adf_is_duplexing
       */
      bool adf_is_duplexing (void) const;

      //!  \copybrief get_extended_status::tpu_detected
      /*!  \copydetails get_extended_status::tpu_detected
           An optional \a source argument may be passed to explicitly
           select between primary and secondary transparency units.
       */
      bool tpu_detected (const source_value& source = TPU1) const;

      //!  \copybrief get_extended_status::tpu_enabled
      /*!  \copydetails get_extended_status::tpu_enabled
           An optional \a source argument may be passed to explicitly
           select between primary and secondary transparency units.

           If this function returns \c true for a \a source of ::TPU2,
           then ::TPU_AREA_2 may be used as an option_value.

           \sa get_extended_identity::tpu_is_IR_type()
       */
      bool tpu_enabled (const source_value& source = TPU1) const;

      //!  \copybrief get_extended_status::tpu_error
      /*!  \copydetails get_extended_status::tpu_error
           An optional \a source argument may be passed to explicitly
           select between primary and secondary transparency units.
       */
      bool tpu_error (const source_value& source = TPU1) const;

      //!  \copybrief get_extended_status::tpu_cover_open
      /*!  \copydetails get_extended_status::tpu_cover_open
           An optional \a source argument may be passed to explicitly
           select between primary and secondary transparency units.
       */
      bool tpu_cover_open (const source_value& source = TPU1) const;

      //!  Indicates trouble with the TPU's lamp.
      /*!  An optional \a source argument may be passed to explicitly
           select between primary and secondary transparency units.
       */
      bool tpu_lamp_error (const source_value& source = TPU1) const;

      //!  Indicates whether the device has support for a holder.
      bool has_holder_support (void) const;

      //!  Tells whether a holder error was detected.
      /*!  This returns true if the holder is empty or some other kind
           of holder related error has occurred.
       */
      bool holder_error (void) const;

      //!  Yields the type of holder detected.
      /*!  A value of \c 0 indicates no holder was detected (assuming
           that the device supports holders in the first place).

           \sa has_holder_support()
       */
      byte holder_type (void) const;

    protected:
      //!  Implements \a source value checking.
      bool tpu_status_(const source_value& source, byte mask) const;

      void check_blk_reply (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_scanner_status_hpp_ */
