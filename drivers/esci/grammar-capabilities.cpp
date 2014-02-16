//  grammar-capabilities.cpp -- component instantiations
//  Copyright (C) 2012-2014  SEIKO EPSON CORPORATION
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

#ifndef USE_SINGLE_FILE_GRAMMAR_INSTANTIATION

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//! \copydoc grammar.cpp

#include <algorithm>
#include <limits>
#include <set>

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/range.hpp>
#include <utsushi/store.hpp>

#include "grammar-capabilities.ipp"

#define for_each BOOST_FOREACH

namespace utsushi {
namespace _drv_ {
namespace esci {

/*! \todo Add a mechanism to specify the default value.  There should
 *        probably be support for first/last for stores and lower,
 *        upper, center and zero for ranges.  For ranges we default to
 *        lower, but switch to zero if it's contained in the range.
 */
class constraint_visitor
  : public boost::static_visitor< utsushi::constraint::ptr >
{
public:
  constraint_visitor ()
    : max_(std::numeric_limits< integer >::max ())
    , multiplier_(1)
  {}
  constraint_visitor (double multiplier)
    : max_(std::numeric_limits< integer >::max ())
    , multiplier_(multiplier)
  {}
  constraint_visitor (const integer& max)
    : max_(max)
    , multiplier_(1)
  {}

  result_type
  operator() (const capabilities::range& r) const
  {
    quantity lo (std::min (quantity (r.lower_), max_));
    quantity hi (std::min (quantity (r.upper_), max_));
    quantity dv (lo);

    if (lo > hi) std::swap (lo, hi);
    if (lo < 0 && hi > 0) dv = 0;

    lo *= multiplier_;
    hi *= multiplier_;
    dv *= multiplier_;

    return constraint::ptr
      (from< utsushi::range > ()
       -> lower (lo)
       -> upper (hi)
       -> default_value (dv));
  }

  result_type
  operator() (const std::vector< integer >& v) const
  {
    using boost::lambda::_1;

    std::vector< quantity > v_capped (v.size (), quantity (1 + max_));

    v_capped.erase
      (remove_copy_if (v.begin (), v.end (), v_capped.begin (), _1 > max_),
       v_capped.end ());

    if (v_capped.empty ())
      return constraint::ptr ();

    for_each (quantity q, v_capped) q *= multiplier_;

    return constraint::ptr
      (from< utsushi::store > ()
       -> alternatives (v_capped.begin (), v_capped.end ())
       -> default_value (v_capped.front ()));
  }

private:
  quantity max_;
  quantity multiplier_;
};

bool
capabilities::operator== (const capabilities& rhs) const
{
  return (   adf == rhs.adf
          && tpu == rhs.tpu
          && fb  == rhs.fb
          && col == rhs.col
          && fmt == rhs.fmt
          && jpg == rhs.jpg
          && thr == rhs.thr
          && dth == rhs.dth
          && gmm == rhs.gmm
          && gmt == rhs.gmt
          && cmx == rhs.cmx
          && sfl == rhs.sfl
          && mrr == rhs.mrr
          && bsz == rhs.bsz
          && pag == rhs.pag
          && rsm == rhs.rsm
          && rss == rhs.rss
          && crp == rhs.crp
          && fcs == rhs.fcs
          && flc == rhs.flc
          && fla == rhs.fla
          && qit == rhs.qit);
}

void
capabilities::clear ()
{
  *this = capabilities ();
}

capabilities::operator bool () const
{
  return (   adf
          || tpu
          || fb
          || col
          || fmt
          || jpg
          || thr
          || dth
          || gmm
          || gmt
          || cmx
          || sfl
          || mrr
          || bsz
          || pag
          || rsm
          || rss
          || crp
          || fcs
          || flc
          || fla
          || qit);
}

bool
capabilities::has_duplex () const
{
  using namespace code_token::capability;

  return (adf
          && adf->flags
          && adf->flags->end () != std::find (adf->flags->begin (),
                                              adf->flags->end (),
                                              adf::DPLX));
}

utsushi::constraint::ptr
capabilities::border_fill () const
{
  using namespace code_token::capability::flc;
  using utsushi::constraint;

  if (!flc
      || flc->empty ())
    return constraint::ptr ();

  std::string default_fill (N_("None"));
  std::set< std::string > s;

  for_each (quad token, *flc)
    {
      std::string fill;

      switch (token)
        {
        case WH: fill = N_("White"); break;
        case BK: fill = N_("Black"); break;
        default:
          log::error ("unknown border-fill token: %1%") % str (token);
        }

      if (!fill.empty ()) s.insert (fill);
    }

  if (s.empty ()) return constraint::ptr ();

  return constraint::ptr
    (from< store > ()
     -> alternatives (s.begin (), s.end ())
     -> default_value (default_fill));
}

utsushi::constraint::ptr
capabilities::border_size (const quantity& default_value) const
{
  using utsushi::constraint;

 if (!fla) return constraint::ptr ();

 constraint::ptr cp (boost::apply_visitor (constraint_visitor (0.01), *fla));
 cp->default_value (default_value);

 return cp;
}

utsushi::constraint::ptr
capabilities::buffer_size (const boost::optional< integer >& default_value) const
{
  using utsushi::constraint;

  if (!bsz) return constraint::ptr ();

  constraint::ptr cp (boost::apply_visitor (constraint_visitor (), *bsz));
  if (cp && default_value) cp->default_value (*default_value);

  return cp;
}

utsushi::constraint::ptr
capabilities::crop_adjustment () const
{
  if (!crp) return utsushi::constraint::ptr ();

  return boost::apply_visitor (constraint_visitor (0.01), *crp);
}

utsushi::constraint::ptr
capabilities::document_sources (const quad& default_value) const
{
  using namespace code_token::parameter;
  using utsushi::constraint;

  std::set< std::string > s;

  if (adf) s.insert (N_("ADF"));
  if (tpu) s.insert (N_("TPU"));
  if (fb ) s.insert (N_("Flatbed"));

  if (s.empty ()) return constraint::ptr ();

  std::string default_source;

  switch (default_value)
    {
    case ADF: default_source = N_("ADF"); break;
    case TPU: default_source = N_("TPU"); break;
    case FB : default_source = N_("Flatbed"); break;
    default:
      default_source = *s.begin ();
    }

  return constraint::ptr
    (from< store > ()
     -> alternatives (s.begin (), s.end ())
     -> default_value (default_source));
}

utsushi::constraint::ptr
capabilities::double_feed () const
{
  using namespace code_token::capability::adf;
  using utsushi::constraint;

  if (!adf || !adf->flags) return constraint::ptr ();

  bool dfl1 (std::count (adf->flags->begin (), adf->flags->end (), DFL1));
  bool dfl2 (std::count (adf->flags->begin (), adf->flags->end (), DFL2));

  if (dfl1 && dfl2)
    {
      store::ptr s = make_shared< store > ();

      s -> alternative (N_("Off"))
        -> alternative (N_("Normal"))
        -> alternative (N_("Sensitive"))
        -> default_value (s->front ());

      return s;
    }
  if (dfl1)
    {
      return make_shared< constraint > (toggle ());
    }
  if (dfl2)
    {
      /*! \todo Determine whether this is a F/W bug or not.  If not,
       *        figure out how to deal with it.  The constraint can
       *        be a simple toggle but the value to be sent to the
       *        device needs to be different from the dfl1 toggle.
       */
    }

  return constraint::ptr ();
}

utsushi::constraint::ptr
capabilities::dropouts () const
{
  using namespace code_token::capability::col;
  using utsushi::constraint;

  if (!col
      || col->empty ())
    return constraint::ptr ();

  std::string default_dropout (N_("None"));
  std::set< std::string > s;

  for_each (quad token, *col)
    {
      std::string dropout;

      switch (token)
        {
        case R001: dropout = N_("Red (1 bit)"); break;
        case R008: dropout = N_("Red (8 bit)"); break;
        case R016: dropout = N_("Red (16 bit)"); break;
        case G001: dropout = N_("Green (1 bit)"); break;
        case G008: dropout = N_("Green (8 bit)"); break;
        case G016: dropout = N_("Green (16 bit)"); break;
        case B001: dropout = N_("Blue (1 bit)"); break;
        case B008: dropout = N_("Blue (8 bit)"); break;
        case B016: dropout = N_("Blue (16 bit)"); break;
          // ignore all non-dropouts
        case C003:
        case C024:
        case C048:
        case M001:
        case M008:
        case M016: break;
        default:
          log::error ("unknown dropout: %1%") % str (token);
        }

      if (!dropout.empty ()) s.insert (dropout);
    }

  if (s.empty ()) return constraint::ptr ();

  return constraint::ptr
    (from< store > ()
     -> alternatives (s.begin (), s.end ())
     -> default_value (default_dropout));
}

utsushi::constraint::ptr
capabilities::formats (const boost::optional< quad >& default_value) const
{
  using namespace code_token::capability::fmt;
  using utsushi::constraint;

  if (!fmt
      || fmt->empty ())
    return constraint::ptr ();

  std::string default_format;
  std::set< std::string > ss;

  for_each (quad token, *fmt)
    {
      std::string s;

      switch (token)
        {
        case RAW : s = N_("RAW" ); break;
        case JPG : s = N_("JPEG"); break;
        default:
          log::error ("unknown image transfer format: %1%") % str (token);
        }

      if (!s.empty ())
        {
          ss.insert (s);
          if (default_value
              && token == *default_value) default_format = s;
        }
    }

  if (ss.empty ()) return constraint::ptr ();

  if (!default_value)
    default_format = *ss.begin ();
  else if (default_format.empty ())
    {
      log::error ("unknown default image transfer format: %1%, using first")
        % str (*default_value)
        ;
      default_format = *ss.begin ();
    }

  return constraint::ptr
    (from< store > ()
     -> alternatives (ss.begin (), ss.end ())
     -> default_value (default_format));
}

utsushi::constraint::ptr
capabilities::gamma (const boost::optional< quad >& default_value) const
{
  using namespace code_token::capability::gmm;
  using utsushi::constraint;

  if (!gmm
      || gmm->empty ())
    return constraint::ptr ();

  std::string default_gamma;
  std::set< std::string > ss;

  for_each (quad token, *gmm)
    {
      std::string s;

      switch (token)
        {
        case UG10: s = "1.0"; break;
        case UG18: s = "1.8"; break;
        case UG22: s = "2.2"; break;
        default:
          log::error ("unknown user gamma token: %1%") % str (token);
        }

      if (!s.empty ())
        {
          ss.insert (s);
          if (default_value
              && token == *default_value) default_gamma = s;
        }
    }

  if (ss.empty ()) return constraint::ptr ();

  if (!default_value)
    default_gamma = *ss.begin ();
  else if (default_gamma.empty ())
    {
      log::error
        ("unknown default user gamma token: %1%, using first")
        % str (*default_value)
        ;
      default_gamma = *ss.begin ();
    }

  return constraint::ptr
    (from< store > ()
     -> alternatives (ss.begin (), ss.end ())
     -> default_value (default_gamma));
}

utsushi::constraint::ptr
capabilities::image_count (const boost::optional< integer >& default_value) const
{
  using utsushi::constraint;

  if (!pag) return constraint::ptr ();

  constraint::ptr cp (boost::apply_visitor (constraint_visitor (), *pag));
  if (cp && default_value) cp->default_value (*default_value);

  return cp;
}

utsushi::constraint::ptr
capabilities::image_types (const boost::optional< quad >& default_value) const
{
  using namespace code_token::capability::col;
  using utsushi::constraint;

  if (!col
      || col->empty ())
    return constraint::ptr ();

  std::string default_type;
  std::set< std::string > s;

  for_each (quad token, *col)
    {
      std::string type;

      switch (token)
        {
          //! \todo Use values that are more command-line friendly
        case C003: type = N_("Color (1 bit)"); break;
        case C024: type = N_("Color (8 bit)"); break;
        case C048: type = N_("Color (16 bit)"); break;
        case M001: type = N_("Gray (1 bit)"); break;
        case M008: type = N_("Gray (8 bit)"); break;
        case M016: type = N_("Gray (16 bit)"); break;
          // ignore all dropouts
        case R001:
        case R008:
        case R016:
        case G001:
        case G008:
        case G016:
        case B001:
        case B008:
        case B016: break;
        default:
          log::error ("unknown image type: %1%") % str (token);
        }

      if (!type.empty ())
        {
          s.insert (type);
          if (default_value
              && token == *default_value) default_type = type;
        }
    }

  if (s.empty ()) return constraint::ptr ();

  if (!default_value)
    default_type = *s.begin ();
  else if (default_type.empty ())
    {
      log::error ("unknown default image type: %1%, using first")
        % str (*default_value)
        ;
      default_type = *s.begin ();
    }

  return constraint::ptr
    (from< store > ()
     -> alternatives (s.begin (), s.end ())
     -> default_value (default_type));
}

utsushi::constraint::ptr
capabilities::jpeg_quality (const boost::optional< integer >& default_value) const
{
  if (!jpg) return utsushi::constraint::ptr ();

  constraint_visitor cv;
  utsushi::constraint::ptr cp (cv (*jpg));

  if (default_value) cp->default_value (*default_value);

  return cp;
}

utsushi::constraint::ptr
capabilities::resolutions (const quad& direction,
                           const boost::optional< integer >& default_value,
                           const integer& max) const
{
  using namespace code_token::capability;
  using utsushi::constraint;

  constraint::ptr cp;

  if (RSM == direction && rsm)
    {
      cp = boost::apply_visitor (constraint_visitor (max), *rsm);
      if (cp && default_value) cp->default_value (*default_value);
      return cp;
    }
  if (RSS == direction && rss)
    {
      cp = boost::apply_visitor (constraint_visitor (max), *rss);
      if (cp && default_value) cp->default_value (*default_value);
      return cp;
    }

  return cp;
}

utsushi::constraint::ptr
capabilities::threshold (const boost::optional< integer >& default_value) const
{
  if (!thr) return utsushi::constraint::ptr ();

  constraint_visitor cv;
  utsushi::constraint::ptr cp (cv (*thr));

  if (default_value) cp->default_value (*default_value);

  return cp;
}

capabilities::range::range (const integer& lower, const integer& upper)
  : lower_(lower)
  , upper_(upper)
{}

bool
capabilities::range::operator== (const capabilities::range& rhs) const
{
  return (   lower_ == rhs.lower_
          && upper_ == rhs.upper_);
}

bool
capabilities::document_source::operator== (const capabilities::document_source& rhs) const
{
  return (   flags      == rhs.flags
          && resolution == rhs.resolution);
}

bool
capabilities::tpu_source::operator== (const capabilities::tpu_source& rhs) const
{
  return (   document_source::operator== (rhs)
          && area             == rhs.area
          && alternative_area == rhs.alternative_area);
}

capabilities::focus_control::focus_control ()
  : automatic (false)
{}

bool
capabilities::focus_control::operator== (const capabilities::focus_control& rhs) const
{
  return (   automatic == rhs.automatic
          && position  == rhs.position);
}

template class
decoding::basic_grammar_capabilities< decoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif /* !defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
