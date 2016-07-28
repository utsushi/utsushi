//  compound.cpp -- protocol variant command base class implementation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>

#include <boost/throw_exception.hpp>

#include "compound.hpp"
#include "exception.hpp"

#if __cplusplus >= 201103L
#define NS std
#else
#define NS boost
#endif

namespace utsushi {
namespace _drv_ {
namespace esci {

compound_base::~compound_base ()
{
  namespace reply = code_token::reply;

  if (cnx_)
    {
      // At this point, any subclass part has been blown to bits.
      // We had better make sure we have a valid hook to execute.
      // We cannot run the subclass' hook here anymore.

      hook_[reply::FIN ] = bind (&compound_base::finish_hook_, this);
      *cnx_ << finish ();
    }
}

void
compound_base::operator>> (connexion& cnx)
{
  namespace request = code_token::request;
  namespace reply   = code_token::reply;

  if (!cnx_)
    send_command_signature_(cnx);

  if (cnx_ != &cnx)
    BOOST_THROW_EXCEPTION (logic_error ("crossed wires"));

  if (!request_.code)
    return;

  do
    {
      cnx_->send (req_blk_.data (), req_len_);

      if (0 < request_.size)
        cnx_->send (par_blk_.data (), request_.size);

      cnx_->recv (hdr_blk_.data (), hdr_len_);
      decode_reply_block_();

      if (0 < reply_.size)
        recv_data_block_();

      if (request_.code != reply_.code)
        {
          if (request::FIN != request_.code)
            {
              /**/ if (reply::UNKN == reply_.code) {} // defer to hook
              else if (reply::INVD == reply_.code) {} // defer to hook
              else
                {
                  log::fatal ("%1%: %2% request got a %3% reply,"
                              " terminating compound command session")
                    % info_.product_name ()
                    % str (request_.code)
                    % str (reply_.code)
                    ;
                  *cnx_ << finish ();
                }
            }
          else                  // something went very wrong
            {
              BOOST_THROW_EXCEPTION (protocol_error ());
            }
        }
      hook_[reply_.code] ();
    }
  while (!is_ready_() && delay_elapsed ());

  request_.code = quad ();
}

bool
compound_base::is_in_session () const
{
  return cnx_;
}

bool
compound_base::is_busy () const
{
  using namespace code_token;

  return status_.is_busy () && reply::MECH != reply_.code;
}

bool
compound_base::is_warming_up () const
{
  return status_.is_warming_up ();
}

bool
compound_base::delay_elapsed () const
{
  struct timespec t = { 0, 100000000 /* ns */ };

  return 0 == nanosleep (&t, 0);
}

const streamsize compound_base::req_len_ = 12;
const streamsize compound_base::hdr_len_ = 64;

compound_base::compound_base (bool pedantic)
  : pedantic_(pedantic)
  , cnx_(0)
  , req_blk_(req_len_)
  , par_blk_()
  , hdr_blk_(hdr_len_)
  , dat_blk_()
  , dat_ref_(NS::ref (dat_blk_))
  , info_ref_(NS::ref (info_))
  , caps_ref_(NS::ref (capa_))
  , parm_ref_(NS::ref (resa_))
  , stat_ref_(NS::ref (stat_))
{
  namespace reply = code_token::reply;

  hook_[reply::FIN ] = bind (&compound_base::finish_hook_, this);
  hook_[reply::CAN ] = bind (&compound_base::noop_hook_, this);
  hook_[reply::UNKN] = bind (&compound_base::unknown_request_hook_, this);
  hook_[reply::INVD] = bind (&compound_base::invalid_request_hook_, this);
  hook_[reply::INFO] = bind (&compound_base::get_information_hook_, this);
  hook_[reply::CAPA] = bind (&compound_base::get_capabilities_hook_, this);
  hook_[reply::CAPB] = bind (&compound_base::get_capabilities_hook_, this);
  hook_[reply::PARA] = bind (&compound_base::noop_hook_, this);
  hook_[reply::PARB] = bind (&compound_base::noop_hook_, this);
  hook_[reply::RESA] = bind (&compound_base::get_parameters_hook_, this);
  hook_[reply::RESB] = bind (&compound_base::get_parameters_hook_, this);
  hook_[reply::STAT] = bind (&compound_base::get_status_hook_, this);
  hook_[reply::MECH] = bind (&compound_base::noop_hook_, this);
  hook_[reply::TRDT] = bind (&compound_base::noop_hook_, this);
  hook_[reply::IMG ] = bind (&compound_base::noop_hook_, this);
  hook_[reply::EXT0] = bind (&compound_base::extension_hook_, this);
  hook_[reply::EXT1] = bind (&compound_base::extension_hook_, this);
  hook_[reply::EXT2] = bind (&compound_base::extension_hook_, this);
}

void
compound_base::recv_data_block_()
{
  dat_ref_.get ().resize (reply_.size);
  cnx_->recv (dat_ref_.get ().data (), reply_.size);
}

bool
compound_base::encode_request_block_(const quad& code, integer size)
{
  using namespace encoding;

  req_blk_.clear ();

  bool r = encode_.header_(std::back_inserter (req_blk_),
                           header (code, size));

  dat_ref_ = NS::ref (dat_blk_);
  info_ref_ = NS::ref (info_);
  caps_ref_ = NS::ref (capa_);
  parm_ref_ = NS::ref (resa_);
  stat_ref_ = NS::ref (stat_);

  if (r)
    {
      request_ = header (code, size);
    }
  else
    {
      log::error ("%1%") % encode_.trace ();
    }

  return r;
}

void
compound_base::decode_reply_block_()
{
  using namespace decoding;

  grammar::iterator head = hdr_blk_.begin ();
  grammar::iterator info = head + req_len_;
  grammar::iterator tail = head + hdr_len_;

  if (!decode_.header_(head, info, reply_))
    {
      log::error ("%1%") % decode_.trace ();
    }

  status_.clear ();             // so we don't merge status info

  if (!decode_.status_(info, tail, status_))
    {
      log::error ("%1%") % decode_.trace ();
    }

  if (pedantic_) status_.check (reply_);

  if (!status_.is_ready ())
    {
      log::brief ("device is not ready: %1%") % str (*status_.nrd);
    }

  decode_reply_block_hook_();
}

//! \todo Add handling of doc here or in overrides
void
compound_base::decode_reply_block_hook_() throw ()
{
}

compound_base&
compound_base::finish ()
{
  namespace request = code_token::request;

  encode_request_block_(request::FIN);

  return *this;
}

void
compound_base::finish_hook_()
{
  noop_hook_();

  if (is_ready_()) cnx_ = 0;
}

void
compound_base::unknown_request_hook_()
{
  log::error ("%1%: %2% request unknown")
    % info_.product_name ()
    % str (request_.code)
    ;
  noop_hook_();
}

void
compound_base::invalid_request_hook_()
{
  log::error ("%1%: %2% request invalid at this point")
    % info_.product_name ()
    % str (request_.code)
    ;
  noop_hook_();
}

compound_base&
compound_base::get (information& info)
{
  namespace request = code_token::request;

  if (encode_request_block_(request::INFO))
    {
      info_ref_ = NS::ref (info);
    }

  return *this;
}

compound_base&
compound_base::get_information ()
{
  return get (info_);
}

void
compound_base::get_information_hook_()
{
  using namespace decoding;

  grammar::iterator head = dat_blk_.begin ();
  grammar::iterator tail = head + reply_.size;

  info_.clear ();
  if (decode_.information_(head, tail, info_))
    {
      info_ref_.get () = info_;
    }
  else
    {
      log::error ("%1%") % decode_.trace ();
    }
}

compound_base&
compound_base::get (capabilities& caps, bool flip_side_only)
{
  namespace request = code_token::request;

  if (encode_request_block_(!flip_side_only
                            ? request::CAPA
                            : request::CAPB))
    {
      caps_ref_ = NS::ref (caps);
    }

  return *this;
}

compound_base&
compound_base::get_capabilities (bool flip_side_only)
{
  return get (!flip_side_only
              ? capa_
              : capb_,
              flip_side_only);
}

void
compound_base::get_capabilities_hook_()
{
  namespace reply = code_token::reply;

  using namespace decoding;

  if (reply::CAPB == reply_.code)
    {
      if (0 == reply_.size)
        {
          caps_ref_.get () = capabilities ();
          return;
        }
    }

  capabilities& caps (reply::CAPA == reply_.code
                      ? capa_
                      : capb_);

  grammar::iterator head = dat_blk_.begin ();
  grammar::iterator tail = head + reply_.size;

  caps.clear ();
  if (decode_.capabilities_(head, tail, caps))
    {
      caps_ref_.get () = caps;
    }
  else
    {
      log::error ("%1%") % decode_.trace ();
    }
}

compound_base&
compound_base::get (parameters& parm, bool flip_side_only)
{
  namespace request = code_token::request;

  if (encode_request_block_(!flip_side_only
                            ? request::RESA
                            : request::RESB))
    {
      par_blk_.clear ();
      parm_ref_ = NS::ref (parm);
    }

  return *this;
}

compound_base&
compound_base::get_parameters (bool flip_side_only)
{
  return get (!flip_side_only
              ? resa_
              : resb_,
              flip_side_only);
}

compound_base&
compound_base::get (parameters& parm, const std::set< quad >& ts,
                    bool flip_side_only)
{
  using namespace encoding;

  namespace request = code_token::request;

  if (ts.empty ())
    return get_parameters (flip_side_only);

  par_blk_.reserve (sizeof (quad) * ts.size ());
  par_blk_.clear ();

  if (encode_.parameter_subset_(std::back_inserter (par_blk_), ts))
    {
      if (encode_request_block_(!flip_side_only
                                ? request::RESA
                                : request::RESB, par_blk_.size ()))
        {
          parm_ref_ = NS::ref (parm);
        }
    }
  else
    {
      log::error ("%1%") % encode_.trace ();
    }

  return *this;
}

compound_base&
compound_base::get_parameters (const std::set< quad >& ts,
                               bool flip_side_only)
{
  return get (!flip_side_only
              ? resa_
              : resb_,
              ts, flip_side_only);
}

/*! \bug Parameters get requests for ADF, TPU and FB may return no
 *       data block if that document source is not available.  The
 *       implementation will incorrectly log this as a parse error.
 *  \bug #parLOST is not handled
 */
void
compound_base::get_parameters_hook_()
{
  namespace reply = code_token::reply;
  namespace par = reply::info::par;

  using decoding::grammar;

  if (0 == reply_.size && reply::RESB == reply_.code)
    {
      resb_ = resa_;            // for internal consistency
      parm_ref_.get () = resa_;
      return;
    }

  parameters& parm (reply::RESA == reply_.code
                    ? resa_
                    : resb_);

  if (par_blk_.empty ())        // requested all parameters
    {
      parm.clear ();
    }

  // Requesting parameters for a non-existent document source (ADF,
  // TPU, FB) should return an empty data block.

  if (0 < reply_.size)
    {
      grammar::iterator head = dat_blk_.begin ();
      grammar::iterator tail = head + reply_.size;

      if (decode_.scan_parameters_(head, tail, parm))
        {
          parm_ref_.get () = parm;
        }
      else
        {
          log::error ("%1%") % decode_.trace ();
        }
    }

  if (status_.par
      && par::OK != *status_.par)
    {
      log::error ("failed getting parameters (%1%)") % str (*status_.par);
    }
}

compound_base&
compound_base::get (hardware_status& stat)
{
  namespace request = code_token::request;

  if (encode_request_block_(request::STAT))
    {
      stat.clear ();            // hook may bypass updating of stat_ref_
      stat_ref_ = NS::ref (stat);
    }

  return *this;
}

compound_base&
compound_base::get_status ()
{
  return get (stat_);
}

void
compound_base::get_status_hook_()
{
  using namespace decoding;

  grammar::iterator head = dat_blk_.begin ();
  grammar::iterator tail = head + reply_.size;

  if (head != tail)             // there was something to report on
    {
      stat_.clear ();           // so we don't merge status info
      if (decode_.hardware_status_(head, tail, stat_))
        {
          stat_ref_.get () = stat_;
        }
      else
        {
          log::error ("%1%") % decode_.trace ();
        }
    }
}

compound_base&
compound_base::extension (const byte_buffer& request_payload,
                          byte_buffer& reply_payload, size_t n)
{
  namespace request = code_token::request;

  static const quad ext_[]
    = { request::EXT0,
        request::EXT1,
        request::EXT2,
  };

  if (n >= sizeof (ext_) / sizeof (*ext_))
    BOOST_THROW_EXCEPTION
      (domain_error ("unknown extension designation"));

  par_blk_ = request_payload;
  bool r = encode_request_block_(ext_[n], par_blk_.size ());

  if (r)
    {
      dat_ref_ = NS::ref (reply_payload);
      dat_ref_.get ().clear ();
    }

  return *this;
}

void
compound_base::extension_hook_()
{
  dat_ref_ = NS::ref (dat_blk_);
}

void
compound_base::noop_hook_()
{
  if (0 == reply_.size) return;

  log::trace ("%1%: ignoring unexpected payload (%2% bytes)")
    % str (reply_.code)
    % reply_.size;
}

void
compound_base::send_signature_(connexion& cnx, const byte cmd[2])
{
  if (!cnx_)
    {
      byte rep;

      cnx.send (cmd, 2);
      cnx.recv (&rep, 1);

      if (ACK != rep)
        {
          if (NAK == rep)
            BOOST_THROW_EXCEPTION (invalid_command ());

          BOOST_THROW_EXCEPTION (unknown_reply ());
        }
      cnx_ = &cnx;
    }
  else
    {
      if (pedantic_ && !request_.code)
        {
          log::brief ("ignoring attempt to resend command bytes");
          log::trace ("attempt hints at a logic error in the code");
        }
    }
}

bool
compound_base::is_ready_() const
{
  using namespace code_token;
  namespace nrd = reply::info::nrd;

  if (status_.is_in_use ())
    BOOST_THROW_EXCEPTION
      (device_busy (SEC_("The device is in use.  Please wait until the"
                         " device becomes available, then try again.")));

  return !(status_.is_busy ()
           || (status_.is_warming_up () && reply::MECH != reply_.code));
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
