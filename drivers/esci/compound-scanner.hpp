//  compound-scanner.hpp -- devices that handle compound commands
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_compound_scanner_hpp_
#define drivers_esci_compound_scanner_hpp_

#include <deque>
#include <string>

#include <utsushi/connexion.hpp>
#include <utsushi/constraint.hpp>
#include <utsushi/context.hpp>

#include "buffer.hpp"
#include "scanner.hpp"
#include "scanner-control.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//! \todo Add options to modify flip-side settings
class compound_scanner : public scanner
{
public:
  compound_scanner (const connexion::ptr& cnx);

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

  boost::optional< quad > transfer_format_(const parameters& p) const;
  bool compressed_transfer_(const parameters& p) const;
  std::string transfer_content_type_(const parameters& p) const;

  void queue_image_data_();
  void fill_data_queue_();
  bool media_out () const;

  //! Image size information policy query
  /*! The image size reported at the start of image data acquisition
   *  is an \e estimate.  The actual image size is only known when all
   *  image data has been received.  In certain situations it is vital
   *  and in others desirable to know the final size at the start of
   *  the acquisition process.
   */
  bool use_final_image_size_(const parameters& parm) const;
  bool enough_image_data_(const parameters& parm,
                          const std::deque< data_buffer >& q) const;

  media probe_media_size_(const string& doc_source);
  void  update_scan_area_(const media& size, value::map& vm) const;
  void  update_scan_area_range_(value::map& vm);

  bool satisfies_changing_constraint (const value::map::value_type& p,
                                      const value::map& vm,
                                      const information& info) const;

  bool validate (const value::map& vm) const;
  void finalize (const value::map& vm);

  void configure_adf_options ();
  void configure_flatbed_options ();
  void configure_tpu_options ();

  typedef boost::optional< std::vector< quad > > source_capabilities;

  void add_doc_source_options (option::map& opts,
                               const information::source& src,
                               const integer& min_w,
                               const integer& min_h,
                               const source_capabilities& src_caps,
                               const constraint::ptr& sw_res_x,
                               const constraint::ptr& sw_res_y,
                               const capabilities& caps) const;

  void add_resolution_options (option::map& opts,
                               const constraint::ptr& sw_res_x,
                               const constraint::ptr& sw_res_y,
                               const information::source& src) const;
  void add_scan_area_options (option::map& opts,
                              const integer& min_w,
                              const integer& min_h,
                              const information::source& src) const;
  void add_crop_option (option::map& opts,
                        const information::source& src,
                        const source_capabilities& src_caps,
                        const capabilities& caps) const;
  void add_deskew_option (option::map& opts,
                          const source_capabilities& src_caps) const;
  void add_overscan_option (option::map& opts,
                            const source_capabilities& src_caps) const;

  option::map& doc_source_options (const quad& q);
  option::map& doc_source_options (const value& v);
  const option::map& doc_source_options (const quad& q) const;
  const option::map& doc_source_options (const value& v) const;

  void align_document (const string& doc_source,
                       quantity& tl_x, quantity& tl_y,
                       quantity& br_x, quantity& br_y) const;

  //! Device Specific Reference Data
  /*! These variables hold various pieces of device information that
   *  is meant for reference purposes during the object's life-time.
   *  To that end they have been declared \c const.  However, as the
   *  exact values depend on the type of device used, they normally
   *  need to be modified during \e construction.  Constructors, and
   *  only constructors, are allowed to use \c const_cast in order to
   *  modify these variables.
   *
   *  At the very bottom of the chain of events that constructs any
   *  compound_scanner object, they are initialized with information
   *  obtained from the device and an optional device specific data
   *  file.  Subclass constructors are free to modify the result of
   *  this in any way necessary to make devices behave.
   *
   *  \note The min_area_width_ and min_area_height_ values are not
   *        based on any protocol information.  Their values have been
   *        determined experimentally.  At least one device did not
   *        like zero and anything less than 0.05 inch results in such
   *        small images that it is not really worth scanning anymore.
   */
  //! @{

  //! Load device information, capabilities and default parameters
  /*! The constructor's little helper, this function tries to consult
   *  a product specific file to obtain information about the device,
   *  its capabilities and default scan parameters.
   *
   *  \returns A \c true value if successful, \c false otherwise.
   */
  bool get_file_defs_(const std::string& product);

  const information  info_;
  const capabilities caps_;
  const capabilities caps_flip_;
  const parameters   defs_;
  const parameters   defs_flip_;

  const quantity min_area_width_;
  const quantity min_area_height_;

  // Preferred resolution constraints for when software emulation is
  // available
  const constraint::ptr fb_res_x_;
  const constraint::ptr fb_res_y_;
  const constraint::ptr adf_res_x_;
  const constraint::ptr adf_res_y_;
  const constraint::ptr tpu_res_x_;
  const constraint::ptr tpu_res_y_;
  //! @}

  mutable scanner_control acquire_;
  hardware_status stat_;

  parameters parm_;
  parameters parm_flip_;
  bool read_back_;

  data_buffer buffer_;
  data_buffer::size_type offset_;

  bool streaming_flip_side_image_;
  std::deque< data_buffer > face_;
  std::deque< data_buffer > rear_;
  size_t image_count_;          //!< \todo Move to base class
  sig_atomic_t cancelled_;
  bool         media_out_;
  bool         auto_crop_;

  option::map flatbed_;
  option::map adf_;
  option::map tpu_;

  integer adf_duplex_min_doc_width_;
  integer adf_duplex_min_doc_height_;
  integer adf_duplex_max_doc_width_;
  integer adf_duplex_max_doc_height_;

  context::size_type pixel_width () const;
  context::size_type pixel_height () const;
  context::_pxl_type_ pixel_type () const;
};

}       // namespace driver
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_compound_scanner_hpp_ */
