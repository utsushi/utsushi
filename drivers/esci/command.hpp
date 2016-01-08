//  command.hpp -- ESC/I protocol commands
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

#ifndef drivers_esci_command_hpp_
#define drivers_esci_command_hpp_

#include <string>

#include <utsushi/connexion.hpp>
#include <utsushi/log.hpp>

#include "code-point.hpp"

namespace utsushi {
namespace _drv_ {
  //! ESC/I protocol commands and constants.
  namespace esci
  {
    //!  ESC/I protocol command base class.
    /*!  The ESC/I protocols combine a largish collection of commands
         with a number of rules.  The collection of commands has been
         modelled and captured in the command class hierarchy.  Large
         parts of the command collection follow a similar pattern and
         have been implemented via template subclasses.  A number of
         the commands specialise or derive from these templates.

         A notable exception to these are the image data acquisition
         commands.  These derive directly from this class.  Finally,
         a few commands do not quite fit the template subclass mould
         and also derive directly from this class.

         The following commands are implemented:

         \li \c CAN    \ref abort_scan
         \li \c EOT    \ref end_of_transmission
         \li \c ESC_!  \ref get_push_button_status
         \li \c ESC_(  \ref capture_scanner
         \li \c ESC_)  \ref release_scanner
         \li \c ESC_@  \ref initialize
         \li \c ESC_A  \ref set_scan_area
         \li \c ESC_B  \ref set_halftone_processing
         \li \c ESC_C  \ref set_color_mode
         \li \c ESC_D  \ref set_bit_depth
         \li \c ESC_F  \ref get_status
         \li \c ESC_G  \ref start_standard_scan
         \li \c ESC_H  \ref set_zoom
         \li \c ESC_I  \ref get_identity
         \li \c ESC_K  \ref set_mirroring
         \li \c ESC_L  \ref set_brightness
         \li \c ESC_M  \ref set_color_correction
         \li \c ESC_N  \ref set_film_type
         \li \c ESC_P  \ref set_energy_saving_time
         \li \c ESC_Q  \ref set_sharpness
         \li \c ESC_R  \ref set_resolution
         \li \c ESC_S  \ref get_command_parameters
         \li \c ESC_Z  \ref set_gamma_correction
         \li \c ESC_b  \ref set_dither_pattern
         \li \c ESC_d  \ref set_line_count
         \li \c ESC_e  \ref set_option_unit
         \li \c ESC_f  \ref get_extended_status
         \li \c ESC_g  \ref set_scan_mode
         \li \c ESC_i  \ref get_hardware_property
         \li \c ESC_m  \ref set_color_matrix
         \li \c ESC_p  \ref set_focus_position
         \li \c ESC_q  \ref get_focus_position
         \li \c ESC_s  \ref set_auto_area_segmentation
         \li \c ESC_t  \ref set_threshold
         \li \c ESC_w  \ref cancel_warming_up
         \li \c ESC_z  \ref set_gamma_table
         \li \c FF     \ref eject_media
         \li \c FS_F   \ref get_scanner_status
         \li \c FS_G   \ref start_extended_scan
         \li \c FS_I   \ref get_extended_identity
         \li \c FS_S   \ref get_scan_parameters
         \li \c FS_W   \ref set_scan_parameters
         \li \c FS_X   \ref scanner_control
         \li \c FS_Y   \ref scanner_inquiry
         \li \c PF     \ref load_media

         \todo  Implement ESC_[ and ESC_].  The latter is identical to
                ESC_) in terms of logic.  The former needs to send an
                additional 40 byte data block with credentials (a 20
                byte username and a 20 byte SHA-1 hash of a string
                that concatenates the username and password).  A 0xc0
                return value indicates permission denied, the other
                values are the same as for ESC_(.

         These commands can usefully be divided into four groups and
         this is reflected in their names:

         \li \c get_*      for \ref getter type commands
         \li \c set_*      for \ref setter type commands
         \li \c scanner_*  for \ref compound type commands
         \li all other commands are \ref action type commands

         Note that the command naming is a better indicator for the
         command's group than its location in the inheritance tree.
         The inheritance tree is more a result of the implementation
         than a logical grouping of the commands.

         \todo  Document "handshake" concept (with figures)?  Here or
                in the respective base classes?

         In general, command implementations validate the protocol
         "handshakes".  That is, they check single byte replies and
         the static content of information blocks.  Undocumented
         values normally trigger an exception.  Data blocks and
         non-static content of information blocks may optionally be
         validated on a per command basis.
     */
    class command
    {
    public:
      virtual ~command (void) {}

      //!  Runs a command on the other end of a connexion.
      /*!  This member function implements the I/O dynamics associated
           with the "execution" of a command.  Typical implementations
           send a few bytes down the connexion and read back a reply.
           The more complicated commands may send and/or receive more
           bytes based on the content of the reply.

           In a sense, you can think of this operator as performing a
           remote procedure call.
       */
      virtual void operator>> (connexion& cnx) = 0;

    protected:
      command (void) {}

      std::string
      name (void) const
      {
        return typeid (*this).name ();
      }

      //!  Checks content of reserved bits in a byte.
      /*!  A helper for pedantic protocol reply checking, this member
           function logs a brief message when one or more bits from a
           \a mask are set in the byte at an \a offset from \a p.
           The message includes the \a type of the reply, so info and
           data blocks can be told apart.
       */
      void
      check_reserved_bits (const byte *p, streamsize offset, byte mask,
                           const std::string& type = "data") const
      {
        if (mask & p[offset])
          {
            log::brief ("%1$s: %2$s[%3$2u] = %4$02x")
              % name ()
              % type
              % offset
              % (0xff & (mask & p[offset]));
          }
      }

    private:
      //  Leave these unimplemented until the class obtains member
      //  variables.  This prevents the compiler from synthesizing
      //  _public_ copy constructors and assignment operators.
      command (const command&);
      command& operator= (const command&);
    };

    //!  Converts a 16-bit unsigned integer into a 2-byte parameter.
    /*!  The first variants of the ESC/I protocol used only 1-byte
         and 2-byte values.  This helper function encodes the value
         \a v into the two byte sequence expected by the protocol.
         The bytes are stored starting at \a p, ordered from least
         significant to most significant.

         \sa from_uint32_t(), to_uint16_t()
     */
    inline void
    from_uint16_t (byte *p, uint16_t v)
    {
      p[0] = 0xff &  v;
      p[1] = 0xff & (v >> 8);
    }

    //!  Converts a 32-bit unsigned integer into a 4-byte parameter.
    /*!  The later variants of the ESC/I protocol added a number of
         extended commands that use 4-byte values.  These bytes are,
         again, ordered from least to most significant.  This helper
         function encodes the value \a v to the four byte sequence
         expected by the protocol.  The bytes are stored starting at
         \a p.

         \sa from_uint16_t(), to_uint32_t()
     */
    inline void
    from_uint32_t (byte *p, uint32_t v)
    {
      from_uint16_t (p + 0, 0xffff &  v);
      from_uint16_t (p + 2, 0xffff & (v >> 16));
    }

    //!  Converts a 2-byte sequence into a 16-bit unsigned integer.
    /*!  The first variants of the ESC/I protocol used only 1-byte
         and 2-byte values.  This helper function converts the two
         byte protocol values into a proper unsigned integer.  The
         bytes are ordered from least to most significant.

         \sa to_uint32_t(), from_uint16_t()
     */
    inline uint16_t
    to_uint16_t (const byte *p)
    {
      return (traits::to_int_type (p[0])
              | traits::to_int_type (p[1]) << 8);
    }

    //!  Converts a 4-byte sequence into a 32-bit unsigned integer.
    /*!  The later variants of the ESC/I protocol added a number of
         extended commands that use 4-byte values.  These bytes are,
         again, ordered from least to most significant.  This helper
         function converts a four byte protocol value into a proper
         unsigned integer.

         \sa to_uint16_t(), from_uint32_t()
     */
    inline uint32_t
    to_uint32_t (const byte *p)
    {
      return (to_uint16_t (p) | to_uint16_t (p+2) << 16);
    }

    //!  Runs a command on the other end of a connexion.
    /*!  This binary operator provides the syntactic sugar that allows
         one to write highly succinct code that runs multiple commands
         in a sequence.  Because a reference to the in-going connexion
         is returned, you can do things like
         \code
         cnx << cmd1 << cmd2 << cmd3;
         \endcode
     */
    inline
    connexion&
    operator<< (connexion& cnx, command& cmd)
    {
      cmd >> cnx;
      return cnx;
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_command_hpp_ */
