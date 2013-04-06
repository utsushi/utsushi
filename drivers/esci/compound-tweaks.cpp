//  compound-tweaks.cpp -- address model specific issues
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

#include <algorithm>
#include <vector>

#include <boost/none.hpp>

#include "code-token.hpp"
#include "compound-tweaks.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

static void
erase (std::vector< quad >& v, const quad& token)
{
  v.erase (remove (v.begin (), v.end (), token), v.end ());
}

DS_xxx00::DS_xxx00 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  information&  info (const_cast< information& > (info_));
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  capabilities& caps_flip (const_cast< capabilities& > (caps_flip_));
  parameters&   defs_flip (const_cast< parameters& > (defs_flip_));

  // Disable flip-side scan parameter support because driver support
  // for it is not ready yet.  The protocol specification is missing
  // information needed for implementation.
  // Flip-side scan parameter support for these models was only added
  // after their initial launch.

  caps_flip = capabilities ();
  defs_flip = parameters ();

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  // Assume people prefer color over B/W
  defs.col = code_token::parameter::col::C024;

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
  if (caps.adf) erase (*caps.adf, adf::OVSN);
  if (caps.fb ) erase (*caps.fb , fb ::OVSN);
  if (caps.tpu && caps.tpu->other) erase (*caps.tpu->other, tpu::OVSN);

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

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
