//  media.cpp -- related properties
//  Copyright (C) 2013, 2015  SEIKO EPSON CORPORATION
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

#include "utsushi/media.hpp"

#include "utsushi/i18n.hpp"
#include "utsushi/key.hpp"
#include "utsushi/quantity.hpp"

#include <boost/assign/list_inserter.hpp>
#include <boost/foreach.hpp>

#include <map>

namespace utsushi {

namespace {

  const length inches = 1.0;
  const length mm     = inches / 25.4;

  typedef std::map< std::string, media > dictionary;
  dictionary *dict = nullptr;

  /*! \todo Initialize from a user customizable data file with support
   *        for application predefined sizes.  The user should not
   *        have to bother with defining common sizes such as ISO/A3,
   *        only with what list of sizes should be used.  Not everyone
   *        wants to see a next to exhaustive list of sizes for every
   *        media size that has been standardized.  Support for custom
   *        sizes is a must.  PWG 5101.1-2002 defines a decent naming
   *        scheme that we could build on.
   */
  void initialize_dictionary ()
  {
    if (dict) return;

    dict = new dictionary;
    boost::assign::insert (*dict)
      // ISO A Series
      (CCB_N_("ISO/A3"), media (297 * mm, 420 * mm))
      (CCB_N_("ISO/A4"), media (210 * mm, 297 * mm))
      (CCB_N_("ISO/A5"), media (148 * mm, 210 * mm))
      (CCB_N_("ISO/A6"), media (105 * mm, 148 * mm))
      // JIS B Series
      (CCB_N_("JIS/B4"), media (257 * mm, 364 * mm))
      (CCB_N_("JIS/B5"), media (182 * mm, 257 * mm))
      (CCB_N_("JIS/B6"), media (128 * mm, 182 * mm))
      // North American
      (SEC_N_("Ledger")   , media (11.00 * inches, 17.00 * inches))
      (SEC_N_("Legal")    , media ( 8.50 * inches, 14.00 * inches))
      (SEC_N_("Letter")   , media ( 8.50 * inches, 11.00 * inches))
      (SEC_N_("Executive"), media ( 7.25 * inches, 10.50 * inches))
      ;
  }
}       // namespace

struct media::impl
{
  length width_;
  length height_;

  impl ()
    : width_(length ())
    , height_(length ())
  {}

  impl (const length& width, const length& height)
    : width_(width)
    , height_(height)
  {}

};

media::media (const length& width, const length& height)
  : pimpl_(new impl (width, height))
{}

media::media (const media& m)
  : pimpl_(new impl (*m.pimpl_))
{}

media::~media ()
{
  delete pimpl_;
}

media&
media::operator= (const media& m)
{
  if (this != &m)
    *pimpl_ = *m.pimpl_;

  return *this;
}

length
media::width () const
{
  return pimpl_->width_;
}

length
media::height () const
{
  return pimpl_->height_;
}

media
media::lookup (const std::string& name)
{
  if (!dict) initialize_dictionary ();

  std::string base_name (name);
  std::string::size_type pos = base_name.rfind ("/Landscape");

  bool transpose = (pos != std::string::npos);

  if (!transpose)
    pos = base_name.rfind ("/Portrait");

  base_name = base_name.substr (0, pos);

  dictionary::const_iterator it (dict->find (base_name));

  if (dict->end () == it) return media (length (), length ());

  if (transpose)
    return media (it->second.height (), it->second.width ());

  return it->second;
}

/*! \todo Move orientation to a separate property.  It should not be
 *        part of the media name.  Various APIs treat orientation as a
 *        separate entity already and PWG 5101.1-2002 advises to do so
 *        as well.  The only reason not to do so rightaway is rather
 *        poor support for option dependencies.
 */
std::list< std::string >
media::within (const length& min_width, const length& min_height,
               const length& max_width, const length& max_height)
{
  std::list< std::string > rv;

  if (!dict) initialize_dictionary ();

  BOOST_FOREACH (dictionary::value_type entry, *dict)
    {
      length w = entry.second.width ();
      length h = entry.second.height ();
      if (   min_width  <= w && w <= max_width
          && min_height <= h && h <= max_height)
        {
          if (   min_width  <= h && h <= max_width
              && min_height <= w && w <= max_height)
            {
              rv.push_back (entry.first + "/Portrait");
              rv.push_back (entry.first + "/Landscape");
            }
          else
            {
              rv.push_back (entry.first + "/Portrait");
            }
        }
    }

  return rv;
}

}       // namespace utsushi
