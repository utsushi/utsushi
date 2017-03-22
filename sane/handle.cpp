//  handle.cpp -- for a SANE scanner object
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2015  Olaf Meeuwissen
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

#include <cstring>
#include <deque>
#include <stdexcept>
#include <typeinfo>

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/condition-variable.hpp>
#include <utsushi/functional.hpp>
#include <utsushi/memory.hpp>
#include <utsushi/mutex.hpp>
#include <utsushi/range.hpp>
#include <utsushi/store.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/i18n.hpp>

#include "../filters/autocrop.hpp"
#include "../filters/deskew.hpp"
#include "../filters/image-skip.hpp"
#if HAVE_LIBJPEG
#include "../filters/jpeg.hpp"
#endif
#include "../filters/magick.hpp"
#include "../filters/padding.hpp"
#include "../filters/pnm.hpp"
#include "../filters/reorient.hpp"

#include "handle.hpp"
#include "log.hpp"
#include "value.hpp"

using std::exception;
using std::runtime_error;
using utsushi::_flt_::bottom_padder;
using utsushi::_flt_::image_skip;
using utsushi::_flt_::magick;
using utsushi::_flt_::deskew;
using utsushi::_flt_::autocrop;
using utsushi::_flt_::pnm;

namespace sane {

//! Keep backend options separate from frontend options
static const utsushi::key option_prefix ("device");
static const std::string magick_prefix ("magick");
static const std::string filter_prefix ("filter");
static const std::string action_prefix ("action");

//! Well-known SANE option names
/*! Although the well-known option names are defined in the SANE API
 *  specification there are no officially sanctioned symbols that can
 *  be used in one's code.  The add-on \c sane/saneopts.h header file
 *  contains a number of de facto standard symbols.  We will refrain
 *  from relying on these symbols and define our own based upon the
 *  text of the SANE API specification.
 *
 *  \note The SANE 2 draft standard adds a good number of well-known
 *        option names.  The \c sane/saneopts.h header file contains
 *        symbols for some of these well-known option names as well.
 *        For several well-known option names \c sane\saneopts.h uses
 *        strings that do \e not follow the draft standard.
 */
namespace name
{
  // The SANE API blessed well-known option names

  static const std::string num_options ("");
  static const std::string resolution ("resolution");
  static const std::string preview ("preview");
  static const std::string tl_x ("tl-x");
  static const std::string tl_y ("tl-y");
  static const std::string br_x ("br-x");
  static const std::string br_y ("br-y");

  // Selected extensions from the SANE 2 draft

  static const std::string x_resolution ("x-resolution");
  static const std::string y_resolution ("y-resolution");
  static const std::string source ("source");
  static const std::string mode ("mode");

  // Convenience queries for options with similar behaviour

  static bool is_resolution (const std::string& name)
  {
    return (     resolution == name
            || x_resolution == name
            || y_resolution == name);
  }

  static bool is_scan_area (const std::string& name)
  {
    return (   tl_x == name
            || tl_y == name
            || br_x == name
            || br_y == name);
  }

  static bool is_well_known (const std::string& name)
  {
    return (   name == resolution
            || name == preview
            || is_scan_area (name));
  }
}       // namespace name

//! Translate between Utsushi keys and SANE option names
/*! The mappings allow us to completely decouple Utsushi keys from the
 *  SANE option names.
 *
 *  Utsushi keys come first, SANE option names second.
 */
namespace xlate
{
  typedef std::pair< utsushi::key, std::string > mapping;

  static const mapping resolution   ("resolution"  , name::resolution);
  static const mapping resolution_x ("resolution-x", name::x_resolution);
  static const mapping resolution_y ("resolution-y", name::y_resolution);

  static const mapping preview ("preview", name::preview);

  static const mapping tl_x ("tl-x", name::tl_x);
  static const mapping tl_y ("tl-y", name::tl_y);
  static const mapping br_x ("br-x", name::br_x);
  static const mapping br_y ("br-y", name::br_y);

  static const mapping doc_source ("doc-source", name::source);
  static const mapping image_type ("image-type", name::mode);

  static const mapping sw_resolution   ("sw-resolution"  , name::resolution);
  static const mapping sw_resolution_x ("sw-resolution-x", name::x_resolution);
  static const mapping sw_resolution_y ("sw-resolution-y", name::y_resolution);
  static const mapping sw_resolution_bind ("sw-resolution-bind",
                                           "resolution-bind");
  static const mapping resolution_bind ("resolution-bind", "resolution-bind");
}       // namespace xlate

namespace unit
{
  static const utsushi::quantity::non_integer_type mm_per_inch = 25.4;
};

using std::logic_error;
using namespace utsushi;

class bucket                    // fixme: copies code in lib/pump.cpp
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

class iocache
  : public idevice
  , public odevice
{
public:
  typedef shared_ptr< iocache > ptr;

  iocache ()
    : have_bucket_(0)
  {}

  streamsize write (const octet *data, streamsize n)
  {
    if (!data || 0 >= n) return 0;

    bucket::ptr bp (make_bucket (n));
    traits::copy (bp->data_, data, n);
    {
      lock_guard< mutex > lock (mutex_);
      brigade_.push_back (bp);
      ++have_bucket_;
    }
    not_empty_.notify_one ();

    return n;
  }

  void mark (traits::int_type c, const context& ctx)
  {
    bucket::ptr bp (make_bucket (ctx, c));
    {
      lock_guard< mutex > lock (mutex_);
      brigade_.push_back (bp);
      ++have_bucket_;

      odevice::last_marker_ = bp->mark_;
      odevice::ctx_         = bp->ctx_;
    }
    not_empty_.notify_one ();
  }

protected:

  streamsize sgetn (octet *data, streamsize n)
  {
    BOOST_ASSERT (traits::boi () == idevice::last_marker_);

    bucket::ptr bp (front ());

    if (traits::is_marker (bp->mark_))
      {
        BOOST_ASSERT (   traits::eoi () == bp->mark_
                      || traits::eof () == bp->mark_);
        pop_front ();

        return (traits::eoi () == bp->mark_ ? 0 : -1);
      }

    if (!data || 0 >= n)
      return traits::not_marker (0);

    streamsize rv = std::min (n, bp->size_);
    traits::copy (data, bp->data_, rv);
    if (rv == bp->size_)
      {
        pop_front ();
      }
    else
      {
        traits::move (bp->data_, bp->data_ + rv, bp->size_ - rv);
        bp->size_ -= rv;
      }
    return rv;
  }

  bool is_consecutive () const
  {
    BOOST_ASSERT (traits::eoi () == idevice::last_marker_);

    bucket::ptr bp (front ());

    BOOST_ASSERT (   traits::boi () == bp->mark_
                  || traits::eos () == bp->mark_
                  || traits::eof () == bp->mark_);

    if (traits::boi () != bp->mark_)
      const_cast< iocache * > (this)->pop_front ();

    return (traits::boi () == bp->mark_);
  }

  bool obtain_media ()
  {
    BOOST_ASSERT (   traits::eoi () == idevice::last_marker_
                  || traits::eos () == idevice::last_marker_
                  || traits::eof () == idevice::last_marker_);

    bucket::ptr bp (front ());

    if (traits::eoi () == idevice::last_marker_)
      {
        BOOST_ASSERT (   traits::eos () == bp->mark_
                      || traits::eof () == bp->mark_
                      || traits::boi () == bp->mark_);

        if (traits::boi () != bp->mark_) pop_front ();

        return (traits::boi () == bp->mark_);
      }
    else
      {
        BOOST_ASSERT (   traits::eos () == bp->mark_
                      || traits::eof () == bp->mark_
                      || traits::bos () == bp->mark_);

        pop_front ();

        return (traits::bos () == bp->mark_);
      }
  }

  bool set_up_image ()
  {
    BOOST_ASSERT (   traits::eoi () == idevice::last_marker_
                  || traits::bos () == idevice::last_marker_);

    bucket::ptr bp (front ());

    BOOST_ASSERT (   traits::boi () == bp->mark_
                  || traits::eos () == bp->mark_
                  || traits::eof () == bp->mark_);

    pop_front ();

    return (traits::boi () == bp->mark_);
  }

  bool set_up_sequence ()
  {
    BOOST_ASSERT (   traits::eos () == idevice::last_marker_
                  || traits::eof () == idevice::last_marker_);

    bucket::ptr bp (front ());

    BOOST_ASSERT (   traits::bos () == bp->mark_
                  || traits::eof () == bp->mark_);

    if (traits::bos () != bp->mark_) pop_front ();

    return (traits::bos () == bp->mark_);
  }

  bucket::ptr front () const
  {
    {
      unique_lock< mutex > lock (mutex_);

      while (!have_bucket_)
        not_empty_.wait (lock);
    }
    return brigade_.front ();
  }

  void pop_front ()
  {
    bucket::ptr bp (front ());
    {
      lock_guard< mutex > lock (mutex_);
      brigade_.pop_front ();
      --have_bucket_;
    }

    if (traits::is_marker (bp->mark_))
      {
        idevice::last_marker_ = bp->mark_;
        idevice::ctx_         = bp->ctx_;
      }

    if (traits::eof () == bp->mark_ && oops_)
      {
        runtime_error e = *oops_;
        oops_ = boost::none;
        BOOST_THROW_EXCEPTION (e);
      }
  }

  bucket::ptr make_bucket (streamsize size)
  {
    bucket::ptr bp;

    while (!bp)
      {
        try
          {
            bp = make_shared< bucket > (size);
          }
        catch (const std::bad_alloc&)
          {
            bool retry_alloc;
            {
              lock_guard< mutex > lock (mutex_);

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
    return bp;
  }

  bucket::ptr make_bucket (const context& ctx, streamsize marker)
  {
    bucket::ptr bp;

    while (!bp)
      {
        try
          {
            bp = make_shared< bucket > (ctx, marker);
          }
        catch (const std::bad_alloc&)
          {
            bool retry_alloc;
            {
              lock_guard< mutex > lock (mutex_);

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
    return bp;
  }

  std::deque< bucket::ptr >::size_type have_bucket_;

  std::deque< bucket::ptr > brigade_;
  mutable mutex mutex_;
  mutable condition_variable not_empty_;

  boost::optional< runtime_error > oops_;

public:
  void on_notify (utsushi::log::priority level, const std::string& message)
  {
    utsushi::log::message (level, utsushi::log::SANE_BACKEND, message);

    switch (level)
      {
      case utsushi::log::FATAL: break;
      case utsushi::log::ALERT: break;
      case utsushi::log::ERROR: break;
      default:
        return;                 // not an error -> do not terminate
      }

    // The scan sequence has been terminated.  Mark this on our
    // odevice end so that subsequent access on the idevice end
    // will be able to rethrow the exception.

    oops_ = runtime_error (message);
    mark (traits::eof (), odevice::ctx_);
  }
  void on_cancel ()
  {
    oops_ = runtime_error ("Device initiated cancellation.");
    mark (traits::eof (), odevice::ctx_);
  }
};

void
on_notify (iocache::ptr p, utsushi::log::priority level,
           const std::string& message)
{
  if (p) p->on_notify (level, message);
}

handle::handle(const scanner::info& info)
  : name_(info.name () + " (" + info.udi () + ")")
  , idev_(scanner::create (info))
  , pump_(make_shared< pump > (idev_))
  , last_marker_(traits::eos ())
  , work_in_progress_(false)
  , cancel_requested_(work_in_progress_)
  , emulating_automatic_scan_area_(false)
  , do_automatic_scan_area_(false)
{
  opt_.add_options ()
    (name::num_options, quantity (0),
     attributes ())
    ;

  if (HAVE_MAGICK_PP
      && idev_->options ()->count ("lo-threshold")
      && idev_->options ()->count ("hi-threshold"))
    {
      if (idev_->options ()->count ("scan-area"))
        {
          using utsushi::value;

          constraint::ptr c ((*idev_->options ())["scan-area"]
                             .constraint ());
          if (value ("Auto Detect") != (*c) (value ("Auto Detect")))
            {
              dynamic_pointer_cast< store >
                (c)->alternative ("Auto Detect");

              // All SANE options are exposed so we cannot really
              // stick this in an option as we do in the GUI.
              emulating_automatic_scan_area_ = true;
              do_automatic_scan_area_ = false;
            }
        }

      // Playing tricky games with the option namespacing here to get
      // software emulated options listed with a reasonable SANE name.
      // An utsushi::key normally uses a '/' to separate namespaces
      // but SANE does not allow those.  We already map the '/' to a
      // '-', so using a '-' here will make it appear to be in the
      // filter_prefix namespace without the need to be a member of
      // that namespace.

      if (!idev_->options ()->count ("deskew"))
        {
          opt_.add_options ()
            (filter_prefix + "-deskew", toggle (),
             attributes (tag::enhancement)(level::standard),
             SEC_N_("Deskew"));
        }
    }

  filter::ptr reorient;
  if (HAVE_MAGICK)
    {
      filter::ptr magick (make_shared< _flt_::magick > ());
      if (magick->options ()->count ("auto-orient"))
        {
          reorient = make_shared< _flt_::reorient > ();
          option rotate = (*reorient->options ())["rotate"];
          // More option namespace trickery, see comment above
          opt_.add_options ()
            (filter_prefix + "-rotate", rotate.constraint(),
             attributes (tag::enhancement)(level::standard),
             rotate.name (),
             rotate.text ());
        }
    }

  opt_.add_option_map ()
    (option_prefix, idev_->options ())
    (action_prefix, idev_->actions ())
    ;

  image_skip flt;
  opt_.add_option_map ()
    (filter_prefix, flt.options ())
    ;

  filter::ptr magick;
  if (HAVE_MAGICK)
    {
      magick = make_shared< utsushi::_flt_::magick > ();
    }
  if (magick)
    {
      opt_.add_option_map ()
        (magick_prefix, magick->options ());

      opt_.add_options ()
        (magick_prefix + "-image-type", (from< utsushi::store > ()
                                         -> alternative (SEC_N_("Monochrome"))
                                         -> alternative (SEC_N_("Grayscale"))
                                         -> default_value (SEC_N_("Color"))),
         attributes (tag::general)(level::standard),
         SEC_N_("Image Type"))
        ;
    }

  sod_.reserve (opt_.size ());

  SANE_Option_Descriptor sod;
  memset (&sod, 0x0, sizeof (sod));

  // SANE API requires this option to be at index 0
  option num_opts (opt_[name::num_options]);
  add_option (num_opts);

  // To accommodate SANE frontends, we group options by tag priority.
  // Groups are created based on tag information.

  std::set< key > seen;
  seen.insert (name::num_options);

  std::set<std::string> option_blacklist;
  if (magick)
    {
      option_blacklist.insert(option_prefix / "image-type");
      option_blacklist.insert(option_prefix / "threshold");
    }

  tags::const_iterator it (tags::begin ());
  for (; tags::end () != it; ++it)
    {
      if (tag::application == *it) continue;

      bool group_added = false;
      option::map::iterator om_it (opt_.begin ());
      for (; opt_.end () != om_it; ++om_it)
        {
          // FIXME skip software resolutions for a more intuitive UI
          //       We make up for this in update_options().
          using namespace xlate;
          /**/ if (option_prefix / sw_resolution.first   == om_it->key ())
            seen.insert (om_it->key ());
          else if (option_prefix / sw_resolution_x.first == om_it->key ())
            seen.insert (om_it->key ());
          else if (option_prefix / sw_resolution_y.first == om_it->key ())
            seen.insert (om_it->key ());
          else if (option_prefix / sw_resolution_bind.first == om_it->key ())
            seen.insert (om_it->key ());
          else if (0 == om_it->key ().find (magick_prefix))
            {
              if (!(   key (magick_prefix) / "threshold"   == om_it->key ()
                    || key (magick_prefix) / "brightness"  == om_it->key ()
                    || key (magick_prefix) / "contrast"    == om_it->key ()
                    || key (magick_prefix + "-image-type") == om_it->key ()))
                seen.insert (om_it->key ());
            }

          if (!seen.count (om_it->key ())
              && om_it->tags ().count (*it))
            {
              if (!option_blacklist.count (om_it->key ()))
                {
                  if (!group_added)
                    {
                      add_group (option_prefix / *it,
                                 it->name (), it->text ());
                      group_added = true;
                    }
                  add_option (*om_it);
                }
              seen.insert (om_it->key ());
            }
        }

      if (tag::geometry == *it)
        {
          // It looks like scanimage already rearranges the top-left
          // and bottom-right options for us in something that users
          // might find easier to use.  We have nothing to do here.
        }
    }

  //  Pick up options and actions without any tags

  bool group_added (false);
  option::map::iterator om_it (opt_.begin ());
  for (; opt_.end () != om_it; ++om_it)
    {
      if (!seen.count (om_it->key ()))
        {
          if (!group_added)
            {
              add_group (option_prefix / "~", "Other");
              group_added = true;
            }
          add_option (*om_it);
          seen.insert (om_it->key ());
        }
    }

  opt_[name::num_options] = quantity::integer_type (sod_.size ());

  // As per SANE API, sect. 4.4 "Code Flow", the number of options is
  // fixed for a given handle.  Don't let any frontend modify it.
  sod_[0].cap &= ~(SANE_CAP_HARD_SELECT | SANE_CAP_SOFT_SELECT);

  // FIXME hack to get the other source only options desensitized
  //       Here's praying this doesn't trigger constraint::violations
  option source = opt_[option_prefix / "doc-source"];
  if (!source.constraint ()->is_singular ())
    {
      if (dynamic_cast< store * > (source.constraint ().get ()))
        {
          store s = source.constraint< store > ();

          value current = source;
          for (store::const_iterator it = s.begin (); it != s.end (); ++it)
            source = *it;

          source = current;
        }
    }
  update_options (nullptr);
}

std::string
handle::name () const
{
  return name_;
}

SANE_Int
handle::size () const
{
  return sod_.size ();
}

const SANE_Option_Descriptor *
handle::descriptor (SANE_Int index) const
{
  return &sod_[index];
}

bool
handle::is_active (SANE_Int index) const
{
  return SANE_OPTION_IS_ACTIVE (sod_[index].cap);
}

bool
handle::is_button (SANE_Int index) const
{
  return SANE_TYPE_BUTTON == sod_[index].type;
}

bool
handle::is_group (SANE_Int index) const
{
  return SANE_TYPE_GROUP == sod_[index].type;
}

bool
handle::is_settable (SANE_Int index) const
{
  return SANE_OPTION_IS_SETTABLE (sod_[index].cap);
}

bool
handle::is_automatic (SANE_Int index) const
{
  return (is_settable (index)
          && (SANE_CAP_AUTOMATIC & sod_[index].cap));
}

bool
handle::is_scanning () const
{
  return (work_in_progress_
          && (traits::boi () == last_marker_));
}

SANE_Status
handle::get (SANE_Int index, void *value) const
{
  SANE_Status status = SANE_STATUS_GOOD;

  utsushi::key k (sod_[index].orig_key);
  sane::value  v (const_cast< option::map& > (opt_)[k]);

  // FIXME remove unit conversion kludge
  if (name::is_scan_area (sod_[index].sane_key))
    {
      v *= unit::mm_per_inch;
    }

  if (k == option_prefix / "scan-area"
      && emulating_automatic_scan_area_
      && do_automatic_scan_area_)
    {
      v = utsushi::value ("Auto Detect");
    }

  v >> value;

  return status;
}

SANE_Status
handle::set (SANE_Int index, void *value, SANE_Word *info)
{
  utsushi::key k (sod_[index].orig_key);

  sane::value v (opt_[k]);
  v << value;
  // FIXME remove unit conversion kludge
  if (name::is_scan_area (sod_[index].sane_key))
    v /= unit::mm_per_inch;

  if (k == option_prefix / "scan-area"
      && emulating_automatic_scan_area_)
    {
      const utsushi::value automatic ("Auto Detect");
      do_automatic_scan_area_ = (automatic == utsushi::value (v));
      if (do_automatic_scan_area_)
        v = utsushi::value ("Maximum");
    }

  if (SANE_TYPE_BUTTON != option_descriptor (opt_[k]).type)
    if (opt_[k] == v) return SANE_STATUS_GOOD;

  if (is_scanning ()) return SANE_STATUS_DEVICE_BUSY;

  end_scan_sequence ();

  SANE_Status status = SANE_STATUS_GOOD;

  if (SANE_TYPE_BUTTON != option_descriptor (opt_[k]).type)
    {
      try
        {
          value::map vm;

          if (k == option_prefix / "scan-area"
              && emulating_automatic_scan_area_)
            {
              vm[k] = v;
              if (opt_.count (option_prefix / "auto-kludge"))
                vm[option_prefix / "auto-kludge"]
                  = toggle (do_automatic_scan_area_);
            }

          if (k == key (magick_prefix + "-image-type")
              && opt_.count (option_prefix / "image-type"))
            {
              string type = v;
              if (type == "Monochrome")
                type = "Grayscale";
              vm[k] = v;
              try {
                vm[option_prefix / "image-type"] = type;
              }
              catch (const std::out_of_range&){}
            }

          if (vm.empty ())
            opt_[k] = v;
          else
            opt_.assign (vm);

          if (opt_.count (option_prefix / "long-paper-mode")
              && opt_.count (filter_prefix + "-deskew"))
            {
              using utsushi::value;

              toggle t1 = value (opt_[option_prefix / "long-paper-mode"]);
              opt_[filter_prefix + "-deskew"].active (!t1);
              toggle t2 = value (opt_[filter_prefix + "-deskew"]);
              opt_[option_prefix / "long-paper-mode"].active (!t2);
            }

          update_options (info);

          if (info)
            {
              if (v != opt_[k])
                *info |= SANE_INFO_INEXACT;

              *info |= SANE_INFO_RELOAD_PARAMS;
            }
        }
      catch (const constraint::violation&)
        {
          status = SANE_STATUS_INVAL;
        }
    }
  else
    {
      try
        {
          std::string basename (k);
          std::string::size_type pos = basename.find_last_of ("/");

          if (std::string::npos != pos)
            k = basename.substr (pos + 1);

          result_code rc = (*idev_->actions ())[k].run ();
          if (rc)
            {
              log::error (rc.message ());
              status = SANE_STATUS_CANCELLED;
            }
        }
      catch (const runtime_error& e)
        {
          log::alert (e.what ());
          status = SANE_STATUS_CANCELLED;
        }
      catch (...)
        {
          status = SANE_STATUS_UNSUPPORTED;
        }
    }

  return status;
}

SANE_Status
handle::set (SANE_Int index, SANE_Word *info)
{
  if (is_scanning ()) return SANE_STATUS_DEVICE_BUSY;

  end_scan_sequence ();

  return SANE_STATUS_UNSUPPORTED;
}

context
handle::get_context () const
{
  if (idevice::ptr iptr = iptr_.lock ())
    return iptr->get_context ();

  return idev_->get_context ();
}

streamsize
handle::start ()
{
  // Of all silly things!  Frontends don't always continue reading
  // until they receive a SANE_Status other than SANE_STATUS_GOOD.
  // That leaves us in a state where we first have to clean up any
  // work_in_progress_ before we can get started, really.  To make
  // things even more entertaining, frontends may decide to cancel
  // while we are busy cleaning up.

  if (work_in_progress_)
    {
      const streamsize max_length = 1024;
      octet buffer[max_length];

      streamsize rv;

      do
        {
          rv = this->read (buffer, max_length);
        }
      while (!traits::is_marker (rv));

      BOOST_ASSERT (!work_in_progress_);

      if (traits::eof () == rv)
        return rv;
    }

  BOOST_ASSERT (!work_in_progress_);
  BOOST_ASSERT (!cancel_requested_);

  BOOST_ASSERT (   traits::eoi () == last_marker_
                || traits::eos () == last_marker_
                || traits::eof () == last_marker_);

  // State transitions may be time consuming so there will be some
  // work_in_progress_, at least until we're mostly done.

  work_in_progress_ = true;

  streamsize lm = last_marker_;
  streamsize rv = marker ();    // changes value of last_marker_

  if (traits::boi () != rv)
    {
      // We try to work our way through a smallish maze of state
      // transitions to arrive at traits::boi().  Note that this
      // should not allow the traits::eof() marker to occur more
      // than once in the sequence, starting from last_marker_.

      /**/ if (traits::eoi () == lm)
        {
          if (traits::eos () == rv) rv = marker ();
          if (traits::eof () == rv) rv = marker ();
          if (traits::bos () == rv) rv = marker ();
        }
      else if (traits::eos () == lm)
        {
          if (traits::eof () == rv) rv = marker ();
          if (traits::bos () == rv) rv = marker ();
        }
      else if (traits::eof () == lm)
        {
          if (traits::bos () == rv) rv = marker ();
        }
    }

  if (traits::is_marker (rv))
    {
      if (   traits::eoi () == rv
          || traits::eos () == rv
          || traits::eof () == rv)
        {
          work_in_progress_ = false;
          cancel_requested_ = work_in_progress_;
        }

      if (traits::boi () != last_marker_) cache_.reset ();
    }

  BOOST_ASSERT (   traits::boi () == last_marker_
                || traits::eos () == last_marker_
                || traits::eof () == last_marker_);

  return rv;
}

streamsize
handle::read (octet *buffer, streamsize length)
{
  // Not all SANE frontends take a hint when we told them there is no
  // more image data or the acquisition has been cancelled (even when
  // said SANE frontend requested cancellation itself!).  Cluebat the
  // frontend until it takes note.

  if (!is_scanning ()) return last_marker_;

  // Now, back to our regular programming.

  BOOST_ASSERT (work_in_progress_);
  BOOST_ASSERT (traits::boi () == last_marker_);

  streamsize rv;

  try
    {
      if (idevice::ptr iptr = iptr_.lock ())
        {
          rv = iptr->read (buffer, length);
        }
      else
        {
          rv = idev_->read (buffer, length);
        }
    }
  catch (const exception& e)
    {
      work_in_progress_ = false;
      cancel_requested_ = work_in_progress_;

      last_marker_ = traits::eof ();
      cache_.reset ();

      throw;
    }

  if (traits::is_marker (rv))
    {
      if (   traits::eoi () == rv
          || traits::eof () == rv)
        {
          work_in_progress_ = false;
          cancel_requested_ = work_in_progress_;
        }

      last_marker_ = rv;
      if (traits::eof () == last_marker_) cache_.reset ();
    }

  BOOST_ASSERT (  !traits::is_marker (rv)
                || traits::eoi () == last_marker_
                || traits::eof () == last_marker_);

  return rv;
}

void
handle::cancel ()
{
  if ((cancel_requested_ = work_in_progress_))
    {
      end_scan_sequence ();
    }
}

void
handle::end_scan_sequence ()
{
  pump_->cancel ();
}

streamsize
handle::marker ()
{
  static bool revert_overscan = false;

  /**/ if (  !cache_
           || traits::eos () == last_marker_
           || traits::eof () == last_marker_)
    {
      pump_->cancel ();         // prevent deadlock

      stream::ptr str  = make_shared< stream > ();

      const std::string xfer_raw = "image/x-raster";
      const std::string xfer_jpg = "image/jpeg";
      std::string xfer_fmt = idev_->get_context ().content_type ();

      /**/ if (xfer_raw == xfer_fmt) {}
      else if (HAVE_LIBJPEG && xfer_jpg == xfer_fmt) {}
      else                      // bail as soon as possible
        {
          log::alert
            ("unsupported transfer format: '%1%'") % xfer_fmt;

          last_marker_ = traits::eof ();
          return last_marker_;
        }

      bool bilevel = (opt_[option_prefix / "image-type"] == "Monochrome");
      if (HAVE_MAGICK
          && opt_.count (magick_prefix + "-image-type"))
        {
          bilevel = (opt_[magick_prefix + "-image-type"] == "Monochrome");
          if (bilevel)
            {
              opt_[option_prefix / "image-type"] = string ("Grayscale");
            }
          else
            {
              opt_[option_prefix / "image-type"] = value (opt_[magick_prefix + "-image-type"]);
            }
        }

      toggle force_extent = true;
      quantity width  = -1.0;
      quantity height = -1.0;
      try
        {
          force_extent = value (opt_[option_prefix / "force-extent"]);
          width   = value (opt_[option_prefix / "br-x"]);
          width  -= value (opt_[option_prefix / "tl-x"]);
          height  = value (opt_[option_prefix / "br-y"]);
          height -= value (opt_[option_prefix / "tl-y"]);
        }
      catch (const std::out_of_range&)
        {
          force_extent = false;
          width  = -1.0;
          height = -1.0;
        }
      if (force_extent) force_extent = (width > 0 || height > 0);

      filter::ptr autocrop;
      if (HAVE_MAGICK_PP
          && emulating_automatic_scan_area_
          && do_automatic_scan_area_)
        {
          if (opt_.count (option_prefix / "overscan"))
            {
              toggle t = value (opt_[option_prefix / "overscan"]);
              if (!t)
                {
                  opt_[option_prefix / "overscan"] = toggle (true);
                  revert_overscan = true;
                }
            }
          autocrop = make_shared< _flt_::autocrop > ();
        }

      if (autocrop)
        {
          (*autocrop->options ())["lo-threshold"] = value (opt_[option_prefix / "lo-threshold"]);
          (*autocrop->options ())["hi-threshold"] = value (opt_[option_prefix / "hi-threshold"]);
        }

      filter::ptr deskew;
      if (HAVE_MAGICK_PP
          && !autocrop && opt_.count (filter_prefix + "-deskew"))
        {
          toggle t = value ((opt_)[filter_prefix + "-deskew"]);

          if (opt_.count (option_prefix / "long-paper-mode")
              && (value (toggle (true))
                  == opt_[option_prefix / "long-paper-mode"]))
            t = false;

          if (t)
            deskew = make_shared< _flt_::deskew > ();
        }

      if (deskew)
        {
          (*deskew->options ())["lo-threshold"] = value (opt_[option_prefix / "lo-threshold"]);
          (*deskew->options ())["hi-threshold"] = value (opt_[option_prefix / "hi-threshold"]);
        }

      if (HAVE_MAGICK_PP
          && opt_.count (option_prefix / "long-paper-mode"))
        {
          string s = value (opt_[option_prefix / "doc-source"]);
          toggle t = value (opt_[option_prefix / "long-paper-mode"]);
          if (s == "ADF" && t && opt_.count (option_prefix / "scan-area"))
            {
              t = (opt_[option_prefix / "scan-area"] == value ("Auto Detect")
                   || do_automatic_scan_area_);
              if (t && !autocrop)
                autocrop = make_shared< _flt_::autocrop > ();
              if (t)
                (*autocrop->options ())["trim"] = t;
            }
        }
      if (autocrop) force_extent = false;

      filter::ptr reorient;
      if (HAVE_MAGICK
          && opt_.count (filter_prefix + "-rotate"))
        {
          value angle = value (opt_[filter_prefix + "-rotate"]);
          reorient = make_shared< _flt_::reorient > ();
          (*reorient->options ())["rotate"] = angle;
        }

      toggle resample = false;
      if (opt_.count (option_prefix / "enable-resampling"))
        resample = value (opt_[option_prefix / "enable-resampling"]);

      filter::ptr magick;
      if (HAVE_MAGICK)
        {
          magick = make_shared< _flt_::magick > ();
          if (reorient)
            {
              (*magick->options ())["auto-orient"] = toggle (true);
            }
        }

      if (magick)
        {
          toggle bound = true;
          quantity res_x  = -1.0;
          quantity res_y  = -1.0;

          std::string sw (resample ? "sw-" : "");
          if (opt_.count (option_prefix / (sw + "resolution-x")))
            {
              res_x = value (opt_[option_prefix / (sw + "resolution-x")]);
              res_y = value (opt_[option_prefix / (sw + "resolution-y")]);
            }
          if (opt_.count (option_prefix / (sw + "resolution-bind")))
            bound = value (opt_[option_prefix / (sw + "resolution-bind")]);

          if (bound)
            {
              res_x = value (opt_[option_prefix / (sw + "resolution")]);
              res_y = value (opt_[option_prefix / (sw + "resolution")]);
            }

          (*magick->options ())["resolution-x"] = res_x;
          (*magick->options ())["resolution-y"] = res_y;
          (*magick->options ())["force-extent"] = force_extent;
          (*magick->options ())["width"]  = width;
          (*magick->options ())["height"] = height;

          (*magick->options ())["bilevel"] = toggle (bilevel);

          quantity threshold  = value (opt_[key (magick_prefix) / "threshold"]);
          quantity brightness = value (opt_[key (magick_prefix) / "brightness"]);
          quantity contrast   = value (opt_[key (magick_prefix) / "contrast"]);
          (*magick->options ())["threshold"] = threshold;
          (*magick->options ())["brightness"] = brightness;
          (*magick->options ())["contrast"]   = contrast;

          // keep magick filter's default format to generate image/x-raster
      }

      {
        toggle sw_color_correction = false;
        if (opt_.count (option_prefix / "sw-color-correction"))
        {
          sw_color_correction =
            value (opt_[option_prefix / "sw-color-correction"]);
          for (int i = 1; sw_color_correction && i <= 9; ++i)
            {
              key k ("cct-" + boost::lexical_cast< std::string > (i));
              (*magick->options ())[k] = value (opt_[option_prefix / k]);
            }
        }
      (*magick->options ())["color-correction"] = sw_color_correction;
    }

      toggle skip_blank = !bilevel; // \todo fix filter limitation
      if (magick)
        skip_blank = true;
      quantity skip_thresh = -1.0;
      filter::ptr blank_skip (make_shared< image_skip > ());
      try
        {
          (*blank_skip->options ())["blank-threshold"]
            = value (opt_[key (filter_prefix) / "blank-threshold"]);
          skip_thresh = value ((*blank_skip->options ())["blank-threshold"]);
        }
      catch (const std::out_of_range&)
        {
          skip_blank = false;
          log::error ("Disabling blank skip functionality");
        }
      // Don't even try skipping of completely white images.  We are
      // extremely unlikely to encounter any of those.
      skip_blank = (skip_blank
                    && (quantity (0.) < skip_thresh));

      /**/ if (xfer_raw == xfer_fmt)
        {
          str->push (make_shared< _flt_::padding > ());
        }
#if HAVE_LIBJPEG
      else if (xfer_jpg == xfer_fmt)
        {
          str->push (make_shared< _flt_::jpeg::decompressor > ());
        }
#endif
      else
        {
          log::alert
            ("unsupported transfer format: '%1%'") % xfer_fmt;

          BOOST_THROW_EXCEPTION
            (logic_error
             ((format ("unsupported transfer format: '%1%'") % xfer_fmt)
              .str ()));
        }

      if (skip_blank)  str->push (blank_skip);
      str->push (make_shared< pnm > ());
      if (autocrop)    str->push (autocrop);
      if (deskew)      str->push (deskew);
      if (reorient)    str->push (reorient);
      if (magick)      str->push (magick);

      iocache::ptr cache (make_shared< iocache > ());
      str->push (odevice::ptr (cache));
      cache_ = idevice::ptr (cache);
      iptr_ = cache_;

      pump_->connect (bind (on_notify, cache, _1, _2));
      pump_->connect_cancel (bind (&iocache::on_cancel, cache));
      pump_->start (str);
    }
  else
    {
      if (revert_overscan)
        {
          opt_[option_prefix / "overscan"] = toggle (false);
          revert_overscan = false;
        }
    }

  streamsize rv = traits::eof ();

  if (idevice::ptr iptr = iptr_.lock ())
    {
      try
        {
          rv = iptr->marker ();
          if (traits::eof () == rv)
            rv = iptr->marker ();
        }
      catch (const exception& e)
        {
          work_in_progress_ = false;
          cancel_requested_ = work_in_progress_;

          last_marker_ = traits::eof ();
          cache_.reset ();

          throw;
        }
    }

  if (traits::is_marker (rv)) last_marker_ = rv;

  return rv;
}

void
handle::add_option (option& visitor)
{
  if (name::num_options == visitor.key () && 0 < sod_.size ())
    return;

  if (sod_.empty ()
      && name::num_options != visitor.key ())
    {
      BOOST_THROW_EXCEPTION
        (logic_error
         ("SANE API specification violation\n"
          "The option number count has to be the first option."));
    }

  try
    {
      option_descriptor sod (visitor);
      sod_.push_back (sod);
    }
  catch (const runtime_error& e)
    {
      log::error (e.what ());
    }
}

void
handle::update_options (SANE_Word *info)
{
  if (opt_.count (option_prefix / "enable-resampling"))
    {
      toggle t = value (opt_[option_prefix / "enable-resampling"]);

      std::vector< option_descriptor >::iterator it = sod_.begin ();
      for (; it != sod_.end (); ++it)
        {
          using namespace xlate;

          mapping sw_res;
          /**/ if (sw_resolution.second == it->sane_key)
            sw_res = (t ? sw_resolution : resolution);
          else if (sw_resolution_x.second == it->sane_key)
            sw_res = (t ? sw_resolution_x : resolution_x);
          else if (sw_resolution_y.second == it->sane_key)
            sw_res = (t ? sw_resolution_y : resolution_y);
          else if ("resolution-bind" == it->sane_key)
            sw_res = (t ? sw_resolution_bind : resolution_bind);
          else
            continue;           // nothing to do

          utsushi::key k (option_prefix / sw_res.first);

          if (opt_.count (k))
            {
              *it = option_descriptor (opt_[k]);
              if (info) *info |= SANE_INFO_RELOAD_OPTIONS;
              if (info) *info |= SANE_INFO_RELOAD_PARAMS;
            }
        }
    }

  std::vector< option_descriptor >::iterator it = sod_.begin ();

  ++it;                         // do not modify name::num_options

  for (; it != sod_.end (); ++it)
    {
      if (opt_.count (it->orig_key))
        {
          option_descriptor od (opt_[it->orig_key]);

          if (*it != od)
            {
              *it = od;
              if (info) *info |= SANE_INFO_RELOAD_OPTIONS;
            }
        }
    }
  update_capabilities (info);
}

void
handle::update_capabilities (SANE_Word *info)
{
  std::vector< option_descriptor >::iterator it = sod_.begin ();

  ++it;                         // do not modify name::num_options

  for (; it != sod_.end (); ++it)
    {
      SANE_Int cap = it->cap;

      if (!opt_.count (it->orig_key))
        {
          it->cap |= SANE_CAP_INACTIVE;
        }
      else                      // check Utsushi option attributes
        {
          option opt (opt_[it->orig_key]);

          if (opt.is_active ())
            {
              it->cap &= ~SANE_CAP_INACTIVE;
            }
          else
            {
              it->cap |= SANE_CAP_INACTIVE;
            }
          if (opt.is_read_only ())
            {
              it->cap &= ~(  SANE_CAP_HARD_SELECT
                           | SANE_CAP_SOFT_SELECT);
            }
        }

      if (info && cap != it->cap) *info |= SANE_INFO_RELOAD_OPTIONS;
    }
}

struct match_key
{
  const utsushi::key& k_;

  match_key (const utsushi::key& k)
    : k_(k)
  {}

  bool operator() (const handle::option_descriptor& od) const
  {
    return k_ == od.orig_key;
  }
};

//! Converts an utsushi::key into a valid SANE option descriptor name
/*! Any utsushi::key characters that are not allowed are converted to
 *  an ASCII dash (\c 0x2D).  Keys matching a well-known SANE option
 *  are converted to the corresponding SANE option descriptor name (as
 *  defined in Sec. 4.5 of the specification).  Several other keys may
 *  be converted in a similar way to provide more meaningful command
 *  line options.
 */
static
std::string
sanitize_(const utsushi::key& k)
{
  if (k == name::num_options)
    return k;

  // SANE API sanctioned ASCII characters for option names

  static const std::string lower_case ("abcdefghijklmnopqrstuvwxyz");
  static const std::string dash_digit ("-0123456789");

  static const std::list< xlate::mapping >
    dictionary = boost::assign::list_of
    (xlate::resolution)
    (xlate::preview)
    (xlate::tl_x)
    (xlate::tl_y)
    (xlate::br_x)
    (xlate::br_y)
    // SANE 2 draft extensions
    (xlate::resolution_x)
    (xlate::resolution_y)
    (xlate::doc_source)
    (xlate::image_type)
    // Software emulated resolutions
    (xlate::sw_resolution)
    (xlate::sw_resolution_x)
    (xlate::sw_resolution_y)
    (xlate::resolution_bind)
    (xlate::sw_resolution_bind)
    ;

  std::string rv (k);

  BOOST_FOREACH (xlate::mapping entry, dictionary)
    {
      /**/ if (k == option_prefix / entry.first)
        {
          rv = entry.second;
        }
      else if (rv == std::string (entry.first))
        {
          rv = entry.second;
        }
      else if (0 == rv.find (std::string (option_prefix)))
        {
          std::string tmp (rv.substr (std::string (option_prefix).size () + 1));
          if (0 == tmp.find_first_of (lower_case))
            rv = tmp;
        }
      else if (0 == rv.find (filter_prefix))
        {
          std::string tmp (rv.substr (filter_prefix.size () + 1));
          if (0 == tmp.find_first_of (lower_case))
            rv = tmp;
        }
      else if (0 == rv.find (magick_prefix))
        {
          std::string tmp (rv.substr (magick_prefix.size () + 1));
          if (tmp == std::string (entry.first))
            rv = entry.second;
          else if (0 == tmp.find_first_of (lower_case))
            rv = tmp;
        }
      else if (0 == rv.find (action_prefix))
        {
          std::string tmp (rv.substr (action_prefix.size () + 1));
          if (0 == tmp.find_first_of (lower_case))
            rv = tmp;
        }
    }

  if (0 != rv.find_first_of (lower_case))
    {
      BOOST_THROW_EXCEPTION
        (logic_error
         ("SANE API specification violation\n"
          "Option names must start with a lower-case ASCII character."));
    }

  std::string::size_type i (0);
  while (std::string::npos != i)
    {
      i = rv.find_first_not_of (lower_case + dash_digit, i);
      if (std::string::npos != i)
        rv.at (i) = '-';
    }

  return rv;
}

handle::option_descriptor::option_descriptor ()
{
  sane_key = sanitize_(orig_key);

  name  = sane_key.c_str ();
  title = name_.c_str ();
  desc  = desc_.c_str ();

  type = SANE_TYPE_GROUP;
  unit = SANE_UNIT_NONE;
  size = 0;
  cap  = SANE_CAP_INACTIVE;

  constraint_type  = SANE_CONSTRAINT_NONE;
  constraint.range = nullptr;
}

handle::option_descriptor::option_descriptor (const handle::option_descriptor& od)
{
  constraint_type = SANE_CONSTRAINT_NONE;       // inspected by operator=
  *this = od;
}

/*! \todo  Make sure SOD::title is a single line?
 *  \todo  Use \c '\\n' to separate paragraphs in SOD::desc
 *  \todo  Remove markup from SOD::title and SOD::desc
 */
handle::option_descriptor::option_descriptor (const option& visitor)
{
  orig_key = visitor.key ();
  sane_key = sanitize_(orig_key);
  name_ = visitor.name ();
  if (visitor.text ())
    desc_  = visitor.text ();
  else
    desc_ = visitor.name ();

  name  = sane_key.c_str ();
  title = name_.c_str ();
  desc  = desc_.c_str ();

  const sane::value sv (visitor);
  type = sv.type ();
  unit = sv.unit ();
  size = sv.size ();

  cap = SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;

  constraint_type  = SANE_CONSTRAINT_NONE;
  constraint.range = nullptr;

  if (!name::is_well_known (sane_key))
    {
      if (!visitor.is_at(level::standard))
        cap |= SANE_CAP_ADVANCED;
    }
  if (   0 == std::string (orig_key).find (filter_prefix)
      || 0 == std::string (orig_key).find (magick_prefix))
    {
      cap |= SANE_CAP_EMULATED;
    }
  if (name::is_resolution (sane_key))
    {
      unit = SANE_UNIT_DPI;
    }
  if (name::is_scan_area (sane_key))
    {
      type = SANE_TYPE_FIXED;
      unit = SANE_UNIT_MM;
    }

  if (name::num_options == sane_key)
    {
      return;
    }

  if (SANE_TYPE_BUTTON == type)
    {
      return;
    }

  // The SANE_Option_Descriptor basics have been taken care off now.
  // Next, deal with the UI constraint and add an appropriate SANE
  // constraint type (if necessary).

  if (constraint::ptr cp = visitor.constraint ())
    {
      /**/ if (typeid (*cp.get ()) == typeid (utsushi::constraint))
        {
          // setting constrained on bounded value type
        }
      else if (range *r = dynamic_cast< range * > (cp.get ()))
        {
          SANE_Range *sr = new SANE_Range;

          // FIXME remove unit conversion kludge
          quantity factor (1);
          if (name::is_scan_area (sane_key))
            factor = unit::mm_per_inch;

          sane::value (r->lower () * factor, type) >> &sr->min;
          sane::value (r->upper () * factor, type) >> &sr->max;
          sane::value (r->quant () * factor, type) >> &sr->quant;

          constraint.range = sr;
          constraint_type  = SANE_CONSTRAINT_RANGE;
        }
      else if (store *s = dynamic_cast< store * > (cp.get ()))
        {
          /**/ if (   type == SANE_TYPE_INT
                   || type == SANE_TYPE_FIXED)
            {
              SANE_Word *sw = new SANE_Word[1 + s->size ()];
              SANE_Word *wp = sw;

              *wp++ = s->size ();

              store::const_iterator it;
              for (it = s->begin (); s->end () != it; ++wp, ++it)
                {
                  sane::value v (*it, type);
                  // FIXME remove unit conversion kludge
                  if (name::is_scan_area (sane_key))
                    v *= unit::mm_per_inch;
                  v >> wp;
                }

              constraint.word_list = sw;
              constraint_type = SANE_CONSTRAINT_WORD_LIST;
            }
          else if (SANE_TYPE_STRING == type)
            {
              SANE_String_Const *sl = new SANE_String_Const[1 + s->size ()];
              strings_.reserve (s->size ());

              int i = 0;
              store::const_iterator it = s->begin ();
              for (; s->end () != it; ++i, ++it)
                {
                  string s = *it;
                  strings_.push_back(s);
                  sl[i] = strings_.back ().c_str ();
                }
              sl[i] = NULL;

              constraint.string_list = sl;
              constraint_type = SANE_CONSTRAINT_STRING_LIST;
            }
          else
            {
              BOOST_THROW_EXCEPTION
                (runtime_error
                 ("SANE API: list constraint value type not supported"));
            }
        }
      else
        {
          if (SANE_TYPE_BOOL != type)
            BOOST_THROW_EXCEPTION
              (runtime_error
               ("SANE API: constraint type not supported"));
        }
    }
  else
    {
      // setting _not_ constrained on bounded value type
      // Constraining on bounded value type through the SANE API will
      // somewhat limit the possibilities but never cause a violation;
      // setting can be added safely.
    }
}

handle::option_descriptor::~option_descriptor ()
{
  /**/ if (constraint_type == SANE_CONSTRAINT_NONE)
    {
    }
  else if (constraint_type == SANE_CONSTRAINT_RANGE)
    {
      delete constraint.range;
    }
  else if (constraint_type == SANE_CONSTRAINT_WORD_LIST)
    {
      delete [] constraint.word_list;
    }
  else if (constraint_type == SANE_CONSTRAINT_STRING_LIST)
    {
      delete [] constraint.string_list;
    }
  else
    {
      log::error ("unknown constraint type");
    }
}

handle::option_descriptor&
handle::option_descriptor::operator= (const handle::option_descriptor& rhs)
{
  orig_key = rhs.orig_key;
  sane_key = sanitize_(orig_key);
  name_    = rhs.name_;
  desc_    = rhs.desc_;
  strings_ = rhs.strings_;

  name  = sane_key.c_str ();
  title = name_.c_str ();
  desc  = desc_.c_str ();

  type = rhs.type;
  unit = rhs.unit;
  size = rhs.size;
  cap  = rhs.cap;

  /**/ if (constraint_type == SANE_CONSTRAINT_NONE)
    {
    }
  else if (constraint_type == SANE_CONSTRAINT_RANGE)
    {
      delete constraint.range;
    }
  else if (constraint_type == SANE_CONSTRAINT_WORD_LIST)
    {
      delete [] constraint.word_list;
    }
  else if (constraint_type == SANE_CONSTRAINT_STRING_LIST)
    {
      delete [] constraint.string_list;
    }
  else
    {
      log::error ("unknown constraint type");
    }
  constraint_type = rhs.constraint_type;
  /**/ if (constraint_type == SANE_CONSTRAINT_NONE)
    {
      constraint.range = rhs.constraint.range;
    }
  else if (constraint_type == SANE_CONSTRAINT_RANGE)
    {
      SANE_Range *sr = new SANE_Range;
      memcpy (sr, rhs.constraint.range, sizeof (*sr));
      constraint.range = sr;
    }
  else if (constraint_type == SANE_CONSTRAINT_WORD_LIST)
    {
      size_t sz = 1 + rhs.constraint.word_list[0];
      SANE_Word *sw = new SANE_Word[sz];
      memcpy (sw, rhs.constraint.word_list, sz * sizeof (SANE_Word));
      constraint.word_list = sw;
    }
  else if (constraint_type == SANE_CONSTRAINT_STRING_LIST)
    {
      SANE_String_Const *sl = new SANE_String_Const[strings_.size () + 1];

      int i = 0;
      std::vector< utsushi::string >::const_iterator it;
      for (it = strings_.begin (); it != strings_.end (); ++i, ++it)
        {
          sl[i] = it->c_str ();
        }
      sl[i] = NULL;
      constraint.string_list = sl;
    }
  else
    {
      log::error ("unknown constraint type");
    }
  return *this;
}

bool
handle::option_descriptor::operator== (const handle::option_descriptor& rhs)
  const
{
  bool rv (   orig_key == rhs.orig_key
           && sane_key == rhs.sane_key
           && name_    == rhs.name_
           && desc_    == rhs.desc_
           && strings_ == rhs.strings_);

  // Compare the SANE_Option_Descriptor base "class" part

  rv &= (  (!name  && !rhs.name )       // both NULL
         || (name  &&  rhs.name  && (0 == strcmp (name , rhs.name ))));
  rv &= (  (!title && !rhs.title)
         || (title &&  rhs.title && (0 == strcmp (title, rhs.title))));
  rv &= (  (!desc  && !rhs.desc )
         || (desc  &&  rhs.desc  && (0 == strcmp (desc , rhs.desc ))));

  rv &= (type == rhs.type);
  rv &= (unit == rhs.unit);
  rv &= (size == rhs.size);
  rv &= (cap  == rhs.cap );

  if (rv && constraint_type == rhs.constraint_type)
    {
      /**/ if (constraint_type == SANE_CONSTRAINT_NONE)
        {
          // nothing to do
        }
      else if (constraint_type == SANE_CONSTRAINT_RANGE)
        {
          const SANE_Range *lhs_ = constraint.range;
          const SANE_Range *rhs_ = rhs.constraint.range;

          rv &= (   lhs_->min   == rhs_->min
                 && lhs_->max   == rhs_->max
                 && lhs_->quant == rhs_->quant);
        }
      else if (constraint_type == SANE_CONSTRAINT_WORD_LIST)
        {
          const SANE_Word *lhs_ = constraint.word_list;
          const SANE_Word *rhs_ = rhs.constraint.word_list;

          SANE_Int n = *lhs_;
          do
            {
              rv &= (*lhs_ == *rhs_);
              ++lhs_;
              ++rhs_;
            }
          while (rv && --n >= 0);
        }
      else if (constraint_type == SANE_CONSTRAINT_STRING_LIST)
        {
          const SANE_String_Const *lhs_ = constraint.string_list;
          const SANE_String_Const *rhs_ = rhs.constraint.string_list;

          rv &= (lhs_ && rhs_);
          while (rv && *lhs_ && *rhs_)
            {
              rv &= (0 == strcmp (*lhs_, *rhs_));
              ++lhs_;
              ++rhs_;
            }
          rv &= (!lhs_ && !rhs_);       // both NULL for equality
        }
      else
        {
          BOOST_THROW_EXCEPTION
            (runtime_error
             ("SANE API: list constraint value type not supported"));
        }
    }

  return rv;
}

void
handle::add_group (const key& key, const string& name, const string& text)
{
  option_descriptor sod;

  sod.orig_key = key;
  sod.sane_key = sanitize_(key);
  sod.name_ = name;
  sod.desc_ = text;
  sod.name  = sod.sane_key.c_str ();
  sod.title = sod.name_.c_str ();
  if (sod.desc_)
    sod.desc  = sod.desc_.c_str ();
  else
    sod.desc  = sod.sane_key.c_str ();
  sod.type  = SANE_TYPE_GROUP;
  sod.unit  = SANE_UNIT_NONE;
  sod.size  = 0;
  sod.cap   = 0;
  sod.constraint_type = SANE_CONSTRAINT_NONE;

  sod_.push_back (sod);
}

}       // namespace sane
