//  get-extended-identity.hpp -- probe for capabilities
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

#ifndef drivers_esci_get_extended_identity_hpp_
#define drivers_esci_get_extended_identity_hpp_

#include "bounding-box.hpp"
#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  I/O directions on the device side.
    enum io_direction {
      INPUT,                    //!<  to the device (from the sensor?)
      OUTPUT,                   //!<  from the device to the driver
    };

    //!  A more extensive capability query.
    /*!  One of the extended commands, this command provides access to
         a lot of the device's capabilities.  This covers some of the
         information also available via the get_extended_status and
         get_identity commands as well as some information specific to
         this command.

         Some of the information available:
         - command_level()
         - product_name() and rom_version()
         - base, minimum and maximum scan resolutions
         - maximum supported scan areas for flatbed, ADF and TPU
         - maximum supported scan width

         Most integer values are encoded in sequences of four bytes
         (as opposed to the two bytes of non-extended commands) where
         each following byte is more significant than the previous.

         The various scan areas are given in pixels and correlate to a
         scan area in inches by way of the base_resolution().

         \sa base_resolution(), min_resolution(), max_resolution(),
             scan_area(), max_scan_width()
     */
    class get_extended_identity : public getter<FS,UPPER_I,80>
    {
    public:
      //!  \copybrief getter::getter
      /*!  \copydetails getter::getter
       */
      get_extended_identity (bool pedantic = false);

      //!  \copybrief get_identity::command_level
      /*!  \copydetails get_identity::command_level
       */
      std::string command_level (void) const;

      //!  \copybrief get_extended_status::product_name
      /*!  \copydetails get_extended_status::product_name
       */
      std::string product_name (void) const;

      //!  Reports the firmware's version number.
      /*!  This is only really useful for documentation purposes (or
           an occasional version specific work-around).
       */
      std::string rom_version (void) const;

      //!  Reports the device's base resolution.
      /*!  This is the resolution that converts a scan_area() in pixels
           into one in inches.

           \note  The get_hardware_property::base_resolution() is \e not
                  related.
       */
      uint32_t base_resolution (void) const;

      //!  Reports the device's minimum resolution.
      /*!  It is not known whether this value depends on the currently
           active option.
       */
      uint32_t min_resolution (void) const;

      //!  Reports the device's maximum resolution.
      /*!  It is not known whether this value depends on the currently
           active option.
       */
      uint32_t max_resolution (void) const;

      //!  Reports the maximum scan width.
      /*!  The value is in pixels.
       */
      uint32_t max_scan_width (void) const;

      //!  \copybrief get_extended_status::scan_area
      /*!  Use the base_resolution() to convert to an area in inches.

           \note  ::TPU1 applies to infra-red scans as well.
       */
      bounding_box<uint32_t>
      scan_area (const source_value& source = MAIN) const;

      //!  \copybrief get_extended_status::is_flatbed_type
      /*!  \copydetails get_extended_status::is_flatbed_type
       */
      bool is_flatbed_type (void) const;

      //!  \copybrief get_extended_status::has_lid_option
      /*!  \copydetails get_extended_status::has_lid_option
       */
      bool has_lid_option (void) const;

      //!  \copybrief get_extended_status::has_push_button
      /*!  \copydetails get_extended_status::has_push_button
       */
      bool has_push_button (void) const;

      //!  \copybrief get_extended_status::adf_is_page_type
      /*!  \copydetails get_extended_status::adf_is_page_type
       */
      bool adf_is_page_type (void) const;

      //!  \copybrief get_extended_status::adf_is_duplex_type
      /*!  \copydetails get_extended_status::adf_is_duplex_type
       */
      bool adf_is_duplex_type (void) const;

      //!  \copybrief get_extended_status::adf_is_first_sheet_loader
      /*!  \copydetails get_extended_status::adf_is_first_sheet_loader
       */
      bool adf_is_first_sheet_loader (void) const;

      //!  Tells whether the TPU supports IR scanning.
      /*!  \sa set_scan_parameters::option_unit()

           \todo rename to tpu_has_IR_mode or tpu_has_IR_support
       */
      bool tpu_is_IR_type (void) const;

      //!  Tells whether the lamp in the main body can be changed.
      /*!  It is unclear what this means.
       */
      bool supports_lamp_change (void) const;

      //!  Tells whether the ADF detects the end of a page
      /*!  \sa end_of_transmission
       */
      bool detects_page_end (void) const;

      //!  Tells whether the energy savings time can be changed.
      /*!  \sa set_energy_saving_time
       */
      bool has_energy_savings_setter (void) const;

      bool adf_is_auto_form_feeder (void) const;

      //! \copybrief get_extended_status::adf_detects_double_feed
      /*! \copydetails get_extended_status::adf_detects_double_feed
       */
      bool adf_detects_double_feed (void) const;

      bool supports_auto_power_off (void) const;

      bool supports_quiet_mode (void) const;

      bool supports_authentication (void) const;

      bool supports_compound_commands (void) const;

      //!  Yields the bit depth for an \a io direction.
      /*!  In the ::OUTPUT direction this is the maximum bit depth
           that can be requested by the driver.

           \sa set_scan_parameters::bit_depth()
       */
      byte bit_depth (const io_direction& io) const;

      //!  Tells how documents are aligned on the ADF
      /*!  Scan area computations should take this information into
           account when available.

           \sa alignment_value
       */
      byte document_alignment (void) const;

    protected:
      void check_blk_reply (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_extended_identity_hpp_ */
