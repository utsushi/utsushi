//  pump.cpp -- move image octets from a source to a sink
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

#include <csignal>
#include <deque>
#include <new>
#include <stdexcept>

#include <boost/throw_exception.hpp>

#include "utsushi/condition-variable.hpp"
#include "utsushi/i18n.hpp"
#include "utsushi/log.hpp"
#include "utsushi/memory.hpp"
#include "utsushi/mutex.hpp"
#include "utsushi/pump.hpp"
#include "utsushi/thread.hpp"

namespace utsushi {

using std::invalid_argument;

class bucket
{
public:
  typedef shared_ptr< bucket > ptr;

  octet *data_;
  union {
    streamsize size_;
    streamsize mark_;
  };
  context ctx_;

  bucket (streamsize size)
    : data_(new octet[size])
    , size_(size)
  {}

  bucket (const context& ctx, streamsize marker)
    : data_(nullptr)
    , mark_(marker)
    , ctx_(ctx)
  {}

  ~bucket ()
  {
    delete [] data_;
  }
};

namespace {

const key ASYNC ("acquire-async");

void
init_(option::map::ptr& option_)
{
  option_->add_options ()
    (ASYNC, toggle (true),
     attributes (/* level::debug */),
     CCB_N_("Acquire image data asynchronously"),
     CCB_N_("When true, image acquisition will proceed independently from"
            " the rest of the program.  Normally, this would be what you"
            " want because it keeps the program responsive to user input"
            " and updated with respect to progress.  However, in case of"
            " trouble shooting you may want to turn this off to obtain a"
            " more predictable program flow.\n"
            "Note, you may no longer be able to cancel image acquisition"
            " via the normal means when this option is set to false.")
     );
}

void
require_(input::ptr iptr)
{
  if (iptr) return;

  BOOST_THROW_EXCEPTION (invalid_argument ("no image data source"));
}

void
require_(output::ptr optr)
{
  if (optr) return;

  BOOST_THROW_EXCEPTION (invalid_argument ("no output destination"));
}

}       // namespace

class pump::impl
{
public:
  impl (input::ptr iptr);
  ~impl ();

  void start (input::ptr iptr, output::ptr optr, toggle);
  void start (output::ptr optr, toggle);

  void cancel ();

  streamsize acquire_and_process (input::ptr iptr, output::ptr optr);

  streamsize acquire_data (input::ptr iptr);
  streamsize process_data (output::ptr optr);

  streamsize acquire_image (input::ptr iptr);
  bucket::ptr process_image (output::ptr optr);

  bucket::ptr make_bucket (streamsize size);

  bucket::ptr pop ();
  void push (bucket::ptr bp);

  void mark (traits::int_type c, const context& ctx);

  input::ptr  iptr_;

  //! \todo Replace with query on iptr_?
  sig_atomic_t is_cancelling_;
  sig_atomic_t is_pumping_;

  thread *acquire_;
  thread *process_;

  std::deque< bucket::ptr >::size_type have_bucket_;

  std::deque< bucket::ptr > brigade_;
  mutex brigade_mutex_;
  condition_variable not_empty_;

  notify_signal_type signal_notify_;
  cancel_signal_type signal_cancel_;

private:
  impl (const impl&);
  impl& operator= (const impl&);
};

pump::impl::impl (input::ptr iptr)
  : iptr_(iptr)
  , is_cancelling_(false)
  , is_pumping_(false)
  , acquire_(nullptr)
  , process_(nullptr)
  , have_bucket_(0)
{
  require_(iptr);
}

pump::impl::~impl ()
{
  if (acquire_)
    {
      acquire_->join ();
    }
  delete acquire_;

  if (process_)
    {
      process_->join ();
    }
  delete process_;
}

void
pump::impl::start (input::ptr iptr, output::ptr optr,
                   toggle acquire_asynchronously)
{
  require_(iptr);
  require_(optr);

  if (!is_cancelling_ && is_pumping_)
    {
      log::error ("still acquiring, cancel first");
      return;
    }

  if (is_cancelling_)
    {
      log::brief ("waiting for cancellation to complete");
      if (acquire_) acquire_->join ();
      is_cancelling_ = false;
    }
  else
    {
      if (acquire_) acquire_->detach ();
    }

  if (process_) process_->join ();

  delete acquire_; acquire_ = nullptr;
  delete process_; process_ = nullptr;
  brigade_.clear ();
  have_bucket_ = 0;

  iptr_ = iptr;

  if (!acquire_asynchronously)
    {
      log::trace ("acquiring image data synchronously");
      acquire_and_process (iptr, optr);
      return;
    }

  // Note that starting order of threads is undefined.

  acquire_ = new thread (&impl::acquire_data, this, iptr);
  process_ = new thread (&impl::process_data, this, optr);
}

void
pump::impl::start (output::ptr optr, toggle acquire_asynchronously)
{
  start (iptr_, optr, acquire_asynchronously);
}

void
pump::impl::cancel ()
{
  if (!iptr_) return;

  iptr_->cancel ();
  is_cancelling_ = true;
}

streamsize
pump::impl::acquire_and_process (input::ptr iptr, output::ptr optr)
{
  streamsize rv = traits::eof ();
  try
    {
      is_pumping_ = true;
      rv = *iptr | *optr;
    }
  catch (const std::exception& e)
    {
      optr->mark (traits::eof (), context ());
      signal_notify_(log::ALERT, e.what ());
    }
  catch (...)
    {
      optr->mark (traits::eof (), context ());
      signal_notify_(log::ALERT,
                     "unknown exception during acquisition and processing");
    }
  is_pumping_ = false;
  if (traits::eof () == rv) signal_cancel_();
  return rv;
}

streamsize                      // read part of operator|
pump::impl::acquire_data (input::ptr iptr)
{
  try
    {
      is_pumping_ = true;
      streamsize rv = iptr->marker ();
      if (traits::bos () != rv)
        {
          mark (traits::eof (), context ());
          is_pumping_ = false;
          signal_cancel_();
          return rv;
        }

      mark (traits::bos (), iptr->get_context ());
      while (   traits::eos () != rv
             && traits::eof () != rv)
        {
          rv = acquire_image (iptr);
        }
      mark (rv, iptr->get_context ());
      is_pumping_ = false;
      if (traits::eof () == rv) signal_cancel_();
      return rv;
    }
  catch (const std::exception& e)
    {
      mark (traits::eof (), context ());
      signal_notify_(log::ALERT, e.what ());
    }
  catch (...)
    {
      mark (traits::eof (), context ());
      signal_notify_(log::ALERT, "unknown exception during acquisition");
    }
  is_pumping_ = false;
  signal_cancel_();
  return traits::eof ();
}

streamsize                      // write part of operator|
pump::impl::process_data (output::ptr optr)
{
  try
    {
      bucket::ptr bp = pop ();
      if (traits::bos () != bp->mark_)
        {
          optr->mark (traits::eof (), context ());
          return bp->mark_;
        }

      optr->mark (traits::bos (), bp->ctx_);
      while (   traits::eos () != bp->mark_
             && traits::eof () != bp->mark_)
        {
          bp = process_image (optr);
        }
      optr->mark (bp->mark_, bp->ctx_);
      return bp->mark_;
    }
  catch (const std::exception& e)
    {
      optr->mark (traits::eof (), context ());
      signal_notify_(log::ALERT, e.what ());
    }
  catch (...)
    {
      optr->mark (traits::eof (), context ());
      signal_notify_(log::ALERT, "unknown exception during processing");
    }
  return traits::eof ();
}

streamsize                      // read part of operator>>
pump::impl::acquire_image (input::ptr iptr)
{
  streamsize n = iptr->marker ();
  if (traits::boi () != n) return n;

  const streamsize buffer_size = iptr->buffer_size ();
  bucket::ptr bp;

  mark (traits::boi (), iptr->get_context ());

  bp = make_bucket (buffer_size);
  n = iptr->read (bp->data_, bp->size_);
  while (   traits::eoi () != n
         && traits::eof () != n)
    {
      bp->size_ = n;
      push (bp);
      bp = make_bucket (buffer_size);
      n = iptr->read (bp->data_, bp->size_);
    }
  mark (n, iptr->get_context ());
  if (traits::eof () == n) signal_cancel_();
  return n;
}

bucket::ptr            // write part of operator>>
pump::impl::process_image (output::ptr optr)
{
  bucket::ptr bp = pop ();
  if (traits::boi () != bp->mark_) return bp;

  optr->mark (traits::boi (), bp->ctx_);
  bp = pop ();
  while (   traits::eoi () != bp->mark_
         && traits::eof () != bp->mark_)
    {
      const octet *p = bp->data_;
      streamsize m;

      while (0 < bp->size_) {
        m          = optr->write (p, bp->size_);
        p         += m;
        bp->size_ -= m;
      }
      bp = pop ();
    }
  optr->mark (bp->mark_, bp->ctx_);
  return bp;
}

bucket::ptr
pump::impl::make_bucket (streamsize size)
{
  bucket::ptr rv;

  while (!rv)
    {
      try
        {
          rv = make_shared< bucket > (size);
        }
      catch (const std::bad_alloc&)
        {
          bool retry_alloc;
          {
            lock_guard< mutex > lock (brigade_mutex_);

            retry_alloc = have_bucket_;
          }
          if (retry_alloc)
            {
              this_thread::yield ();
            }
          else
            {
              throw;
            }
        }
    }
  return rv;
}

bucket::ptr
pump::impl::pop ()
{
  bucket::ptr bp;

  {
    unique_lock< mutex > lock (brigade_mutex_);

    while (!have_bucket_)
      not_empty_.wait (lock);

    bp = brigade_.front ();
    brigade_.pop_front ();
    --have_bucket_;
  }

  return bp;
}

void
pump::impl::push (bucket::ptr bp)
{
  {
    lock_guard< mutex > lock (brigade_mutex_);
    brigade_.push_back (bp);
    ++have_bucket_;
  }
  not_empty_.notify_one ();
}

void
pump::impl::mark (traits::int_type c, const context& ctx)
{
  push (make_shared< bucket > (ctx, c));
}

pump::pump (idevice::ptr idev)
  : pimpl_(new impl (idev))
{
  init_(option_);
}

pump::~pump ()
{
  delete pimpl_;
}

void
pump::start (odevice::ptr odev)
{
  pimpl_->start (odev, value ((*option_)[ASYNC]));
}

void
pump::start (stream::ptr str)
{
  pimpl_->start (str, value ((*option_)[ASYNC]));
}

void
pump::cancel ()
{
  pimpl_->cancel ();
}

connection
pump::connect (const notify_signal_type::slot_type& slot) const
{
  return pimpl_->signal_notify_.connect (slot);
}

connection
pump::connect_cancel (const cancel_signal_type::slot_type& slot) const
{
  return pimpl_->signal_cancel_.connect (slot);
}

}       // namespace utsushi
