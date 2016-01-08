//  start-extended-scan.hpp -- to acquire image data
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008, 2013  Olaf Meeuwissen
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

#ifndef drivers_esci_start_extended_scan_hpp_
#define drivers_esci_start_extended_scan_hpp_

#include "start-scan.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Acquiring image data in larger blocks.
    /*!  The extended start scan command's "handshake" is implemented
         by this class.  Unlike the bulk of implemented commands, the
         handshake is split over two member functions.  This allows
         for repeatedly fetching of image data chunks.

         The implementation ensures that replies upon receipt of image
         data are sent when necessary.  It also deals with the timing
         issues and both types of scan cancellation.

         \note  Access to the 0x12 status bits, indicating presence of
                an option unit and support for the extended commands,
                is not provided on purpose.  They are not relevant to
                this command.  Moreover, extended command support is a
                prerequisite for the use this command.
         \note  Access to the 0x2c status bits is not provided.  These
                bits are always zero when using this command.
     */
    class start_extended_scan : public start_scan
    {
    public:
      //!  Creates a command to start scanning in extended mode.
      /*!  \copydetails getter::getter()

           \note  Pedantic checking of the data block has been limited
                  to the error code.  The remaining bytes are image
                  data which can not be meaningfully checked within
                  the scope of this command.
       */
       start_extended_scan (bool pedantic = false);
       start_extended_scan (byte error_code_mask, bool pedantic = false);
      ~start_extended_scan (void);

      //!  \copybrief start_scan::operator>>()
      /*!  \copydetails start_scan::operator>>()

           \internal
           The device only accepts one of ::ACK, ::EOT and ::CAN after
           operator>> and will return ::NAK if it receives something
           else.  It continues to send image data in the latter case.
           The device will also return a ::NAK if ::EOT is sent at the
           wrong time, i.e. when is_at_page_end() returns \c false.
       */
      void operator>> (connexion& cnx);

      //!  \copybrief start_scan::operator++()
      /*!  \copydetails start_scan::operator++()
       */
      chunk operator++ (void);

      //!  \copybrief start_scan::detected_fatal_error()
      /*!  When this function returns \c true something has gone very
           wrong.  The get_scanner_status API may be useful in finding
           out more precisely what went wrong.

           \sa buf_getter::detected_fatal_error()
       */
      bool detected_fatal_error (void) const;

      //!  \copybrief start_scan::is_ready()
      /*!  \copydetails start_scan::is_ready()
       */
      bool is_ready (void) const;

      //!  Tells whether a page end was detected during a scan.
      /*!  Some devices can detect the end of a page while scanning.
           When they have detected an end of page, this function
           returns \c true.

           \note  Although seemingly similar, this is \e not the same
                  as start_standard_scan::is_at_area_end().

           \sa get_extended_identity::detects_page_end()
       */
      bool is_at_page_end (void) const;

      //!  Tells whether cancellation was requested on the device side.
      /*!  Some devices, typically multi-function devices, support the
           cancellation of scans on the hardware side.  When the scan
           is cancelled on the hardware side, this function returns \c
           true.

           A cancellation request from the hardware side is processed
           as a part of the scan logic and operator++() will send the
           appropriate command at the first opportunity.
       */
      bool is_cancel_requested (void) const;

      //!  \copybrief start_scan::cancel()
      /*!  \copydetails start_scan::cancel()

           This command supports end-of-medium detection.

           \sa abort_scan, end_of_transmission
       */
      void cancel (bool at_area_end = false);

    protected:
      static const byte reserved_error_code_bits_ = 0x0f;
      static const byte cmd_[2]; //!<  command bytes
      byte blk_[14];             //!<  information block

      bool do_at_end_;          //!<  abort at medium end detection
      byte error_code_mask_;    //!<  status flags to be ignored
      byte error_code_;         //!<  collection of status flags

      uint32_t chunk_count_;    //!<  number of chunks still to go
      uint32_t final_bytes_;    //!<  size of the last chunk

      //!  Computes the number of bytes in the next chunk.
      streamsize size_(void) const;
      //!  Says whether there are chunks left for acquisition.
      bool   more_chunks_(void) const;

      virtual void  setup_chunk_(streamsize size, bool with_error_code) = 0;
      virtual chunk fetch_chunk_(streamsize size, bool with_error_code) = 0;

      //!  \copybrief buf_getter::validate_info_block
      /*!  \copydetails buf_getter::validate_info_block
       */
      virtual void validate_info_block (void) const;

      //!  Cleans up the error code.
      /*!  The error code is checked for "unreserved" and unsupported
           bits.  Any such bits found will be logged pedantically and
           summarily cleared.
       */
      virtual void scrub_error_code_(void);
    };

    //! \todo  Choose one of start_ext_scan_alloc and start_ext_scan_reuse

    class start_ext_scan_alloc : public start_extended_scan
    {
    public:
       start_ext_scan_alloc (bool pedantic = false)
         : start_extended_scan (pedantic)
      {}

    protected:
      void  setup_chunk_(streamsize size, bool with_error_code)
      {}

      chunk fetch_chunk_(streamsize size, bool with_error_code)
      {
        return chunk (size, with_error_code);
      }
    };

    class start_ext_scan_reuse : public start_extended_scan
    {
      chunk chunk_;

    public:
       start_ext_scan_reuse (bool pedantic = false)
         : start_extended_scan (pedantic)
      {}

    protected:
      void  setup_chunk_(streamsize size, bool with_error_code)
      {
        if (!chunk_) chunk_ = chunk (size, with_error_code);
      }

      chunk fetch_chunk_(streamsize size, bool with_error_code)
      {
        return chunk_;
      }
  };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_start_extended_scan_hpp_ */
