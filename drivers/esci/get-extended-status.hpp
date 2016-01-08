//  get-extended-status.hpp -- query for device status
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

#ifndef drivers_esci_get_extended_status_hpp_
#define drivers_esci_get_extended_status_hpp_

#include "bounding-box.hpp"
#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  A more extensive status (and capability) query.
    /*!  Building on the get_status() command, this command retrieves
         a second block of information from the device that augments
         the single byte of information of the first block.  Although
         technically the size of the second block is variable, only a
         42 byte block reply has been documented.

         The reply contains a mixed-bag of status and capability which
         has been exposed through the class' public API.  Most notable
         amongst the API entries are the queries that tell whether the
         device has an automatic document feeder (ADF) or transparency
         unit (TPU), whether any of these options have been enabled as
         the result of sending a set_option_unit() command and what
         their respective maximum scan areas are.

         There is also API to retrieve the product_name(), many of the
         ADF characteristics as well as option status.

         \note  Despite the command's name, this is \e not one of the
                extended commands.  See get_scanner_status for that.

         \sa adf_detected(), tpu_detected(), adf_enabled(), tpu_enabled(),
             scan_area(), product_name()
     */
    class get_extended_status : public buf_getter<ESC,LOWER_F>
    {
    public:
      //!  \copybrief buf_getter::buf_getter
      /*!  \copydetails buf_getter::buf_getter
       */
      get_extended_status (bool pedantic = false);

      //!  Reports the device's product name.
      /*!  Also known as the firmware name, it provides a unique
           hardware interface independent handle for a device.

           \note  The product name is not necessarily the same as
                  the name under which the device is marketed.
       */
      std::string product_name (void) const;
      //!  Reports the device's type.
      /*!  There are two documented return values, 0 and 3.  Devices
           of type 3 return media out, media jam and cover open
           status via the corresponding \c main_* functions, whereas
           type 1 devices should be queried via the \c adf_* and/or
           \c tpu_* ones.

           \todo  Provide a unified interface to these statusses.
       */
      uint8_t device_type (void) const;

      //!  Tells whether a \a source may be able to detect media size.
      /*!  If this function returns \c true, media_value() can be used
           to find out what size the device detected.
       */
      bool supports_size_detection (const source_value& source) const;

      //!  Yields the detected media value for a \a source.
      /*!  Some devices can detect media sizes on the fly.  When that
           is the case this function returns a non-zero value.  Note,
           however, that that may still return ::UNKNOWN when it sees
           odd-sized media.

           \sa ::media_value
       */
      uint16_t media_value (const source_value& source) const;

      //!  Tells whether the device is a flatbed type scanner.
      /*!  If \c true, the device's main source is the flatbed.  In
           case the function returns \c false it is a holder type
           scanner.
       */
      bool is_flatbed_type (void) const;
      //!  Tells whether the device has a lid type option unit.
      /*!  If \c true, the option unit is integrated in the device's
           lid.
       */
      bool has_lid_option (void) const;
      //!  Tells whether the device has a push button.
      /*!  \sa get_push_button_status
       */
      bool has_push_button (void) const;

      //!  Indicates whether a fatal error has occurred.
      /*!  The error condition may have happened in the device's main
           body or in the option unit.
       */
      bool fatal_error (void) const;
      //!  Indicates whether the device's lamp is warming up.
      /*!  \sa cancel_warming_up
       */
      bool is_warming_up (void) const;

      //!  Says whether an error has been detected by the main body.
      /*!  This function returns \c true if any of main_media_out(),
           main_media_jam() and main_cover_open() return \c true.
           Note that main_media_out() is not really an error
           condition, though.

           \sa device_type()
       */
      bool main_error (void) const;
      //!  Indicates whether the main body detected an out of media.
      /*!  \copydetails adf_media_out
       */
      bool main_media_out (void) const;
      //!  Indicates whether the main body detected a jam.
      /*!  \copydetails adf_media_jam
       */
      bool main_media_jam (void) const;
      //!  Indicates whether the main body's cover is open.
      /*!  The cover needs to be closed before the device can scan.
       */
      bool main_cover_open (void) const;

      //!  Indicates whether an ADF unit is available.
      /*!  If \c true, ::ADF_SIMPLEX can be used as an option_value.
           \sa adf_is_duplex_type()
       */
      bool adf_detected (void) const;
      //!  Indicates whether the ADF unit puts media on the glass plate.
      /*!  If \c true, media is put on the glass plate before the scan
           starts.  If \c false, media will be transported along the
           sensor during the scan.
       */
      bool adf_is_page_type (void) const;
      //!  Indicates whether the ADF unit can be used in duplex mode.
      /*!  If \c true, ::ADF_DUPLEX can be used as an option_value.
       */
      bool adf_is_duplex_type (void) const;
      //!  Indicates which sheet an ADF unit loads.
      /*!  ADF units may load media from the top of the stack or from
           the bottom of the stack.  This query returns \c true for top
           loading ADF units.
       */
      bool adf_is_first_sheet_loader (void) const;
      //!  Indicates whether the ADF unit is active.
      /*!  The next scan will be done using the ADF unit.
       */
      bool adf_enabled (void) const;
      //!  Says whether an error has been detected by the ADF unit.
      /*!  This function returns \c true if any of kind of ADF related
           error condition has been detected.  It also returns \c true
           when adf_media_out() was detected even though that is not
           really an error.
       */
      bool adf_error (void) const;
      //!  Indicates whether the ADF unit detected a double feed error.
      /*!  If \c true, the ADF unit suspects more than a single sheet
           entered the unit.

           \sa set_scan_parameters::double_feed_sensitivity(),
               scan_parameters::double_feed_sensitivity()
       */
      bool adf_double_feed (void) const;
      //!  Indicates whether the ADF unit ran out of media.
      /*!  If \c true, the ADF unit can no longer load media and the
           device stops scanning.
       */
      bool adf_media_out (void) const;
      //!  Indicates whether the ADF unit has jammed.
      /*!  If \c true, the jam needs to be cleared before the device
           can be used again.
       */
      bool adf_media_jam (void) const;
      //!  Indicates whether (one of) the ADF unit's cover(s) is open.
      /*!  ADF units may have several covers and if one of them is not
           closed, this function returns \c true.  All covers need to
           be closed before the device can scan.
       */
      bool adf_cover_open (void) const;
      //!  Indicates whether the ADF unit is set to use duplex mode.
      /*!  If \c true, ::ADF_DUPLEX has been set as the option_value.
       */
      bool adf_is_duplexing (void) const;

      //!  Indicates whether a transparency unit is available.
      /*!  If \c true, ::TPU_AREA_1 can be used as an option_value.
       */
      bool tpu_detected (void) const;
      //!  Indicates whether the transparency unit is active.
      /*!  If \c true, the next scan will use the TPU.
       */
      bool tpu_enabled (void) const;
      //!  Says whether an error has been detected by the TPU.
      /*!  This function returns \c true if any kind of TPU related
           error condition has been detected.
       */
      bool tpu_error (void) const;
      //!  Indicates whether the TPU cover is open.
      /*!  The cover needs to be closed before the device can scan.
       */
      bool tpu_cover_open (void) const;

      //!  Yields the maximum scan area for a \a source in pixels.
      /*!  Use the maximum resolution from get_identity to convert to
           inches.
       */
      bounding_box<uint32_t>
      scan_area (const source_value& source = MAIN) const;

    protected:
      void check_data_block (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_extended_status_hpp_ */
