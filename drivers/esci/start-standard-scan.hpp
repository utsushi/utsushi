//  start-standard-scan.hpp -- to acquire image data
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

#ifndef drivers_esci_start_standard_scan_hpp_
#define drivers_esci_start_standard_scan_hpp_

#include "constant.hpp"
#include "start-scan.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Acquiring image data per line or in blocks.
    /*!  The standard start scan command's "handshake" is implemented
         by this class.  Unlike the bulk of implemented commands, the
         handshake is split over two member functions.  This allows
         for repeatedly fetching of image data chunks.

         The implementation ensures that replies upon receipt of image
         data are sent when necessary.  It also deals with the timing
         issues of scan cancellation.

         \note  Access to the 0x12 status bits, indicating presence of
                an option unit and support for the extended commands,
                is not provided on purpose.  They are not relevant to
                this command.
     */
    class start_standard_scan : public start_scan
    {
    public:
      //!  Creates a command to start a scan in standard mode.
      /*!  The default is to scan in line mode, which acquires a
           single scan line of image data per operator++() call.
           Explicitly passing a non-zero \a line_count will trigger
           block mode, even if \a line_count equals one.

           \copydetails getter::getter()

           \note  Pedantic checking of the data block has not been
                  provided because it only contains image data, none
                  of which can be meaningfully validated at the
                  command level.

           \sa set_line_count
       */
       start_standard_scan (uint8_t line_count = 0, bool pedantic = false);
      ~start_standard_scan (void);

      //!  \copybrief start_scan::operator>>()
      /*!  \copydetails start_scan::operator>>()

           \internal
           The device only accepts ::ACK and ::CAN after operator>>
           and will return ::NAK if it receives something else.  It
           continues to send image data in the latter case.
       */
      void operator>> (connexion& cnx);

      //!  \copybrief start_scan::operator++()
      /*!  \copydetails start_scan::operator++()
       */
      chunk operator++ (void);

      //!  Tells whether the device detected a fatal error.
      /*!  When this function returns \c true something has gone very
           wrong.  The get_extended_status API may be useful in trying
           to find out more precisely what went wrong.

           \sa buf_getter::detected_fatal_error()
       */
      bool detected_fatal_error (void) const;

      //!  \copybrief start_scan::is_ready()
      /*!  \copydetails start_scan::is_ready()
       */
      bool is_ready (void) const;

      //!  Tells whether the scan area has been processed completely.
      /*!  When this function returns \c true after operator++() has
           returned an empty chunk, all image data has been acquired
           for the current page and passed on to the caller.

           When scanning in color page sequence mode, ::PAGE_GRB or
           ::PAGE_RGB, three pages make up a single image.  In that
           case this function should return \c true three times for
           a complete and valid image.

           \note  Although seemingly similar, this is \e not the same
                  as start_extended_scan::is_at_page_end().
       */
      bool is_at_area_end (void) const;

      //!  Indicates how to interpret the image data.
      /*!  The color attribute information is encoded in the 0x0c bits
           of the status byte.  This member function decodes this info
           into a \ref color_value based on the current color \a mode.

           \sa set_color_mode, buf_getter::color_attributes()
       */
      color_value color_attributes (const color_mode_value& mode) const;

      //!  \copybrief start_scan::cancel()
      /*!  \copydetails start_scan::cancel()

           This command does not support end-of-medium detection.

           \sa abort_scan
       */
      void cancel (bool at_area_end = false);

    protected:
      static const byte cmd_[2]; //!<  command bytes
      byte blk_[6];              //!<  information block

      uint8_t line_count_;      //!<  number of scan lines to acquire

      //!  Computes the number of bytes in the next chunk.
      streamsize size_(void) const;
      //!  Says whether there are chunks left for acquisition.
      bool more_chunks_(void) const;

      //!  \copybrief buf_getter::validate_info_block
      /*!  \copydetails buf_getter::validate_info_block
       */
      virtual void validate_info_block (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_start_standard_scan_hpp_ */
