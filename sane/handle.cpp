//  handle.cpp -- for a SANE scanner object
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#include <cstring>
#include <stdexcept>
#include <typeinfo>

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/range.hpp>
#include <utsushi/store.hpp>
#include "../filters/padding.hpp"
#include <utsushi/stream.hpp>
#include <utsushi/i18n.hpp>

#include "handle.hpp"
#include "log.hpp"
#include "value.hpp"

using std::exception;
using std::runtime_error;

namespace sane {

//! Keep backend options separate from frontend options
static const utsushi::key option_prefix ("device");

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

}       // namespace xlate

namespace unit
{
  static const utsushi::quantity::non_integer_type mm_per_inch = 25.4;
};

using std::logic_error;
using namespace utsushi;

#define null_ptr 0

handle::handle(const scanner::id& id)
  : name_(id.name () + " (" + id.udi () + ")")
  , idev_(scanner::create (connexion::create (id.iftype (), id.path ()), id))
  , last_marker_(traits::eos ())
  , work_in_progress_(false)
  , cancel_requested_(work_in_progress_)
{
  opt_.add_options ()
    (name::num_options, quantity (0),
     attributes ())
    ;
  opt_.add_option_map ()
    (option_prefix, idev_->options ())
    ;

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

  tags::const_iterator it (tags::begin ());
  for (; tags::end () != it; ++it)
    {
      if (tag::application == *it) continue;

      bool group_added = false;
      option::map::iterator om_it (opt_.begin ());
      for (; opt_.end () != om_it; ++om_it)
        {
          if (!seen.count (om_it->key ())
              && om_it->tags ().count (*it))
            {
              if (!group_added)
                {
                  add_group (option_prefix / *it,
                             it->name (), it->text ());
                  group_added = true;
                }
              add_option (*om_it);
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

  //  Pick up options without any tags

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
  update_capabilities (null_ptr);
}

static void
release (SANE_Option_Descriptor& sod)
{
  /**/ if (sod.constraint_type == SANE_CONSTRAINT_NONE)
    {
      return;
    }
  else if (sod.constraint_type == SANE_CONSTRAINT_RANGE)
    {
      delete sod.constraint.range;
    }
  else if (sod.constraint_type == SANE_CONSTRAINT_WORD_LIST)
    {
      delete [] sod.constraint.word_list;
    }
  else if (sod.constraint_type == SANE_CONSTRAINT_STRING_LIST)
    {
      delete [] sod.constraint.string_list;
    }
  else
    {
      log::error ("unknown constraint type");
    }
}

handle::~handle ()
{
  std::for_each (sod_.begin (), sod_.end (), boost::bind (release, _1));
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
  v >> value;

  return status;
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
          if (opt.is_read_only ())
            {
              it->cap &= ~(  SANE_CAP_HARD_SELECT
                           | SANE_CAP_SOFT_SELECT);
            }
        }

      if (info && cap != it->cap) *info |= SANE_INFO_RELOAD_OPTIONS;
    }
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

  if (opt_[k] == v) return SANE_STATUS_GOOD;

  if (is_scanning ()) return SANE_STATUS_DEVICE_BUSY;

  end_scan_sequence ();

  SANE_Status status = SANE_STATUS_GOOD;

  try
    {
      opt_[k] = v;

      // FIXME handling of info is not SANE spec compliant

      if (option_prefix / "image-type" == k)
        {
          // update_option (option_prefix / "transfer-format");

          if (info) *info |= SANE_INFO_RELOAD_OPTIONS;
        }
      if (option_prefix / "doc-source" == k)
        {
          update_option (option_prefix / "duplex");
          update_option (option_prefix / "scan-area");
          update_option (option_prefix / "tl-x");
          update_option (option_prefix / "tl-y");
          update_option (option_prefix / "br-x");
          update_option (option_prefix / "br-y");

          if (info) *info |= SANE_INFO_RELOAD_OPTIONS;
        }
      if (option_prefix / "scan-area" == k)
        {
          update_option (option_prefix / "tl-x");
          update_option (option_prefix / "tl-y");
          update_option (option_prefix / "br-x");
          update_option (option_prefix / "br-y");

          if (info) *info |= SANE_INFO_RELOAD_OPTIONS;
        }

      update_capabilities (info);

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
  if (istream::ptr istr = iptr_.lock ())
    return istr->get_context ();

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

      if (traits::boi () != last_marker_) istr_.reset ();
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
      if (istream::ptr istr = iptr_.lock ())
        {
          rv = istr->read (buffer, length);
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
      istr_.reset ();

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
      if (traits::eof () == last_marker_) istr_.reset ();
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
  idev_->cancel ();
  istr_.reset ();
}

streamsize
handle::marker ()
{
  /**/ if (  !istr_
           || traits::eos () == last_marker_
           || traits::eof () == last_marker_)
    {
      istream::ptr istr (new istream ());
      istr->push (ifilter::ptr (new _flt_::ipadding ()));
      istr->push (idev_);
      istr_ = istr;
      iptr_ = istr_;
    }

  streamsize rv = traits::eof ();

  if (istream::ptr istr = iptr_.lock ())
    {
      try
        {
          rv = istr->marker ();
        }
      catch (const exception& e)
        {
          work_in_progress_ = false;
          cancel_requested_ = work_in_progress_;

          last_marker_ = traits::eof ();
          istr_.reset ();

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

  // Don't expose the transfer-format option until we have suitable
  // filters that can convert to "RAW" format at our disposal.  The
  // jpeg-quality option only makes sense when JPEG transfer-format
  // is selected, so we suppress that as well.

  if (option_prefix / "transfer-format" == visitor.key ())
    {
      visitor = "RAW";
      return;
    }
  if (option_prefix / "jpeg-quality" == visitor.key ())
    return;

  if (sod_.empty ()
      && name::num_options != visitor.key ())
    {
      BOOST_THROW_EXCEPTION
        (logic_error
         (_("SANE API specification violation\n"
            "The option number count has to be the first option.")));
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

void
handle::update_option (const utsushi::key& k)
{
  if (!opt_.count (k)) return;          // no matching Utsushi option

  std::vector< option_descriptor >::iterator it;

  it = find_if (sod_.begin (), sod_.end (),
                match_key (k));

  if (sod_.end () == it) return;        // no matching SANE option

  *it = option_descriptor (opt_[k]);
}

//! Converts an utsushi::key into a valid SANE option descriptor name
/*! Any utsushi::key characters that are not allowed are converted to
 *  an ASCII dash (\c 0x2D).  Keys matching a well-known SANE option
 *  are converted to the corresponding SANE option descriptor name (as
 *  defined in Sec. 4.5 of the specification).  Matching is performed
 *  against the end of the utsushi::key.
 */
static
std::string
sanitize_(const utsushi::key& k)
{
  if (k == name::num_options)
    return k;

  std::string rv (k);

  static const std::list< xlate::mapping >
    well_known = boost::assign::list_of
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
    ;

  BOOST_FOREACH (xlate::mapping entry, well_known)
    {
      if (k == option_prefix / entry.first)
        {
          rv = entry.second;
        }
    }

  // SANE API sanctioned ASCII characters for option names

  static const std::string lower_case ("abcdefghijklmnopqrstuvwxyz");
  static const std::string dash_digit ("-0123456789");

  if (0 != rv.find_first_of (lower_case))
    {
      BOOST_THROW_EXCEPTION
        (logic_error
         (_("SANE API specification violation\n"
            "Option names must start with a lower-case ASCII character.")));
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

/*! \todo  Make sure SOD::title is a single line?
 *  \todo  Use \c '\\n' to separate paragraphs in SOD::desc
 *  \todo  Remove markup from SOD::title and SOD::desc
 */
handle::option_descriptor::option_descriptor (const option& visitor)
{
  orig_key = visitor.key ();
  sane_key = sanitize_(orig_key);

  name  = sane_key.c_str ();
  title = visitor.name ().c_str ();
  if (visitor.text ())
    desc  = visitor.text ().c_str ();
  else
    desc  = visitor.name ().c_str ();

  const sane::value sv (visitor);
  type = sv.type ();
  unit = sv.unit ();
  size = sv.size ();

  cap = SANE_CAP_SOFT_DETECT | SANE_CAP_SOFT_SELECT;

  constraint_type  = SANE_CONSTRAINT_NONE;
  constraint.range = null_ptr;

  if (!name::is_well_known (sane_key))
    {
      if (!visitor.is_at(level::standard))
        cap |= SANE_CAP_ADVANCED;
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

  // The SANE_Option_Descriptor basics have been taken care off now.
  // Next, deal with the UI constraint and add an appropriate SANE
  // constraint type (if necessary).

  if (constraint::ptr cp = visitor.constraint ())
    {
      /**/ if (typeid (*cp.get ()) == typeid (constraint))
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

              int i = 0;
              store::const_iterator it = s->begin ();
              for (; s->end () != it; ++i, ++it)
                {
                  string s = *it;
                  sl[i] = s.c_str ();
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
