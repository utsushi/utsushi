//  device.cpp -- interface default implementations
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include "utsushi/device.hpp"
#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"

namespace utsushi {

using std::exception;
using std::logic_error;

idevice::idevice (const context& ctx)
  : input (ctx), last_marker_(traits::eos())
  , do_cancel_(false)
{}

streamsize
idevice::read (octet *data, streamsize n)
{
  try
    {
      return read_(data, n);
    }
  catch (const exception& e)
    {
      last_marker_ = traits::eof ();
      throw;
    }
}

streamsize
idevice::read_(octet *data, streamsize n)
{
  if (traits::boi() == last_marker_)
    {
      if (0 < n)
        {
          streamsize rv = sgetn (data, n);
          if (0 >= rv)
            {
              finish_image ();
              last_marker_ = (0 == rv
                              ? traits::eoi ()
                              : traits::eof ());
            }
          else
            {
              signal_update_(ctx_.octets_seen () += rv,
                             ctx_.octets_per_image ());
              return rv;
            }
        }
    }
  else if (traits::eoi() == last_marker_)
    {
      last_marker_ = (is_consecutive ()
                      && obtain_media ()
                      && set_up_image ()
                      ? traits::boi()
                      : traits::eos());
    }
  else if (   traits::eos() == last_marker_
           || traits::eof() == last_marker_)
    {
      last_marker_ = (set_up_sequence ()
                      && obtain_media ()
                      ? traits::bos()
                      : traits::eos());
    }
  else if (traits::bos() == last_marker_)
    {
      last_marker_ = (set_up_image ()
                      ? traits::boi()
                      : traits::eos());
    }
  else
    {
      BOOST_THROW_EXCEPTION
        (logic_error (_("unhandled state in idevice::read()")));
    }

  if (do_cancel_ && traits::eos() == last_marker_)
    {
      last_marker_ = traits::eof();
    }
  if (traits::eof() == last_marker_)
    {
      do_cancel_ = false;
    }

  //! \todo Don't emit if not changed
  signal_marker_(last_marker_);

  return last_marker_;
}

streamsize
idevice::marker ()
{
  return read (NULL, 0);
}

void
idevice::cancel ()
{
  if (do_cancel_) return;

  log::brief ("initiating cancellation");

  do_cancel_ = true;
  cancel_();
}

void
idevice::buffer_size (streamsize size)
{
  buffer_size_ = size;
}

bool
idevice::is_single_image () const
{
  return false;
}

bool
idevice::set_up_sequence ()
{
  return true;
}

bool
idevice::is_consecutive () const
{
  return false;
}

bool
idevice::obtain_media ()
{
  return true;
}

bool
idevice::set_up_image ()
{
  return false;
}

void
idevice::finish_image ()
{}

streamsize
idevice::sgetn (octet *data, streamsize n)
{
  return 0;
}

void
idevice::cancel_()
{
}

void
odevice::mark (traits::int_type c, const context& ctx)
{
  output::mark (c, ctx);

  //! \todo Don't emit if not changed
  if (traits::is_marker (c))
    signal_marker_(c);
}

void
odevice::buffer_size (streamsize size)
{
  buffer_size_ = size;
}

decorator<idevice>::decorator (ptr instance)
  : instance_(instance)
{}

streamsize
decorator<idevice>::read (octet *data, streamsize n)
{
  return instance_->read (data, n);
}

streamsize
decorator<idevice>::marker ()
{
  return instance_->marker ();
}

void
decorator<idevice>::cancel ()
{
  return instance_->cancel ();
}

streamsize
decorator<idevice>::buffer_size () const
{
  return instance_->buffer_size ();
}

void
decorator<idevice>::buffer_size (streamsize size)
{
  instance_->buffer_size (size);
}

context
decorator<idevice>::get_context () const
{
  return instance_->get_context ();
}

decorator<odevice>::decorator (ptr instance)
  : instance_(instance)
{}

streamsize
decorator<odevice>::write (const octet *data, streamsize n)
{
  return instance_->write (data, n);
}

void
decorator<odevice>::mark (traits::int_type c, const context& ctx)
{
  instance_->mark (c, ctx);
}

streamsize
decorator<odevice>::buffer_size () const
{
  return instance_->buffer_size ();
}

void
decorator<odevice>::buffer_size (streamsize size)
{
  instance_->buffer_size (size);
}

context
decorator<odevice>::get_context () const
{
  return instance_->get_context ();
}

}       // namespace utsushi
