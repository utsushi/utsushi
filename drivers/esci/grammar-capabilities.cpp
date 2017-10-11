//  grammar-capabilities.cpp -- component instantiations
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
          && qit == rhs.qit
          && lam == rhs.lam);
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
          || qit
          || lam);
}

static bool inline
find_(boost::optional< std::vector< quad > > flags, const quad& token)
{
  return (flags
          && flags->end () != std::find (flags->begin (), flags->end (),
                                         token));
}

bool
capabilities::has_duplex () const
{
  using namespace code_token::capability;

  return (adf && find_(adf->flags, adf::DPLX));
}

bool
capabilities::has_media_end_detection () const
{
  using namespace code_token::capability;

  return (adf && find_(adf->flags, adf::PEDT));
}

bool
capabilities::can_calibrate () const
{
  using namespace code_token::capability;

  return (adf && find_(adf->flags, adf::CALB));
}

bool
capabilities::can_clean () const
{
  using namespace code_token::capability;

  return (adf && find_(adf->flags, adf::CLEN));
}

bool
capabilities::can_eject () const
{
  using namespace code_token::capability;

  return (adf && find_(adf->flags, adf::EJCT));
}

bool
capabilities::can_load () const
{
  using namespace code_token::capability;

  return (adf && find_(adf->flags, adf::LOAD));
}

bool
capabilities::can_crop (const quad& src) const
{
  using namespace code_token::capability;

  if (src == FB ) return (fb  && find_(fb->flags , fb::CRP ));
  if (src == ADF) return (adf && find_(adf->flags, adf::CRP));
  if (src == TPU) return (tpu && find_(tpu->flags, tpu::CRP));

  return false;
}

utsushi::constraint::ptr
capabilities::border_fill () const
{
  using namespace code_token::capability::flc;
  using utsushi::constraint;

  if (!flc
      || flc->empty ())
    return constraint::ptr ();

  std::string default_fill (SEC_N_("None"));
  std::set< std::string > s;

  for_each (quad token, *flc)
    {
      std::string fill;

      switch (token)
        {
        case WH: fill = SEC_N_("White"); break;
        case BK: fill = SEC_N_("Black"); break;
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

  if (adf) s.insert (SEC_N_("ADF"));
  if (tpu) s.insert (SEC_N_("Transparency Unit"));
  if (fb ) s.insert (SEC_N_("Document Table"));

  if (s.empty ()) return constraint::ptr ();

  std::string default_source;

  switch (default_value)
    {
    case ADF: default_source = SEC_N_("ADF"); break;
    case TPU: default_source = SEC_N_("Transparency Unit"); break;
    case FB : default_source = SEC_N_("Document Table"); break;
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
  bool sdf  (std::count (adf->flags->begin (), adf->flags->end (), SDF ));
  bool spp  (std::count (adf->flags->begin (), adf->flags->end (), SPP ));

  if (dfl1 && dfl2)
    {
      store::ptr s = make_shared< store > ();

      s -> alternative (SEC_N_("Off"))
        -> alternative (SEC_N_("Normal"))
        -> alternative (SEC_N_("Thin"))
        -> default_value (s->front ());

      return s;
    }
  if (dfl1)
    {
      return make_shared< constraint > (toggle ());
    }
  if (sdf && spp)
    {
    store::ptr s = make_shared< store > ();

      s -> alternative (SEC_N_("Off"))
        -> alternative (SEC_N_("On"))
        -> alternative (SEC_("Paper Protection"))
        -> default_value (s->front ());

      return s;
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

bool
capabilities::has_double_feed_off_command () const
{
  using namespace code_token::capability::adf;
  using utsushi::constraint;

  if (!adf || !adf->flags) return false;

  return bool (std::count (adf->flags->begin (), adf->flags->end (), DFL0));
}

utsushi::constraint::ptr
capabilities::dropouts () const
{
  using namespace code_token::capability::col;
  using utsushi::constraint;

  if (!col
      || col->empty ())
    return constraint::ptr ();

  std::string default_dropout (SEC_N_("None"));
  std::set< std::string > s;

  int depth_001 = 0;
  int depth_008 = 0;
  int depth_016 = 0;

  const int R = 0x01;
  const int G = 0x02;
  const int B = 0x04;
  const int RGB = (R | G | B);

  for_each (quad token, *col)
    {
      std::string dropout;

      switch (token)
        {
        case R001: depth_001 |= R; break;
        case R008: depth_008 |= R; break;
        case R016: depth_016 |= R; break;
        case G001: depth_001 |= G; break;
        case G008: depth_008 |= G; break;
        case G016: depth_016 |= G; break;
        case B001: depth_001 |= B; break;
        case B008: depth_008 |= B; break;
        case B016: depth_016 |= B; break;
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
    }

  if (depth_001 && RGB != depth_001)
    log::debug ("Bit depth  1 dropouts incomplete, %x") % depth_001;
  if (depth_008 && RGB != depth_008)
    log::debug ("Bit depth  8 dropouts incomplete, %x") % depth_008;
  if (depth_016 && RGB != depth_016)
    log::debug ("Bit depth 16 dropouts incomplete, %x") % depth_016;

  if (   depth_001 == RGB
      || depth_008 == RGB
      || depth_016 == RGB)
    {
      s.insert (SEC_N_("Red"));
      s.insert (SEC_N_("Green"));
      s.insert (SEC_N_("Blue"));
    }

  if (s.empty ()) return constraint::ptr ();

  return constraint::ptr
    (from< store > ()
     -> alternatives (s.begin (), s.end ())
     -> default_value (default_dropout));
}

quad
capabilities::get_dropout (const quad& gray, const string& color) const
{
  using namespace code_token::capability::col;

  if (color == "None") return gray;
  if (color == "Red")
    {
      if (M001 == gray) return R001;
      if (M008 == gray) return R008;
      if (M016 == gray) return R016;
    }
  if (color == "Green")
    {
      if (M001 == gray) return G001;
      if (M008 == gray) return G008;
      if (M016 == gray) return G016;
    }
  if (color == "Blue")
    {
      if (M001 == gray) return B001;
      if (M008 == gray) return B008;
      if (M016 == gray) return B016;
    }

  log::error
    ("internal inconsistency:"
     " '%1%' dropout for '%2%' not supported, using '%2%'")
    % color
    % str (gray)
    ;

  return gray;
}

bool
capabilities::has_dropout (const quad& gray) const
{
  using namespace code_token::capability::col;

  if (!col
      || col->empty ())
    return false;

  // We rely on capabilities::dropouts() requiring the presence of
  // dropouts for all of the RGB components.  In that case, it is
  // sufficient to check for the presence of a single, arbitrary
  // component here.

  if (M001 == gray) return (col->end ()
                            != std::find (col->begin (), col->end (), R001));
  if (M008 == gray) return (col->end ()
                            != std::find (col->begin (), col->end (), R008));
  if (M016 == gray) return (col->end ()
                            != std::find (col->begin (), col->end (), R016));

  if (!(C048 == gray || C024 == gray || C003 == gray))
  log::error ("unknown color value: '%1%'") % str (gray);
    ;
  return false;
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
        case RAW : s = CCB_N_("RAW" ); break;
        case JPG : s = CCB_N_("JPEG"); break;
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
        case C003: type =        "Color (1 bit)";  break;
        case C024: type = SEC_N_("Color");         break;
        case C048: type =        "Color (16 bit)"; break;
        case M001: type = SEC_N_("Monochrome");    break;
        case M008: type = SEC_N_("Grayscale");     break;
        case M016: type =        "Gray (16 bit)";  break;
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
