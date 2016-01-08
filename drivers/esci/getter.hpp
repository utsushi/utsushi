//  getter.hpp -- command template for ESC/I protocol commands
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

#ifndef drivers_esci_getter_hpp_
#define drivers_esci_getter_hpp_

#include <cstring>

#include <locale>
#include <stdexcept>

#include <boost/throw_exception.hpp>

#include "command.hpp"
#include "constant.hpp"
#include "exception.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Fixed size reply capability and state queries.
    /*!  Several commands allow one to find out about the state and/or
         capabilities of the device (on the other end of a connexion).
         This template caters to the cases where the reply to a query
         has a (compile-time) fixed size.

         The getter subclasses do not normally validate the data they
         receive from the device.  This is by design.  The information
         contained in the data is not only device dependent, it also
         depends on the device's current state (in the general case).
         Furthermore, updated protocol versions may "unreserve" some
         of the currently reserved bits and bytes without affecting a
         driver's functionality to a point where it becomes useless.
         Passing over any such changes, then, makes for a more robust
         driver.

         \sa setter
     */
    template <byte b1, byte b2, streamsize size>
    class getter : public command
    {
    public:
      //!  Creates an optionally pedantic instance.
      /*!  The optional \a pedantic argument can be used to control
           checking for "unreserved" bits and bytes as well as any
           deviation from the protocol specification.  When checking,
           the command implementation may log information about bits
           and bytes that are as of yet reserved but contain values
           that do not correspond to that designation.  It may also
           flag parts in a reply that do not conform to the protocol
           specification.

           This may be used to quickly check a new device's protocol
           conformance against this implementation.
       */
      getter (bool pedantic = false)
        : pedantic_(pedantic)
      {
        traits::assign (blk_, sizeof (blk_) / sizeof (*blk_), 0);
      }

      void
      operator>> (connexion& cnx)
      {
        cnx.send (cmd_, sizeof (cmd_) / sizeof (*cmd_));
        cnx.recv (blk_, sizeof (blk_) / sizeof (*blk_));

        if (this->pedantic_)
          this->check_blk_reply ();
      }

    protected:
      bool pedantic_;            //!<  checking of replies or not
      static const byte cmd_[2]; //!<  command bytes
      byte blk_[size];           //!<  information or data block

      //!  Pedantically checks an information or data block.
      /*!  The implementation may log information about bits and bytes
           that are as of yet reserved but contain values that do not
           correspond to that designation or flag parts deviating from
           the protocol specification.

           The default implementation does nothing.
       */
      virtual void
      check_blk_reply (void) const
      {}

      //!  Converts a sequence of \a sz protocol bytes into a string.
      /*!  Parts of the reply received by a query may consists of what
           is really a string literal.  This helper function converts
           the \a sz byte part starting at \a p into a C++ string.

           Note that trailing whitespace (as per "C" locale) is deemed
           irrelevant and will be removed.
       */
      static std::string
      to_string (const byte *p, streamsize sz)
      {
        const std::string::value_type *s
          = static_cast<const std::string::value_type *> (p);

        std::string::value_type str[sz+1];
        std::string::traits_type::copy (str, s, sz);

        do                 // terminate and remove trailing whitespace
          {
            str[sz] = '\0';
            --sz;
          }
        while (0 < sz && isspace (str[sz], std::locale::classic ()));

        return str;
      }
    };

    template <byte b1, byte b2, streamsize size>
    const byte getter<b1,b2,size>::cmd_[2] = { b1, b2 };

    //!  Variable size reply capability and state queries.
    /*!  This template derivative caters to those commands that do not
         have an a priori known reply size.  The reply size is relayed
         by the device (at the other end of an connexion) in a primary
         4-byte sized reply, the information block.

         The class also includes API to access information contained
         in a status byte in the information block.

         \note  Access to the 0x20 bit, indicating end of area, is not
                provided on purpose.  The only time this information
                makes sense is as part of a start_standard_scan, see
                which.
     */
    template <byte b1, byte b2>
    class buf_getter : public getter<b1,b2,4>
    {
    public:
      //!  \copybrief getter::getter
      /*!  \copydetails getter::getter
       */
      buf_getter (bool pedantic = false)
        : getter<b1,b2,4> (pedantic)
        , dat_(NULL)
        , dat_size_(0)
      {}

      virtual
      ~buf_getter (void)
      {
        delete [] dat_;
      }

      //!  Runs a command on the other end of a connexion.
      /*!  This override extends the parent class' implementation and
           requests an additional reply.  The reply size is computed
           from the primary reply and a suitably sized buffer will be
           (re)allocated before the additional reply is requested.
       */
      void
      operator>> (connexion& cnx)
      {
        cnx.send (this->cmd_, sizeof (this->cmd_) / sizeof (*this->cmd_));
        cnx.recv (this->blk_, sizeof (this->blk_) / sizeof (*this->blk_));

        this->validate_info_block ();

        if (0 < size ())
          {
            if (dat_size_ < size ())
              {
                delete [] dat_;
                dat_ = new byte[size ()];
                dat_size_ = size ();
              }
            cnx.recv (dat_, size ());

            if (this->pedantic_)
              this->check_data_block ();
          }
      }

      //!  Computes the size of the data block.
      streamsize
      size (void) const
      {
        return to_uint16_t (this->blk_ + 2);
      }

      //!  Tells whether the device detected a fatal error.
      /*!  When this function returns \c true something has gone very
           wrong.  The get_extended_status API may be useful in trying
           to find out more precisely what went wrong.  For a device
           that supports_extended_commands() get_scanner_status may be
           a better choice though.

           \sa start_scan::detected_fatal_error()
       */
      bool
      detected_fatal_error (void) const
      {
        return 0x80 & this->blk_[1];
      }

      //!  Tells whether the device is ready to start a scan.
      /*!  A device that is not ready to start a scan is normally in
           use by someone else.  For example, somebody may be making
           copies on a multi-function device or a device is accessed
           via its network interface.

           \sa start_scan::is_ready()
       */
      bool
      is_ready (void) const
      {
        return !(0x40 & this->blk_[1]);
      }

      //!  Tells whether an option unit is installed on the device.
      /*!  While the query indicates whether there is an option unit,
           it can not tell which kind.  This information may be had
           via the get_extended_status command and, if supported, the
           get_scanner_status command.

           Known option units include automatic document feeders (ADF,
           both simplex and duplex) and transparency units (TPU).
       */
      bool
      has_option (void) const
      {
        return 0x10 & this->blk_[1];
      }

      //!  Indicates the current color component or color ordering.
      /*!  The color attribute information is encoded in the 0x0c bits
           of the status byte.  This member function decodes this info
           into a color_value based on the current color \a mode.  The
           \a line_mode argument matches that of start_standard_scan
           and is essential to correctly decode any of the \c LINE_*
           color \a mode values.

           \note  It is unclear whether this information is of any use
                  outside of the scope of the start_scan commands.

           \sa set_color_mode, set_scan_parameters::color_mode(),
               start_standard_scan::color_attributes()
       */
      color_value
      color_attributes (const color_mode_value& mode,
                        bool line_mode = false) const
      {
        if ((!line_mode
             && (   LINE_GRB == mode
                 || LINE_RGB == mode))
            || (   PIXEL_GRB == mode
                || PIXEL_RGB == mode))
          {
            if (0x04 == this->blk_[1])
              return GRB;
            if (0x08 == this->blk_[1])
              return RGB;
          }
        else
          {
            if (0x00 == this->blk_[1])
              return MONO;
            if (0x04 == this->blk_[1])
              return GREEN;
            if (0x08 == this->blk_[1])
              return RED;
            if (0x0c == this->blk_[1])
              return BLUE;
          }
        BOOST_THROW_EXCEPTION (range_error ("undocumented color attributes"));
      }

      //!  Tells whether the device supports the extended commands.
      /*!  Extended commands are an addition to the earlier protocol
           versions and aim to reduce I/O between the driver and the
           device.  They provide a cleaner separation between device
           capabilities and state as well.

           All extended commands start with FS rather than with ESC.
       */
      bool
      supports_extended_commands (void) const
      {
        return 0x02 & this->blk_[1];
      }

    protected:
      byte      *dat_;          //!<  data block
      streamsize dat_size_;     //!<  data block byte size

      //!  Validates an information block.
      /*!  Only the so-called header byte of the information block can
           be meaningfully validated.  The block's status byte can only
           be checked for "unreserved" bits.

           \throw  unknown_reply in case of a bad header byte
       */
      virtual void
      validate_info_block (void) const
      {
        if (STX != this->blk_[0])
          BOOST_THROW_EXCEPTION (unknown_reply ());

        if (this->pedantic_)
          {
            this->check_reserved_bits (this->blk_, 1, 0x01, "info");
          }
      }

      //!  Pedantically checks a data block.
      /*!  \copydetails getter::check_blk_reply()
       */
      virtual void
      check_data_block (void) const
      {}
    };

    //!  Basic status query.
    /*!  The most trivial of all the buf_getter commands, this command
         never gets a buffer at all.
     */
    typedef buf_getter<ESC,UPPER_F> get_status;

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_getter_hpp_ */
