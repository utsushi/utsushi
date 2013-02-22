//  pump.cpp -- move image octets from a source to a sink
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

#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"
#include "utsushi/pump.hpp"

namespace utsushi {

using std::invalid_argument;

static const key ASYNC ("acquire-async");

static void
init_(option::map::ptr& option_)
{
  option_->add_options ()
    (ASYNC, toggle (true),
     attributes (/* level::debug */),
     N_("Acquire image data asynchronously"),
     N_("When true, image acquisition will proceed independently from"
        " the rest of the program.  Normally, this would be what you"
        " want because it keeps the program responsive to user input"
        " and updated with respect to progress.  However, in case of"
        " trouble shooting you may want to turn this off to obtain a"
        " more predictable program flow.\n"
        "Note, you may no longer be able to cancel image acquisition"
        " via the normal means when this option is set to false.")
     );
}

pump::pump (idevice::ptr idev)
  : iptr_(idev)
  , is_cancelling_(false)
  , is_pumping_(false)
  , thread_(0)
{
  require_(iptr_);
  init_(option_);
}

pump::pump (istream::ptr istr)
  : iptr_(istr)
  , is_cancelling_(false)
  , is_pumping_(false)
  , thread_(0)
{
  require_(iptr_);
  init_(option_);
}

pump::~pump ()
{
  if (thread_)
    {
      if (is_pumping_)
        cancel ();
      thread_->join ();
    }
  delete thread_;
}

void
pump::start (odevice::ptr odev)
{
  require_(odev);
  start_(odev);
}

void
pump::start (ostream::ptr ostr)
{
  require_(ostr);
  start_(ostr);
}

void
pump::cancel ()
{
  iptr_->cancel ();
  is_cancelling_ = true;
}

connection
pump::connect (const notify_signal_type::slot_type& slot) const
{
  return signal_notify_.connect (slot);
}

void
pump::start_(output::ptr optr)
{
  toggle acquire_asynchronously = value ((*option_)[ASYNC]);

  if (!acquire_asynchronously)
    {
      log::trace ("acquiring image data synchronously");
      optr_ = optr;
      acquire_();
      return;
    }

  if (!is_cancelling_ && is_pumping_)
    {
      log::error ("still acquiring, cancel first");
      return;
    }

  if (is_cancelling_)
    {
      log::brief ("waiting for cancellation to complete");
      if (thread_) thread_->join ();
      is_cancelling_ = false;
    }

  optr_ = optr;
  delete thread_;

  thread_ = new thread (&pump::acquire_, this);
}

void
pump::acquire_()
{
  try
    {
      is_pumping_ = true;
      *iptr_ | *optr_;
    }
  catch (std::runtime_error& e)
    {
      optr_->mark (traits::eof (), context ());
      signal_notify_(log::ALERT, e.what ());
    }
  is_pumping_ = false;
}

void
pump::require_(input::ptr iptr)
{
  if (iptr) return;

  BOOST_THROW_EXCEPTION (invalid_argument (_("no image data source")));
}

void
pump::require_(output::ptr optr)
{
  if (optr) return;

  BOOST_THROW_EXCEPTION (invalid_argument (_("no output destination")));
}

}       // namespace utsushi
