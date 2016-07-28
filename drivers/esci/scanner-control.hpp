//  scanner-control.hpp -- make the device do your bidding
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_scanner_control_hpp_
#define drivers_esci_scanner_control_hpp_

#include "buffer.hpp"
#include "compound.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

class data_buffer
  : public byte_buffer
  , public status
{
public:
  using byte_buffer::clear;
};

//! Make the device do your bidding
/*!
 */
class scanner_control
  : public compound< FS, UPPER_X >
{
public:
  scanner_control (bool pedantic = false);
  virtual ~scanner_control ();

  using base_type_::finish;
  using base_type_::get_information;
  using base_type_::get_capabilities;
  using base_type_::get_status;
  using base_type_::get_parameters;

  //! \name Device Characteristics
  //! @{
  scanner_control& get (information& info);
  scanner_control& get (capabilities& caps, bool flip_side_only = false);
  scanner_control& get (hardware_status& stat);
  //! @}

  //! \name Image Acquisition Controls
  //! @{
  //! \copybrief start_scan::operator>>()
  scanner_control& start ();

  //! \copybrief start_scan::operator++()
  /*! \copydetails start_scan::operator++()
   */
  data_buffer operator++ ();

  //! \copybrief start_scan::cancel()
  /*! \copydetails start_scan::cancel()
   */
  void cancel (bool at_area_end = false);
  //! @}

  //! \name Scan Parameters
  //! @{
  scanner_control& get (parameters& parm, bool flip_side_only = false);
  scanner_control& get (parameters& parm, const std::set< quad >& ts,
                        bool flip_side_only = false);
  scanner_control& set (const parameters& parm, bool flip_side_only = false);
  scanner_control& set_parameters (const parameters& parm,
                                   bool flip_side_only = false);
  //! @}

  //! \name Mechanical Controls
  //! @{
  scanner_control& mechanics (const quad& part, const quad& action = quad (),
                              integer value = 0);
  scanner_control& automatic_feed (const quad& value);
  //! @}

  using base_type_::extension;

  boost::optional< std::vector< status::error > > fatal_error () const;

  //! Indicates expected out-of-media conditions
  bool media_out () const;
  bool media_out (const quad& where) const;
  bool is_duplexing () const;

protected:
  bool acquiring_;              //!< Has acquisition been initiated
  bool do_cancel_;              //!< Should acquisition be aborted
  bool cancelled_;              //!< Has acquisition been aborted

  bool acquiring_face_;         //!< Has face side acquisition started
  bool acquiring_rear_;         //!< Has rear side acquisition started
  int  images_started_;

  data_buffer img_dat_;

  /*! Extends reply block decoding to add state switching logic.
   */
  void decode_reply_block_hook_() throw ();

  void cancel_(bool quietly = false);     //!< \sa abort_scan

  void set_parameters_hook_();
  void image_hook_();

  bool acquiring_image () const;
  bool expecting_more_images () const;
};

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_scanner_control_hpp_ */
