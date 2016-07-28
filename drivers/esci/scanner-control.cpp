//  scanner-control.cpp -- make the device do your bidding
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

#include <utsushi/functional.hpp>

#include "scanner-control.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace {

std::string
str (const std::vector< status::error >& err)
{
  std::string rv;

  std::vector< status::error >::const_iterator it = err.begin ();
  while (err.end () != it)
    {
      rv += esci::str (it->part);
      rv += "/";
      rv += esci::str (it->what);
      ++it;
      if (err.end () != it) rv += ", ";
    }

  return rv;
}

}       // namespace

scanner_control::scanner_control (bool pedantic)
  : base_type_(pedantic)
  , acquiring_(false)
  , do_cancel_(false)
  , cancelled_(false)
  , acquiring_face_(false)
  , acquiring_rear_(false)
  , images_started_(0)
{
  namespace reply = code_token::reply;

  // Override base class hooks
  hook_[reply::PARA] = bind (&scanner_control::set_parameters_hook_, this);
  hook_[reply::PARB] = bind (&scanner_control::set_parameters_hook_, this);
  hook_[reply::IMG ] = bind (&scanner_control::image_hook_, this);
}

scanner_control::~scanner_control ()
{
  cancel_(true);                // suppress log messages
}

scanner_control&
scanner_control::get (information& info)
{
  if (!acquiring_)
    {
      base_type_::get (info);
    }
  else
    {
      log::debug ("cannot get information while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::get (capabilities& caps, bool flip_side_only)
{
  if (!acquiring_)
    {
      base_type_::get (caps, flip_side_only);
    }
  else
    {
      log::debug ("cannot get capabilities while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::get (hardware_status& stat)
{
  if (!acquiring_)
    {
      base_type_::get (stat);
    }
  else
    {
      log::debug ("cannot get status while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::start ()
{
  namespace request = code_token::request;

  if (!acquiring_)
    {
      encode_request_block_(request::TRDT);
    }
  else
    {
      log::debug ("cannot start while acquiring image data");
    }
  return *this;
}

data_buffer
scanner_control::operator++ ()
{
  using namespace code_token;

  if (!acquiring_)
    {
      log::debug ("not in image data acquisition mode");
      return data_buffer ();
    }

  img_dat_ = data_buffer ();
  do
    {
      if (do_cancel_)      // status_.atn triggers cancel requests too
        {
          cancel_();
          if (cancelled_)
            img_dat_.atn = reply::info::atn::CAN;
          return img_dat_;
        }

      encode_request_block_(request::IMG);
      dat_ref_ = ref (static_cast< byte_buffer& > (img_dat_));
      *cnx_ << *this;
    }
  while (acquiring_
         && (0 == reply_.size && !status_.pen && !status_.pst)
         && delay_elapsed ());

  return img_dat_;
}

void
scanner_control::cancel (bool at_area_end)
{
  do_cancel_ = true;
}

scanner_control&
scanner_control::get (parameters& parm, bool flip_side_only)
{
  if (!acquiring_)
    {
      base_type_::get (parm, flip_side_only);
    }
  else
    {
      log::debug ("cannot get parameters while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::get (parameters& parm, const std::set< quad >& ts,
                      bool flip_side_only)
{
  if (!acquiring_)
    {
      base_type_::get (parm, ts, flip_side_only);
    }
  else
    {
      log::debug ("cannot get parameters while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::set (const parameters& parm, bool flip_side_only)
{
  namespace request = code_token::request;

  if (!acquiring_)
    {
      using namespace encoding;

      /*! \todo Use info_.device_buffer_size instead (minimally 1536)?
       *        That appears to be the maximum we should be sending in
       *        any one set request to begin with.  How would we split
       *        set requests that are too large?
       */
      const streamsize ballpark_figure = 1024;

      par_blk_.reserve (ballpark_figure);
      par_blk_.clear ();

      if (encode_.scan_parameters_(std::back_inserter (par_blk_), parm))
        {
          encode_request_block_(!flip_side_only
                                ? request::PARA
                                : request::PARB, par_blk_.size ());
        }
      else
        {
          log::error ("%1%") % encode_.trace ();
        }
    }
  else
    {
      log::debug ("cannot set parameters while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::set_parameters (const parameters& parm, bool flip_side_only)
{
  return set (parm, flip_side_only);
}

//! Cache parameters set on the device side for later reference
/*! This member function updates the instance's parameter cache based
 *  on the parameters that were just successfully sent to the device.
 *  Doing this here obviates the need for a call to get_parameters().
 */
void
scanner_control::set_parameters_hook_()
{
  namespace reply = code_token::reply;
  namespace par = reply::info::par;

  using decoding::grammar;

  if (status_.par
      && par::OK != *status_.par)
    {
      //! \todo Clear caches?
      log::error ("failed setting parameters (%1%)") % str (*status_.par);
      //! \todo How do we communicate this to the caller?  Throw invalid_argument?
      return;
    }

  parameters& parm (reply::PARA == reply_.code ? resa_ : resb_);

  parm.clear ();                // FIXME kludge for #811

  grammar::iterator head = par_blk_.begin ();
  grammar::iterator tail = par_blk_.end ();

  if (!decode_.scan_parameters_(head, tail, parm))
    {
      log::error ("%1%") % decode_.trace ();
    }

  // Assume that setting parameters for both sides happens in a merge
  // kind of fashion for the flip side values as well.  If this isn't
  // the case, the alternative is a full sync with resa_.

  if (reply::PARA == reply_.code)
    {
      resb_.clear ();           // FIXME kludge for #811

      grammar::iterator head = par_blk_.begin ();
      grammar::iterator tail = par_blk_.end ();

      decode_.scan_parameters_(head, tail, resb_);
    }
}

scanner_control&
scanner_control::mechanics (const quad& part, const quad& action,
                            integer value)
{
  namespace request = code_token::request;

  if (!acquiring_)
    {
      using namespace encoding;
      using namespace code_token::mechanic;

      const streamsize max_size = 16;
      hardware_request ctrl;

      /**/ if (ADF == part)
        {
          ctrl.adf = action;
        }
      else if (FCS == part)
        {
          if (fcs::AUTO == action)
            ctrl.fcs = hardware_request::focus ();
          else
            ctrl.fcs = hardware_request::focus (value);
        }
      else if (INI == part)
        {
          ctrl.ini = true;
        }
      else
        {
          log::error ("unknown hardware request type: %1%") % str (part);
          return *this;
        }

      par_blk_.reserve (max_size);
      par_blk_.clear ();

      if (encode_.hardware_control_(std::back_inserter (par_blk_), ctrl))
        {
          encode_request_block_(request::MECH, par_blk_.size ());
        }
      else
        {
          log::error ("%1%") % encode_.trace ();
        }
    }
  else
    {
      log::debug ("cannot control hardware while acquiring image data");
    }
  return *this;
}

scanner_control&
scanner_control::automatic_feed (const quad& value)
{
  namespace request = code_token::request;

  if (!acquiring_)
    {
      par_blk_.reserve (sizeof (value));
      par_blk_.clear ();

      if (encode_.automatic_feed_(std::back_inserter (par_blk_), value))
        {
          encode_request_block_(request::AFM, par_blk_.size ());
        }
      else
        {
          log::error ("%1%") % encode_.trace ();
        }
    }
  else
    {
      log::debug ("cannot set automatic feed while acquiring image data");
    }
  return *this;
}

boost::optional< std::vector < status::error > >
scanner_control::fatal_error () const
{
  if (status_.err.empty ())
    return boost::none;

  if (status_.fatal_error ()
      || (status_.media_out ()
          && (acquiring_image ()
              || expecting_more_images ()
              || 0 == images_started_)))
    return status_.err;

  return boost::none;
}

bool
scanner_control::media_out () const
{
  return (status_.media_out ()
          && !acquiring_image ()
          && !expecting_more_images ()
          && 0 < images_started_);
}

bool
scanner_control::media_out (const quad& where) const
{
  return (status_.media_out (where)
          && !acquiring_image ()
          && !expecting_more_images ()
          && 0 < images_started_);
}

bool
scanner_control::is_duplexing () const
{
  using namespace code_token::parameter;

  const parameters& parm (status_.is_flip_side () ? resb_ : resa_);

  return (parm.adf
          && parm.adf->end () != std::find (parm.adf->begin (),
                                            parm.adf->end (),
                                            adf::DPLX));
}

void
scanner_control::decode_reply_block_hook_() throw ()
{
  namespace reply = code_token::reply;
  namespace typ = reply::info::typ;

  if (reply::TRDT == reply_.code)
    {
      acquiring_ = !(!status_.err.empty ()
                     || status_.is_in_use ()
                     || status_.is_busy ());
      do_cancel_ = false;
      cancelled_ = false;

      acquiring_face_ = false;
      acquiring_rear_ = false;
      images_started_ = 0;

      if (acquiring_)
        log::brief ("starting acquisition of image(s)");
    }

  // Update the acquisition flags based on those status codes that may
  // affect them.  We process the status codes from lowest priority to
  // highest priority so that the latter take precedence.

  if (status_.atn)
    {
      if (status_.is_cancel_requested ())
        {
          log::brief ("received device initiated cancel request");
          cancel ();
        }
    }
  if (status_.pen)
    {
      if (status_.is_flip_side ())
        {
          log::brief ("finished acquisition of rear side image");
          acquiring_rear_ = false;
        }
      else
        {
          log::brief ("finished acquisition of face side image");
          acquiring_face_ = false;
        }

      parameters& parm (status_.is_flip_side () ? resb_ : resa_);

      if (parm.pag && 0 != *parm.pag)
        {
          --*parm.pag;
          if (is_duplexing ())
            --*parm.pag;
        }

      if (!parm.adf)
        {
          acquiring_ = false;
          do_cancel_ = false;
          cancelled_ = false;
        }
      else if (status_.lft
               && 0 == *status_.lft)
        {
          log::brief ("no more images left to acquire");
          acquiring_ = false;
          do_cancel_ = false;
          cancelled_ = false;
        }
      else if (status_.lft)
        {
          log::brief ("%1% image(s) left to acquire")
            % *status_.lft;
        }
    }
  if (status_.pst)
    {
      ++images_started_;
      if (status_.is_flip_side ())
        {
          acquiring_rear_ = acquiring_;
          if (acquiring_rear_)
            log::brief ("starting acquisition of rear side image");
        }
      else
        {
          acquiring_face_ = acquiring_;
          if (acquiring_face_)
            log::brief ("starting acquisition of face side image");
        }
    }
  if (!status_.err.empty ()
      || reply::CAN == reply_.code
      || reply::FIN == reply_.code)
    {
      if (acquiring_)
        {
          /**/ if (reply::FIN == reply_.code)
            {
              log::brief ("image acquisition finished");
            }
          else if (reply::CAN == reply_.code)
            {
              log::brief ("image acquisition cancelled");
            }
          else
            {
              log::brief ("image acquisition terminated: %1%")
                % str (status_.err)
                ;
            }
        }

      acquiring_ = false;
      do_cancel_ = false;
      cancelled_ = (reply::CAN == reply_.code);

      // The acquiring_face_ and acquiring_rear_ flags should *not* be
      // modified here.  Both flags are used to determine whether a PE
      // condition is an error or not.
      // See fatal_error() and media_out().
    }
}

void
scanner_control::image_hook_()
{
  static_cast< status& > (img_dat_) = status_;

  //  Do we need to hold on to pst data so we can detect end-of-image
  // (for uncompressed scans only!) independently of pen conditions?
  // That implies that we also need to track bytes received for each
  // of face and rear images.
}

void
scanner_control::cancel_(bool quietly)
{
  namespace request = code_token::request;

  if (acquiring_)
    {
      encode_request_block_(request::CAN);
      *cnx_ << *this;
    }
  else if (!quietly)
    {
      log::debug ("cannot cancel unless acquiring image data");
    }
}

bool
scanner_control::acquiring_image () const
{
  return (acquiring_face_ || acquiring_rear_);
}

bool
scanner_control::expecting_more_images () const
{
  const parameters& parm (status_.is_flip_side () ? resb_ : resa_);

  return (parm.pag && 0 != *parm.pag);
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
