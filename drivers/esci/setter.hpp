//  setter.hpp -- template and derived ESC/I protocol commands
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

#ifndef drivers_esci_setter_hpp_
#define drivers_esci_setter_hpp_

#include <boost/throw_exception.hpp>

#include "command.hpp"
#include "exception.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Scan parameter modifiers.
    /*!  A number of commands are available that allow one to tell the
         device (on the other end of the connexion) precisely what and
         how to acquire image data.  This covers such basic things as
         the resolution and scan area as well as more advanced things
         like gamma correction and automatic area segmentation.

         Note, however, that not all protocol variants support all of
         the setter commands.

         Setter subclasses typically implement \c operator() to set the
         value that will be used when running the command and return
         a reference to the object.  Depending on a command's natural
         parameter type, different overloads will be implemented.

         The parameter values to be sent to the device are normally \e
         not checked for correctness.  This is by design.  In most, if
         not all, situations the implementation lacks the context that
         is necessary to do a meaningful check.  In addition, future
         versions of the protocol may extend the range of a command's
         allowed parameters.  Not adding checking therefore makes the
         implementation more robust.  Finally, a device ought to know
         best anyway.  Devices already check the parameters they get,
         and return a NAK if something is amiss.

         \sa getter
     */
    template <byte b1, byte b2, streamsize size>
    class setter : public command
    {
    public:
      setter (void)
        : rep_(0)
      {
        traits::assign (dat_, sizeof (dat_) / sizeof (*dat_), 0);
      }

      setter (const setter& s)
        : rep_(s.rep_)
      {
        traits::copy (dat_, s.dat_, sizeof (dat_) / sizeof (*dat_));
      }

      setter&
      operator= (const setter& s)
      {
        if (this != &s)
          {
            rep_ = s.rep_;
            traits::copy (dat_, s.dat_, sizeof (dat_) / sizeof (*dat_));
          }
        return *this;
      }

      //!  Runs a command on the other end of a connexion.
      /*!  This override implements the I/O dynamics for all regular
           setter commands.  The command's "handshake" consists of a
           send command/receive acknowledgement pair, followed by a
           send parameter buffer/receive acknowledgement pair.

           \sa set_dither_pattern
       */
      virtual void
      operator>> (connexion& cnx)
      {
        cnx.send (cmd_, sizeof (cmd_) / sizeof (*cmd_));
        cnx.recv (&rep_, 1);

        this->validate_cmd_reply ();

        cnx.send (dat_, sizeof (dat_) / sizeof (*dat_));
        cnx.recv (&rep_, 1);

        this->validate_dat_reply ();
      }

    protected:
      static const byte cmd_[2]; //!<  command bytes
      byte dat_[size];           //!<  command parameters
      byte rep_;                 //!<  reply byte

      //!  Make sure the reply to a command is as expected.
      /*!  Setter commands return an ACK if the command has been
           accepted on the device side, a NAK otherwise.

           \throw  invalid_command when a NAK is received
           \throw  unknown_reply when receiving an out of the blue
                   value
       */
      virtual void
      validate_cmd_reply (void) const
      {
        if (ACK == rep_)
          return;

        if (NAK == rep_)
          BOOST_THROW_EXCEPTION (invalid_command ());

        BOOST_THROW_EXCEPTION (unknown_reply ());
      }

      //!  Make sure the parameters were accepted.
      /*!  Setter commands return an ACK when the parameters were
           platable to the device, a NAK otherwise.

           \throw  invalid_parameter when a NAK is received
           \throw  unknown_reply when receiving an out of the blue
                   value
       */
      virtual void
      validate_dat_reply (void) const
      {
        if (ACK == rep_)
          return;

        if (NAK == rep_)
          BOOST_THROW_EXCEPTION (invalid_parameter ());

        BOOST_THROW_EXCEPTION (unknown_reply ());
      }
    };

    template <byte b1, byte b2, streamsize size>
    const byte setter<b1,b2,size>::cmd_[2] = { b1, b2 };

    //!  Single byte encoded scan parameter modifiers.
    /*!  A lot of the scan parameter setters simply set a byte encoded
         parameter.  This "simple" setter template derivative captures
         the code pattern needed for such commands.
     */
    template <byte b1, byte b2>
    class sim_setter : public setter<b1,b2,1>
    {
    public:
      //!  Sets the \a parameter to use when running the command.
      /*!  The implementation returns a reference to the object it was
           invoked on.  This way you can concisely set a parameter and
           run the command on the other end of a connexion, like so
           \code
           cnx << sim_setter1 (16);
           \endcode
       */
      sim_setter&
      operator() (const byte parameter)
      {
        this->rep_ = 0;
        this->dat_[0] = parameter;
        return *this;
      }
    };

    //!  Change the active option unit and its mode.
    /*!  This setting is used to select the scan source and the mode
         in which to use the source.  Known source/mode combinations
         have been defined as \ref option_value's.

         The default value is ::MAIN_BODY.  This selects the device's
         \e main scan source.  For most devices this will be the
         flatbed but other main scan sources may exist (most obviously
         for those devices that do not have a flatbed).

         \note  After this command has been processed, the scan area
                is reset to the maximum available with the current
                resolution and zoom settings.  It is best sent before
                set_resolution, set_zoom and set_scan_area.

         \sa option_value
     */
    typedef sim_setter<ESC,LOWER_E> set_option_unit;

    //!  Set scan color and sequence modes.
    /*!  The settings controls the color components and their ordering
         in the image data.

         The default color mode is ::MONOCHROME.

         \sa color_mode_value
     */
    typedef sim_setter<ESC,UPPER_C> set_color_mode;

    //!  Control the number of shades of the color components.
    /*!  This setting impacts not only the number of shades for each
         color component, it also affects the number of bytes that
         have to be transferred from the device to the application.

         \todo  Document pixel packing

         Values of 1 and 8 are normally supported and 16 can often be
         used as well.  Some devices support all values in the [1,16]
         range.

         \note  The maximum supported bit depth value is available via
                get_extended_identity::bit_depth()
     */
    typedef sim_setter<ESC,UPPER_D> set_bit_depth;

    //!  Flip the horizontal orientation of the pixels.
    /*!  Use this to get an image's rightmost pixels at the start of
         the image data.

         Mirroring is activated by setting a \c true value.

         \note  This setting has no effect on the scan area.
     */
    typedef sim_setter<ESC,UPPER_K> set_mirroring;

    //!  Control sharpness of edges in an image.
    /*!  An image can be made to look sharper by emphasizing the edges
         in the image.  Conversely, de-emphasizing the edges makes for
         a smoother image.

         Results depend on input image and output device.

         \sa sharpness_value
     */
    typedef sim_setter<ESC,UPPER_Q> set_sharpness;

    //!  Adjust the brightness.
    /*!  This setting controls the interpretation of the predefined
         gamma tables.  It makes images look lighter/darker depending
         on the value that has been set.

         \note  This setting has no effect when using a custom gamma
                table.

         \sa brightness_value, set_gamma_correction,
             set_scan_parameters::gamma_correction()
     */
    typedef sim_setter<ESC,UPPER_L> set_brightness;

    //!  Set a gamma table.
    /*!  This setting controls which gamma table is in effect.  Use
         set_gamma_table to set or change a \c CUSTOM_GAMMA_* table.

         \sa gamma_table_value
     */
    typedef sim_setter<ESC,UPPER_Z> set_gamma_correction;

    //!  Set a color matrix.
    /*!  This setting selects the color matrix to be used.  Use the
         set_color_matrix command to set or change a ::USER_DEFINED
         matrix.

         \note  The color matrix is not applied to monochrome scans.

         \sa color_matrix_value
     */
    typedef sim_setter<ESC,UPPER_M> set_color_correction;

    //!  Set a halftone mode or dither pattern.
    /*!  This setting is used to select a halftone mode or dither
         pattern.  It only has an effect for bi-level and quad-level
         scans, that is, at bit depths of 1 and 2.

         Use set_dither_pattern to set or change a \c CUSTOM_DITHER_*
         pattern.

         \todo  Document the dependencies between the cross-referenced
                settings?  There are about seven settings that affect
                the interpretation/activity of the value set.

         \sa halftone_dither_value, set_threshold,
             set_auto_area_segmentation
     */
    typedef sim_setter<ESC,UPPER_B> set_halftone_processing;

    //!  Toggle auto area segmentation activation
    /*!  This setting can be used to activate splitting the image in
         text and photo regions.  These regions will then be subject
         to different halftoning/dithering strategies, each tweaked
         to the characteristics of the image type.  The text will be
         subject to thresholding and the image to halftoning.

         Auto area segmentation is activated by setting a \c true
         value.

         \note  This setting is ignored when using a bit depth larger
                than 1, using ::BI_LEVEL \ref halftone_dither_value,
                or doing a scan for negative film.

         \sa set_bit_depth, set_threshold, set_film_type
     */
    typedef sim_setter<ESC,LOWER_S> set_auto_area_segmentation;

    //!  Decide the border between black and white.
    /*!  Pixel values less than the configured threshold will be set
         to zero, other values to one.  The value is used when doing
         a ::BI_LEVEL scan and when auto area segmentation is active.

         \note  This setting only takes effect when scanning at a bit
                depth of one.

         \sa set_halftone_processing, set_auto_area_segmentation
     */
    typedef sim_setter<ESC,LOWER_T> set_threshold;

    //!  Trade quality for speed and vice versa.
    /*!  Use this to get a lesser quality image at higher speed.

         The default value is ::NORMAL_SPEED.

         \sa scan_mode_value
     */
    typedef sim_setter<ESC,LOWER_G> set_scan_mode;

    //!  Sets the number of scan lines per block.
    /*!  This setting can be used to control the size of the image
         data chunks returned by start_standard_scan::operator++().

         The default value is \c 0x00 which selects line mode.  Other
         values select block mode with the number of lines equal to
         the parameter's numeric value.

         \note  The last block may consist of less lines than set.
         \note  When scanning in line sequence mode (see \c LINE_*
                \ref color_mode_value), the line count value should
                be a multiple of three.
         \note  Sending a start_standard_scan command to the device
                resets this setting to its default value of \c 0x00
                for the \e next start_standard_scan command to use.
       */
    typedef sim_setter<ESC,LOWER_D> set_line_count;

    //!  Set the film type about to be scanned.
    /*!  The default value is ::POSITIVE_FILM.

         The default value is always accepted but a negative film
         setting will only be accepted if the TPU option unit has
         been selected for scanning in a mode different from the
         ::TPU_IR_1 mode.  This seems to imply that ::TPU_IR_2 is
         meant for negative film infra-red scans.

         \sa film_type_value
     */
    typedef sim_setter<ESC,UPPER_N> set_film_type;

    //!  Say where the focus should be.
    /*!  This command can always be used (with \c B# level scanners),
         irrespective of focussing support.

         If focussing is not supported, focus remains at the default
         of ::FOCUS_GLASS.  Certain devices only support one of two
         focus positions.  There is \e no guarantee that a position
         requested is actually set.  The get_focus_position command
         should be used to check what setting is in use.

         \note  The set_scan_area command should be run \e before one
                requests auto-focus.  This implies that set_option,
                set_resolution and set_zoom should be too.

         \note  There is no documented way to determine what kind of
                focussing support a device has.

         \todo  Find out what is up with the scan area setting when
                requesting auto-focus (and this is supported).  It
                seems that the value of the height of the scan area
                becomes 1 and cannot be relied upon.
     */
    typedef sim_setter<ESC,LOWER_P> set_focus_position;

    //!  Set the timeout until switching to energy savings mode.
    /*!  The default timeout value is 15 minutes, ::TIMEOUT_015.

         \note  The device may switch to energy savings mode as much as
                one minute \e before the value set with this command.

         \note  When get_extended_identity::has_energy_savings_setter()
                returns \c false, this command has no effect.

         \sa timeout_value
     */
    typedef sim_setter<ESC,UPPER_P> set_energy_saving_time;

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_setter_hpp_ */
