//  compound.hpp -- protocol variant command base and templates
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

#ifndef drivers_esci_compound_hpp_
#define drivers_esci_compound_hpp_

#include <map>

#include <utsushi/connexion.hpp>
#include <utsushi/functional.hpp>

#include "buffer.hpp"
#include "command.hpp"
#include "grammar.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Common part of the scanner_* commands
/*! The scanner control and inquiry commands have a great deal in
 *  common.  This template follows the patterns established by the
 *  getter, setter and action templates and implements the common
 *  parts of the scanner_control and scanner_inquiry commands.
 *
 *  The I/O dynamics of these commands is captured by operator>>() and
 *  differs significantly from the non-compound commands.  Where these
 *  non-compound commands were basically single-shot transactions, the
 *  compound commands allows for multiple transactions and need to be
 *  terminated explicitly.  In that sense they are closer in spirit to
 *  the start_scan commands.
 *
 *  The template provides protected member functions that prepare each
 *  request before it can be sent.  Subclasses can build upon these to
 *  provide a public API.  In typical use, one would send a request as
 *  follows:
 *
 *    \code
 *    cnx << cmd.information ();
 *    \endcode
 *
 *  The information() member sets up request and payload buffers and
 *  operator>>() stuffs these down the connexion in the correct order.
 *  After getting the device reply and corresponding payload (if any),
 *  operator>>() processes the request invariant part as well.  If all
 *  went well, a request specific hook function is called that handles
 *  the remainder of the request.
 *
 *  While the compound command template focusses on the protocol's I/O
 *  aspects, it is engaged in a protective symbiotic relationship with
 *  something that specializes in the encoding/decoding business: the
 *  grammar.
 */
class compound_base
  : public command
{
public:
  typedef decoding::grammar::expectation_failure expectation_failure;

  //! Destroys an instance
  /*! Makes sure the compound command is correctly terminated and
   *  releases any resources acquired throughout its lifetime.
   */
  virtual ~compound_base ();

  //! Executes a request on the other end of a connexion
  /*! The command's signature, the \c b1 \c b2 two-byte sequence, will
   *  be sent if necessary.  A reference to \a cnx will be remembered
   *  by the command, so it can make sure following requests are sent
   *  to the same destination.  This reference is cleared when sending
   *  a finish() request.
   *
   *  If a reply data block is indicated in the reply header data, it
   *  will be fetched, irrespective of any error indications in the
   *  header (either in its data or its info).  Error handling only
   *  starts \e after the reply data block has been received.
   */
  void operator>> (connexion& cnx);

  //! Check if a compound command session has started already
  bool is_in_session () const;
  //! \todo Expose more queries for status_
  bool is_busy () const;
  bool is_warming_up () const;

  //! \todo Change to independent utility function taking an interval
  bool delay_elapsed () const;

protected:
  bool pedantic_;               //!< Checking of replies or not

  connexion *cnx_;

  static const streamsize req_len_;
  static const streamsize hdr_len_;

  byte_buffer req_blk_;         //!< Encoded request header
  byte_buffer par_blk_;         //!< Encoded request payload
  byte_buffer hdr_blk_;         //!< Encoded reply header
  byte_buffer dat_blk_;         //!< Encoded reply payload

  reference_wrapper< byte_buffer > dat_ref_;

  header request_;              //!< Decoded request
  header reply_;                //!< Decoded reply
  status status_;               //!< Decoded status

  encoding::grammar encode_;
  decoding::grammar decode_;

  information     info_;
  capabilities    capa_;
  capabilities    capb_;
  parameters      resa_;
  parameters      resb_;
  hardware_status stat_;

  reference_wrapper< information >     info_ref_;
  reference_wrapper< capabilities >    caps_ref_;
  reference_wrapper< parameters >      parm_ref_;
  reference_wrapper< hardware_status > stat_ref_;

  //! Collection of per request callbacks, indexed by reply code_token
  std::map< quad, function< void () > > hook_;

  //! Creates an optionally pedantic instance
  /*! A map with hook functions for use in operator>>() is initialized
   *  with appropriate functions.  Subclasses should override relevant
   *  hooks in their constructor.
   */
  compound_base (bool pedantic = false);

  //! Sends a command's tell-tale bytes down the connexion if needed
  virtual void send_command_signature_(connexion& cnx) = 0;

  //! Pulls a data block off the wire
  void recv_data_block_();

  //! Prepares a request block for a request \a code
  /*! The \a code and \a size are encoded into the format expected by
   *  the device.  If successful, \c true is returned and the \a code
   *  and \a size are stored in the instance's request_.  Otherwise,
   *  \c false is returned and request_ is \e not modified.
   */
  bool encode_request_block_(const quad& code, integer size = 0);

  //! Interprets the content of a reply block
  /*! Takes a reply header apart and stores all its findings in the
   *  instance's reply_ and status_ member variables.  In pedantic_
   *  mode a status::check() is performed.  Any not-ready status is
   *  logged and signalled to interested parties.
   *
   *  After that is done a decode_reply_block_hook_() function is
   *  called to allow subclasses to act upon the parse results.
   *
   *  \todo Implement parse error handling
   *  \todo Signal not-ready status if any
   *  \todo Decide whether the above should be done as part of this
   *        function or rather after operator>>() has returned from
   *        recv_data_block_().  Note that a data block, if any, \c
   *        has to be received, even in case of errors (other than
   *        I/O errors).
   */
  void decode_reply_block_();

  //! Acts upon the result of decode_reply_block_()
  /*! The default implementation does absolutely nothing.
   */
  virtual void decode_reply_block_hook_() throw ();

  //! Sets up a request to finish the session
  compound_base& finish ();

  //! Forget about the command object's connexion
  void finish_hook_();

  void unknown_request_hook_();
  void invalid_request_hook_();

  //! Sets up a request to get device information
  virtual compound_base& get (information& info);
  compound_base& get_information ();

  //! Decodes the information request's reply payload
  void get_information_hook_();

  //! Sets up a request to retrieve device capabilities
  /*! By default, capabilities applicable to both sides of the medium
   *  are retrieved.  One may, however, request capabilities for the
   *  \a flip_side_only, too.
   */
  virtual compound_base& get (capabilities& caps, bool flip_side_only = false);
  compound_base& get_capabilities (bool flip_side_only = false);

  //! Decodes the capability request's reply payload
  void get_capabilities_hook_();

  //! Sets up a request to obtain \e all current scan parameters
  /*! Normally, settings for both sides of the medium are obtained.
   *  To get settings for the \a flip_side_only, say so.
   */
  virtual compound_base& get (parameters& parm, bool flip_side_only = false);
  compound_base& get_parameters (bool flip_side_only = false);

  //! Sets up a request to obtain a subset of current scan parameters
  /*! This member constructs a request to get the scan settings for
   *  all the tokens from a set \a ts.  The \a flip_side_only flag
   *  controls whether the settings obtained apply to both sides of
   *  the medium or not.
   *
   *  \todo Replace with a templated forward-iterator version
   */
  virtual compound_base& get (parameters& parm, const std::set< quad >& ts,
                              bool flip_side_only = false);
  compound_base& get_parameters (const std::set< quad >& ts,
                                 bool flip_side_only = false);

  //! Decodes the get_parameters() request's reply payload
  void get_parameters_hook_();

  //! Sets up a request to acquire the device's status
  virtual compound_base& get (hardware_status& stat);
  compound_base& get_status ();

  //! Decodes the status request's reply payload
  void get_status_hook_();

  //! Sets up a request to send a \a request_payload to the device
  /*! Such a payload may be sent to any of three sequentially numbered
   *  extensions.  The extension number is controlled via \a n, with a
   *  default of zero (the lowest number).
   *
   *  The effect of sending a payload as well as the handling of its
   *  \e optional \a reply_payload is device specific.
   */
  compound_base& extension (const byte_buffer& request_payload,
                            byte_buffer& reply_payload, size_t n = 0);

  void extension_hook_();

  //! Doesn't do a thing
  /*! Used as the default hook in the initialisation of the \c hook_
   *  map in our constructor this hook does nothing.
   */
  void noop_hook_();

  //! Sets up device for a request session
  /*! The connexion is stored so that any future requests are sent to
   *  the same device.
   */
  void
  send_signature_(connexion& cnx, const byte cmd[2]);

  bool is_ready_() const;
};

template< byte b1, byte b2 >
class compound
  : public compound_base
{
public:
  compound (bool pedantic)
    : compound_base (pedantic)
  {}

protected:
  //! Conveniently refer to the parent class from subclasses
  typedef compound base_type_;

  static const byte cmd_[2];    //!< Command bytes

  void
  send_command_signature_(connexion& cnx)
  {
    send_signature_(cnx, cmd_);
  }
};

template< byte b1, byte b2 >
const byte compound< b1, b2 >::cmd_[2] = { b1, b2 };

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_compound_hpp_ */
