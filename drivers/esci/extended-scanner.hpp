//  extended-scanner.hpp -- devices that handle extended commands
//  Copyright (C) 2013  Olaf Meeuwissen
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

#ifndef drivers_esci_extended_scanner_hpp_
#define drivers_esci_extended_scanner_hpp_

#include <utsushi/connexion.hpp>
#include <utsushi/octet.hpp>

#include "chunk.hpp"
#include "get-extended-identity.hpp"
#include "get-scan-parameters.hpp"
#include "get-scanner-status.hpp"
#include "scanner.hpp"
#include "set-scan-parameters.hpp"
#include "start-extended-scan.hpp"

#define start_extended_scan start_ext_scan_alloc

namespace utsushi {
namespace _drv_ {
namespace esci {

class extended_scanner : public scanner
{
public:
  extended_scanner (const connexion::ptr& cnx);

  void configure ();

  bool is_single_image () const;

protected:
  bool is_consecutive () const;
  bool obtain_media ();
  bool set_up_image ();
  void finish_image ();
  streamsize sgetn (octet *data, streamsize n);

  void set_up_initialize ();
  bool set_up_hardware ();

  void set_up_auto_area_segmentation ();
  void set_up_brightness ();
  void set_up_color_matrices ();
  void set_up_dithering ();
  void set_up_doc_source ();
  void set_up_gamma_tables ();
  void set_up_image_mode ();
  void set_up_mirroring ();
  void set_up_resolution ();
  void set_up_scan_area ();
  void set_up_scan_count ();
  void set_up_scan_speed ();
  void set_up_sharpness ();
  void set_up_threshold ();
  void set_up_transfer_size ();

  bool validate (const value::map& vm) const;
  void finalize (const value::map& vm);

  option::map& doc_source_options (const value& v);
  const option::map& doc_source_options (const value& v) const;

  void configure_doc_source_options ();
  void add_resolution_options ();
  void add_scan_area_options (option::map& opts, const source_value& src);

  media probe_media_size_(const string& doc_source);
  void  update_scan_area_(const media& size, value::map& vm) const;
  void  align_document (const string& doc_source,
                        quantity& tl_x, quantity& tl_y,
                        quantity& br_x, quantity& br_y) const;

  uint32_t get_pixel_alignment ();
  uint32_t clip_to_physical_scan_area_width (uint32_t tl_x, uint32_t br_x);
  uint32_t clip_to_max_pixel_width (uint32_t tl_x, uint32_t br_x);

  void configure_color_correction ();

  void lock_scanner ();
  void unlock_scanner ();

  const get_extended_identity caps_;
  const get_scan_parameters   defs_;

  // Preferred resolution constraints for when software emulation is
  // available
  const constraint::ptr res_;
  //! @}

  start_extended_scan acquire_;
  get_scanner_status  stat_;

  const quantity min_area_width_;
  const quantity min_area_height_;

  set_scan_parameters parm_;
  bool read_back_;              //!< \todo Move to base class

  chunk      chunk_;
  streamsize offset_;

  sig_atomic_t cancelled_;      //!< \todo Move to base class

  int  images_started_;

  option::map flatbed_;
  option::map adf_;
  option::map tpu_;

  context::size_type pixel_width () const;
  context::size_type pixel_height () const;
  context::_pxl_type_ pixel_type () const;

  bool locked_;
};

}       // namespace driver
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_extended_scanner_hpp_ */
