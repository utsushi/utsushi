//  compound-tweaks.cpp -- address model specific issues
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <vector>

#include <boost/none.hpp>

#include <utsushi/i18n.hpp>
#include <utsushi/range.hpp>

#include "code-token.hpp"
#include "compound-tweaks.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

template< typename T >
static void
erase (std::vector< T >& v, const T& value)
{
  v.erase (remove (v.begin (), v.end (), value), v.end ());
}

DS_40::DS_40 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res_x (from< range > ()
                             -> bounds (50, 600)
                             -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (res_x_) = res_x;

      if (caps.rss)
        {
          constraint::ptr res_y (from< range > ()
                                 -> bounds (50, 600)
                                 -> default_value (*defs.rss));
          const_cast< constraint::ptr& > (res_y_) = res_y;
        }
    }

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 1024 * 1024;

  // Color correction parameters

  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 1.012;
  exp[1] = 0.994;
  exp[2] = 0.994;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  1.0392;
  mat[0][1] = -0.0023;
  mat[0][2] = -0.0369;
  mat[1][0] =  0.0146;
  mat[1][1] =  1.0586;
  mat[1][2] = -0.0732;
  mat[2][0] =  0.0191;
  mat[2][1] = -0.1958;
  mat[2][2] =  1.1767;

  read_back_ = false;           // see #1061
}

void
DS_40::configure ()
{
  compound_scanner::configure ();

  add_options ()
    ("speed", toggle (true),
     attributes (),
     N_("Speed"),
     N_("Optimize image acquisition for speed")
     );

  // FIXME disable workaround for #1094
  descriptors_["speed"]->active (false);
  descriptors_["speed"]->read_only (true);

  // FIXME disable workaround for limitations mentioned in #1098
  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

DS_5x0::DS_5x0 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res_x (from< range > ()
                             -> bounds (50, 600)
                             -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (res_x_) = res_x;

      if (caps.rss)
        {
          constraint::ptr res_y (from< range > ()
                                 -> bounds (50, 600)
                                 -> default_value (*defs.rss));
          const_cast< constraint::ptr& > (res_y_) = res_y;
        }
    }

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 256 * 1024;
  if (info_.product_name () == "DS-560") defs.bsz = 1024 * 1024;
  caps.bsz = capabilities::range (1, *defs.bsz);

  // Color correction parameters

  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 1.013;
  exp[1] = 0.992;
  exp[2] = 0.995;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  0.9929;
  mat[0][1] =  0.0066;
  mat[0][2] =  0.0005;
  mat[1][0] =  0.0016;
  mat[1][1] =  1.0116;
  mat[1][2] = -0.0132;
  mat[2][0] =  0.0082;
  mat[2][1] = -0.1479;
  mat[2][2] =  1.1397;
}

void
DS_5x0::configure ()
{
  compound_scanner::configure ();

  add_options ()
    ("speed", toggle (true),
     attributes (),
     N_("Speed"),
     N_("Optimize image acquisition for speed")
     );

  // FIXME disable workaround for #1094
  descriptors_["speed"]->active (false);
  descriptors_["speed"]->read_only (true);

  // FIXME disable workaround for limitations mentioned in #1098
  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

DS_760_860::DS_760_860 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  // Fix up incorrect JPEG quality range
  caps.jpg = capabilities::range (1, 100);

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 1024 * 1024;
}

DS_xxx00::DS_xxx00 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  information&  info (const_cast< information& > (info_));
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Device only ever uses 256 kib for the image data buffer size,
  // never mind what you set (#659).
  caps.bsz = boost::none;
  defs.bsz = 256 * 1024;

  // Disable overscan functionality as it does not seem to behave as
  // one would expect from the documentation.
  if (info.adf    ) info.adf    ->overscan.clear ();
  if (info.flatbed) info.flatbed->overscan.clear ();
  if (info.tpu    ) info.tpu    ->overscan.clear ();
  using namespace code_token::capability;
  if (caps.adf && caps.adf->flags) erase (*caps.adf->flags, adf::OVSN);
  if (caps.fb  && caps.fb ->flags) erase (*caps.fb ->flags, fb ::OVSN);
  if (caps.tpu && caps.tpu->flags) erase (*caps.tpu->flags, tpu::OVSN);

  read_back_ = false;
}

//! Color correction for the DS-5500, DS-6500 and DS-7500
DS_x500::DS_x500 (const connexion::ptr& cnx)
  : DS_xxx00 (cnx)
{
  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 0.987;
  exp[1] = 1.025;
  exp[2] = 0.987;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  1.2167;
  mat[0][1] = -0.2000;
  mat[0][2] = -0.0167;
  mat[1][0] = -0.2000;
  mat[1][1] =  1.3963;
  mat[1][2] = -0.1963;
  mat[2][0] =  0.0226;
  mat[2][1] = -0.2792;
  mat[2][2] =  1.2566;
}

//! Color correction for the DS-50000, DS-60000 and DS-70000
DS_x0000::DS_x0000 (const connexion::ptr& cnx)
  : DS_xxx00 (cnx)
{
  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 0.986;
  exp[1] = 1.011;
  exp[2] = 1.004;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  1.2061;
  mat[0][1] = -0.1764;
  mat[0][2] = -0.0297;
  mat[1][0] = -0.2005;
  mat[1][1] =  1.3300;
  mat[1][2] = -0.1295;
  mat[2][0] = -0.0083;
  mat[2][1] = -0.3662;
  mat[2][2] =  1.3745;
}

PX_M7050::PX_M7050 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  information&  info (const_cast< information& > (info_));
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Keep the "PID XXXX" product names out of sight.  Note that the
  // base class constructor already maps these for "refspec" access
  // purposes.  We could move this tinkering with the firmware info
  // into the refspec but that file may be used for other purposes.
  {
    std::string product;
    if ("PID 08BC" == info.product_name ()) product = "PX-M7050";
    if ("PID 08CC" == info.product_name ()) product = "PX-M7050FX";
    if (!product.empty ())
      info.product.assign (product.begin (), product.end ());
  }

  // Disable 300dpi vertical resolution for performance reasons.
  // Acquiring at 400dpi is faster for some reason.
  if (caps.rss)
    {
      try
        {
          std::vector< integer >& v
            = boost::get< std::vector< integer >& > (*caps.rss);
          erase (v, 300);
        }
      catch (const boost::bad_get& e)
        {
          log::alert (e.what ());
        }
    }

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res_x (from< range > ()
                             -> bounds (50, 1200)
                             -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (res_x_) = res_x;

      if (caps.rss)
        {
          constraint::ptr res_y (from< range > ()
                                 -> bounds (50, 1200)
                                 -> default_value (*defs.rss));
          const_cast< constraint::ptr& > (res_y_) = res_y;
        }
    }

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 256 * 1024;

  // Color correction parameters

  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 1.012;
  exp[1] = 0.991;
  exp[2] = 0.998;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  1.0559;
  mat[0][1] =  0.0471;
  mat[0][2] = -0.1030;
  mat[1][0] =  0.0211;
  mat[1][1] =  1.0724;
  mat[1][2] = -0.0935;
  mat[2][0] =  0.0091;
  mat[2][1] = -0.1525;
  mat[2][2] =  1.1434;
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
