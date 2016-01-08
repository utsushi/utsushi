//  action.hpp -- template and derived ESC/I protocol commands
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

#ifndef drivers_esci_action_hpp_
#define drivers_esci_action_hpp_

#include <boost/throw_exception.hpp>

#include "command.hpp"
#include "exception.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Device movers and shakers ;-)
    /*!  A selected few commands are available to directly control
         hardware processes of the device (on the other side of a
         connexion).  This template captures the implementation of
         these commands.
     */
    template <byte b1, byte b2 = 0x00, streamsize size = 1>
    class action : public command
    {
    public:
      action (void)
        : rep_(0)
      {}

      void
      operator>> (connexion& cnx)
      {
        cnx.send (cmd_, size);
        cnx.recv (&rep_, 1);

        this->validate_reply ();
      }

    protected:
      static const byte cmd_[2]; //!<  command byte(s)
      byte rep_;                 //!<  reply byte

      //!  Makes sure the reply is as expected.
      /*!  Most action commands return an ACK if everything is in
           order.  In case the command should not have been sent a
           NAK is returned.

           \throw  invalid_command when a NAK is received
           \throw  unknown_reply when receiving an out of the blue
                   value

           \todo  Subclass abort_scan and end_of_transmission.  These
                  do \e not return a NAK.
       */
      virtual void
      validate_reply (void) const
      {
        if (ACK == rep_)
          return;

        if (NAK == rep_)
          BOOST_THROW_EXCEPTION (invalid_command ());

        BOOST_THROW_EXCEPTION (unknown_reply ());
      }
    };

    template <byte b1, byte b2, streamsize size>
    const byte action<b1,b2,size>::cmd_[2] = { b1, b2 };

    //!  Stop scanning as soon as possible.
    /*!  This command instructs the device to stop sending image data
         and discard whatever data has been buffered.

         \note  This command is reserved for use by start_scan command
                implementations.
         \note  When sent while the device is awaiting commands, this
                command may be ignored and \e not generate a reply.
     */
    typedef action<CAN> abort_scan;

    //!  Stop scanning at end of medium.
    /*!  This command is used to instruct the device to stop sending
         image data when it detects an end of medium condition.  Any
         internally buffered data will be discarded by the device.

         \note  This command is to be used by the start_extended_scan
                command implementation and should only be sent when
                start_extended_scan::is_at_page_end() returns \c true.
         \note  When sent while the device is awaiting commands, this
                command is ignored and does \e not generate a reply.

         \todo  Figure out how this is different from an abort_scan.
                What use case does this serve?
     */
    typedef action<EOT> end_of_transmission;

    //!  Remove media from an automatic document feeder.
    /*!  This command is only effective when the document feeder has
         been activated.  The device replies with an ACK in case the
         command was effective, a NAK otherwise.  The command ejects
         the media that is inside the ADF unit.  This may refer to a
         single sheet of media that was being scanned as well as the
         whole stack of sheets that the user put in the feeder.  The
         command may defer its reply until the last sheet has been
         ejected.

         Depending on the model, when no media is present, media is
         loaded first, then ejected.

         The command should be sent after an ADF type scan has been
         cancelled.

         \note  Use the \ref load_media command to obtain the next
                media sheet on page type ADF units.
         \note  When doing a duplex scan using a sheet-through type
                ADF unit, this command should only be used to eject
                media after cancellation.

         \todo  Document auto-form-feed behaviour.

         \sa set_option_unit, set_scan_parameters::option_unit()
         \sa get_extended_identity::adf_is_page_type(),
             get_extended_status::adf_is_page_type()
         \sa abort_scan, end_of_transmission
     */
    typedef action<FF>  eject_media;

    //!  Fetch media for the next scan.
    /*!  This command is only effective with activated page type ADF
         units.  The device replies with an ACK in case the command
         was effective, a NAK otherwise.  The command prepares the
         ADF unit for the next scan.  It loads media from the tray if
         none is present and when doing a simplex scan.  In case of a
         duplex scan, it turns over the media so the flip side can be
         scanned.  Only after the flip side has been scanned will the
         command load media from the tray.

         This command should only be used with page type ADF units.

         \note  Use the \ref eject_media command to remove media from
                the ADF unit.

         \sa set_option_unit, set_scan_parameters::option_unit()
         \sa get_extended_identity::adf_is_page_type(),
             get_extended_status::adf_is_page_type()
     */
    typedef action<PF>  load_media;

    //!  Interrupt the lamp's warming up process.
    /*!  Sending this command when the device is not actually warming
         up has no effect.

         \note  This command should only be used when the device has
                support for it.

         \sa get_scanner_status::can_cancel_warming_up(),
             get_scanner_status::is_warming_up(),
             get_extended_status::is_warming_up()
     */
    typedef action<ESC,LOWER_W,2>  cancel_warming_up;

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_action_hpp_ */
