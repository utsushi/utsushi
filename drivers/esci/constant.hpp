//  constant.hpp -- ESC/I protocol constants
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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

#ifndef drivers_esci_constant_hpp_
#define drivers_esci_constant_hpp_

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Select a color component, color space or color ordering.
    /*!  Convenience type to select which of the line numbers to
         return or gamma table to set.

         \sa get_hardware_property::line_number(), set_gamma_table
     */
    enum color_value {
      NO_COLOR = 0,
      MONO,
      RED,
      GREEN,
      BLUE,
      GRB,
      RGB,
    };

    //!  Select a media source.
    /*!  Convenience type to select the the media source one wants to
         use or retrieve information about.  Unlike an \ref option_value,
         it does not imply any mode of operation.

         The ::TPU1 and ::TPU2 values became necessary when the first
         devices with \e two transparency units hit the markets.
     */
    enum source_value {
      NO_SOURCE = 0,
      MAIN,                     //!<  main body, normally the flatbed
      ADF,                      //!<  automatic document feeder
      TPU,                      //!<  transparency unit
      TPU1 = TPU,               //!<  primary transparency unit
      TPU2,                     //!<  secondary transparency unit
    };

    //!  Documented option settings.
    /*!  While the ::option_value constants appear very similar to the
         \ref source_value ones, there is an important difference.
         They select a mode of operation in addition to the source.

         The device will reply with a NAK in case one tries to select
         unavailable option units or unsupported modes.  Capability
         information is available via the various member functions of
         get_extended_status or get_extended_identity.

         \note  Values larger than 0x02 may not be supported by the
                ::set_option_unit command and only be accessible via
                set_scan_parameters::option_unit().

         \sa set_option_unit, set_scan_parameters::option_unit()
     */
    enum option_value {
      MAIN_BODY   = 0x00,
      ADF_SIMPLEX = 0x01,
      ADF_DUPLEX  = 0x02,
      TPU_AREA_1  = 0x01,
      TPU_AREA_2  = 0x05,
      TPU_IR_1    = 0x03,
      TPU_IR_2    = 0x04,
    };

    //!  Documented color modes.
    /*!  Monochrome scans are selected with one of the \c %MONOCHROME
         or \c DROPOUT_* values.  The dropout values can be used to
         select a color that is to be ignored.  One could, for example
         use %DROPOUT_R to ignore red underlined text in an otherwise
         black and white document.

         The \c PAGE_* values select a page scanning mode.  The whole
         page will be scanned once for each of the color components,
         in the order specified.  To be painfully explicit, PAGE_RGB
         first gives you a whole image of red component values, then
         green component values and finally blue component values.  In
         order to create a usable color image from that, the API user
         has to combine the three pages.

         For \c LINE_* values, each scan line is split in three lines
         of monochromatic component values.  For example, %LINE_GRB
         produces a lines of green, red and blue component values in
         that order.  It is up to the API user to combine these into
         pixels.

         Finally, \c PIXEL_* values put all the color component values
         of a single pixel next to each other, in the order requested.

         Not all devices are expected to support all color modes.

         \note  BGR order is not supported by the standard commands.
         \note  Page mode is not supported by the extended commands.

         \sa set_color_mode, set_scan_parameters::color_mode()
     */
    enum color_mode_value {
      MONOCHROME = 0x00,
      DROPOUT_R  = 0x10,
      DROPOUT_G  = 0x20,
      DROPOUT_B  = 0x30,
      PAGE_GRB   = 0x01,
      PAGE_RGB   = 0x11,
      LINE_GRB   = 0x02,
      LINE_RGB   = 0x12,
      LINE_BGR   = 0x22,
      PIXEL_GRB  = 0x03,
      PIXEL_RGB  = 0x13,
      PIXEL_BGR  = 0x23,
    };

    //!  Symbolic names for the documented sharpness values.
    /*!  \sa set_sharpness, set_scan_parameters::sharpness()
     */
    enum sharpness_value {
      SHARP    = 0x01,
      SHARPER  = 0x02,
      SMOOTH   = 0xff,
      SMOOTHER = 0xfe,
    };

    //!  Symbolic names for the documented brightness values.
    /*!  \sa set_brightness, set_scan_parameters::brightness()
     */
    enum brightness_value {
      LIGHT    = 0x01,
      LIGHTER  = 0x02,
      LIGHTEST = 0x03,
      DARK     = 0xff,
      DARKER   = 0xfe,
      DARKEST  = 0xfd,
    };

    //!  Documented gamma table settings.
    /*!  The various values select canned gamma tables for the intended
         output device.  The two \c CUSTOM_GAMMA_* values select a base
         on top of which the (separately defined) custom gamma table is
         applied.

         \sa set_gamma_table
     */
    enum gamma_table_value {
      BI_LEVEL_CRT      = 0x01,
      MULTI_LEVEL_CRT   = 0x02,
      HI_DENSITY_PRINT  = 0x00,
      LO_DENSITY_PRINT  = 0x10,
      HI_CONTRAST_PRINT = 0x20,
      CUSTOM_GAMMA_A    = 0x03,         //!< for a base gamma value of 1.0
      CUSTOM_GAMMA_B    = 0x04,         //!< for a base gamma value of 1.8
    };

    //!  Documented color matrix settings.
    /*!  The ::UNIT_MATRIX can be used to turn off all color correction
         and a ::USER_DEFINED matrix can be selected to make any kind
         of corrections deemed necessary.  The remaining values select
         canned matrices targetting their corresponding color devices.

         \sa set_color_matrix
     */
    enum color_matrix_value {
      UNIT_MATRIX        = 0x00,
      USER_DEFINED       = 0x01,
      DOT_MATRIX_PRINTER = 0x10,
      THERMAL_PRINTER    = 0x20,
      INKJET_PRINTER     = 0x40,
      CRT_DISPLAY        = 0x80,
    };

    //!  Documented halftone and dither methods.
    /*!  \todo  Extend documentation.

         \sa set_dither_pattern
     */
    enum halftone_dither_value {
      BI_LEVEL        = 0x01,
      TEXT_ENHANCED   = 0x03,
      HARD_TONE       = 0x00,
      SOFT_TONE       = 0x10,
      NET_SCREEN      = 0x20,
      BAYER_4_4       = 0x80,
      SPIRAL_4_4      = 0x90,
      NET_SCREEN_4_4  = 0xa0,
      NET_SCREEN_8_4  = 0xb0,
      CUSTOM_DITHER_A = 0xc0,
      CUSTOM_DITHER_B = 0xd0,
    };

    //!  Symbolic names for the documented scan modes.
    enum scan_mode_value {
      NORMAL_SPEED = 0x00,
      HI_SPEED     = 0x01,
    };

    //!  Symbolic names for the documented quiet scan modes.
    enum quiet_mode_value {
      QUIET_DEFAULT  = 0x00,    //!< use current setting without change
      QUIET_MODE_OFF = 0x01,
      QUIET_MODE_ON  = 0x02,
    };

    //!  Symbolic names for the documented film types.
    enum film_type_value {
      POSITIVE_FILM = 0x00,
      NEGATIVE_FILM = 0x01,     //!< cannot be used with ::TPU_IR_1
    };

    //!  Symbolic names for the documented focus positions.
    /*!  Values less than ::FOCUS_GLASS are below the glass plate, those
         larger are above.  The units associated with a focus position
         value, if any, are not known.

         \note  The ::FOCUS_AUTO value is only meaningful when \e setting
                a focus position.
     */
    enum focus_value {
      FOCUS_GLASS = 0x40,
      FOCUS_AUTO  = 0xff,
    };

    //!  Symbolic names for the documented timeout periods.
    /*!  The numbers in the symbolic names are times in minutes.

         \sa  set_energy_saving_time
     */
    enum timeout_value {
      TIMEOUT_015 = 0x00,
      TIMEOUT_030 = 0x01,
      TIMEOUT_045 = 0x02,
      TIMEOUT_060 = 0x03,
      TIMEOUT_120 = 0x04,
      TIMEOUT_180 = 0x05,
      TIMEOUT_240 = 0x06,
    };

    //!  Symbolic names for the document alignment positions.
    /*!  This information can be used to adjust scan areas when only
         their widths and heights are provided.  This may happen with
         detected document sizes or when providing users an option to
         specify scan areas in terms of well-known paper sizes.

         \sa get_extended_identity::document_alignment(), media_value
     */
    enum alignment_value {
      ALIGNMENT_UNKNOWN = 0x00,
      ALIGNMENT_LEFT    = 0x01,
      ALIGNMENT_CENTER  = 0x02,
      ALIGNMENT_RIGHT   = 0x03,
    };

    //!  Auto-detectable media sizes.
    /*!  Some devices can detect media sizes on the fly.  These are
         the supported media size symbols.

         The ::UNKNOWN value is returned when automatic size detection
         is available but the media's size could not be determined.

         \sa http://en.wikipedia.org/wiki/Paper_size

         \todo  Add converter to area<> in mm
     */
    enum media_value {
      A3V = 0x0080,             //!<  ISO A3, portrait
      WLT = 0x0040,             //!<  US tabloid (ANSI B)
      B4V = 0x0020,             //!<  JIS B4, portrait
      LGV = 0x0010,             //!<  US legal, portrait
      A4V = 0x0008,             //!<  ISO A4, portrait
      A4H = 0x0004,             //!<  ISO A4, landscape
      LTV = 0x0002,             //!<  US letter, portrait (ANSI A)
      LTH = 0x0001,             //!<  US letter, landscape (ANSI A)
      B5V = 0x8000,             //!<  JIS B5, portrait
      B5H = 0x4000,             //!<  JIS B5, landscape
      A5V = 0x2000,             //!<  ISO A5, portrait
      A5H = 0x1000,             //!<  ISO A5, landscape
      EXV = 0x0800,             //!<  US executive, portrait
      EXH = 0x0400,             //!<  US executive, landscape
      UNK = 0x0100,             //!<  none of the above
      UNKNOWN = UNK             //!<  none of the above
    };

    //!  Push button size request values.
    /*!  The push button status may include information on the scan
         area that the user wants to scan.  These are the documented
         size request values.  When ::SIZE_REQ_CUSTOM is indicated,
         there was no user preference indicated on the device side.
         In that case the driver should use the size set via its own
         scan area options.

         \note  A value of 7 is possible but not yet documented.
         \note  It is not clear what orientation is to be used.
         \note  It is not clear what standard the B4 value refers to.

         \sa get_push_button_status::size_request()
     */
    enum size_request_value {
      SIZE_REQ_CUSTOM   = 0,    //!<  no preference from device side
      SIZE_REQ_A4       = 1,    //!<  ISO A4
      SIZE_REQ_LETTER   = 2,    //!<  US letter (ANSI A)
      SIZE_REQ_LEGAL    = 3,    //!<  US legal
      SIZE_REQ_B4       = 4,
      SIZE_REQ_A3       = 5,    //!<  ISO A3
      SIZE_REQ_TABLOID  = 6,    //!<  US tabloid (ANSI B)
    };

    //!  Symbolic names for the documented sensitivity values.
    /*!  \sa scan_parameters::double_feed_sensitivity(),
             set_scan_parameters::double_feed_sensitivity(byte)
     */
    enum sensitivity_value {
      SENSITIVITY_OFF = 0x00,
      SENSITIVITY_LO  = 0x01,
      SENSITIVITY_HI  = 0x02,
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_constant_hpp_ */
