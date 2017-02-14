//  compound-tweaks.cpp -- address model specific issues
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

#include <algorithm>
#include <vector>

#include <boost/assign/list_of.hpp>
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
      constraint::ptr res (from< range > ()
                           -> bounds (50, 600)
                           -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (adf_res_x_) = res;
      if (caps.rss)
        {
          const_cast< constraint::ptr& > (adf_res_y_) = res;
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
     CCB_N_("Speed"),
     CCB_N_("Optimize image acquisition for speed")
     );

  // FIXME disable workaround for #1094
  descriptors_["speed"]->active (false);
  descriptors_["speed"]->read_only (true);

  // FIXME disable workaround for limitations mentioned in #1098
  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);

  // autocrop/deskew parameter
  add_options ()
    ("lo-threshold", quantity (12.1))
    ("hi-threshold", quantity (25.4))
    ("auto-kludge", toggle (false))
    ;
  descriptors_["lo-threshold"]->read_only (true);
  descriptors_["hi-threshold"]->read_only (true);
  descriptors_["auto-kludge"]->read_only (true);
}

DS_3x0::DS_3x0 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res (from< range > ()
                           -> bounds (50, 600)
                           -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (adf_res_x_) = res;
      if (caps.rss)
        {
          const_cast< constraint::ptr& > (adf_res_y_) = res;
        }
    }

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 1024 * 1024;
  caps.bsz = capabilities::range (1, *defs.bsz);

  // Color correction parameters

  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 1.015;
  exp[1] = 0.991;
  exp[2] = 0.994;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  1.0250;
  mat[0][1] =  0.0004;
  mat[0][2] = -0.0254;
  mat[1][0] =  0.0003;
  mat[1][1] =  1.0022;
  mat[1][2] = -0.0025;
  mat[2][0] =  0.0049;
  mat[2][1] = -0.0949;
  mat[2][2] =  1.0900;

  // disable HW crop/deskew
  using namespace code_token::capability;
  if (caps.adf && caps.adf->flags)
    {
      erase (*caps.adf->flags, adf::CRP);
      erase (*caps.adf->flags, adf::SKEW);
    }

  read_back_ = false;
}

void
DS_3x0::configure ()
{
  compound_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);

  // autocrop/deskew parameter
  add_options ()
    ("lo-threshold", quantity (65.6))
    ("hi-threshold", quantity (80.4))
    ("auto-kludge", toggle (false))
    ;
  descriptors_["lo-threshold"]->read_only (true);
  descriptors_["hi-threshold"]->read_only (true);
  descriptors_["auto-kludge"]->read_only (true);
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
      constraint::ptr res (from< range > ()
                           -> bounds (50, 600)
                           -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (adf_res_x_) = res;
      if (caps.rss)
        {
          const_cast< constraint::ptr& > (adf_res_y_) = res;
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
     CCB_N_("Speed"),
     CCB_N_("Optimize image acquisition for speed")
     );

  // FIXME disable workaround for #1094
  descriptors_["speed"]->active (false);
  descriptors_["speed"]->read_only (true);

  // FIXME disable workaround for limitations mentioned in #1098
  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);

  // autocrop/deskew parameter
  add_options ()
    ("lo-threshold", quantity (60.2))
    ("hi-threshold", quantity (79.3))
    ("auto-kludge", toggle (false))
    ;
  descriptors_["lo-threshold"]->read_only (true);
  descriptors_["hi-threshold"]->read_only (true);
  descriptors_["auto-kludge"]->read_only (true);
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

PX_Mxxxx::PX_Mxxxx (const connexion::ptr& cnx)
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
    if ("PID 08CE" == info.product_name ()) product = "PX-M860F";
    if ("PID 08CF" == info.product_name ()) product = "WF-6590";
    if (!product.empty ())
      info.product.assign (product.begin (), product.end ());
  }

  // Disable long paper support
  if (info.adf)
    {
      info.adf->max_doc = info.adf->area;
    }

  // In some devices, ADF max scan area differ between simplex and duplex
  if (   "PID 1126" == info.product_name ()
      && info.adf)
    {
      info.adf->min_doc[1] = 826;

      if (info.adf->duplex_passes)
        {
          adf_duplex_min_doc_height_ = 1011;
          adf_duplex_max_doc_height_ = 1170;
        }
    }

  // Disable 300dpi vertical resolution for performance reasons.
  // Acquiring at 400dpi is faster for some reason.
  if (caps.rss)
    {
      try
        {
          std::vector< integer >& v
#if 105800 <= BOOST_VERSION
            = boost::relaxed_get< std::vector< integer >& > (*caps.rss);
#else
            = boost::get< std::vector< integer >& > (*caps.rss);
#endif
          erase (v, 300);
        }
      catch (const boost::bad_get& e)
        {
          log::alert (e.what ());
        }
    }

  if (HAVE_MAGICK)              /* enable resampling */
    {
      if (info.flatbed)
        {
          constraint::ptr fb_res (from< range > ()
                                  -> bounds (50, info.flatbed->resolution)
                                  -> default_value (*defs.rsm));
          const_cast< constraint::ptr& > (fb_res_x_) = fb_res;
          if (caps.rss)
            const_cast< constraint::ptr& > (fb_res_y_) = fb_res;
        }
      if (info.adf)
        {
          constraint::ptr adf_res (from< range > ()
                                   -> bounds (50, info.adf->resolution)
                                   -> default_value (*defs.rsm));
          const_cast< constraint::ptr& > (adf_res_x_) = adf_res;
          if (caps.rss)
            const_cast< constraint::ptr& > (adf_res_y_) = adf_res;
        }
    }

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 256 * 1024;

  // Color correction parameters
  vector< double, 3 > exp;
  matrix< double, 3 > mat;

  exp[0] = 1.012;
  exp[1] = 0.991;
  exp[2] = 0.998;

  mat[0][0] =  1.0559;  mat[0][1] =  0.0471;  mat[0][2] = -0.1030;
  mat[1][0] =  0.0211;  mat[1][1] =  1.0724;  mat[1][2] = -0.0935;
  mat[2][0] =  0.0091;  mat[2][1] = -0.1525;  mat[2][2] =  1.1434;

  static const vector< double, 3 > gamma_exponent_1 = exp;
  static const matrix< double, 3 > profile_matrix_1 = mat;

  exp[0] = 1.009;
  exp[1] = 0.992;
  exp[2] = 0.999;

  mat[0][0] =  1.0042;  mat[0][1] =  0.0009;  mat[0][2] = -0.0051;
  mat[1][0] =  0.0094;  mat[1][1] =  1.0411;  mat[1][2] = -0.0505;
  mat[2][0] =  0.0092;  mat[2][1] = -0.1000;  mat[2][2] =  1.0908;

  static const vector< double, 3 > gamma_exponent_2 = exp;
  static const matrix< double, 3 > profile_matrix_2 = mat;

  exp[0] = 1.010;
  exp[1] = 0.997;
  exp[2] = 0.993;

  mat[0][0] =  0.9864;  mat[0][1] =  0.0248;  mat[0][2] = -0.0112;
  mat[1][0] =  0.0021;  mat[1][1] =  1.0100;  mat[1][2] = -0.0121;
  mat[2][0] =  0.0139;  mat[2][1] = -0.1249;  mat[2][2] =  1.1110;

  static const vector< double, 3 > gamma_exponent_3 = exp;
  static const matrix< double, 3 > profile_matrix_3 = mat;

  exp[0] = 1.014;
  exp[1] = 0.993;
  exp[2] = 0.993;

  mat[0][0] =  0.9861;  mat[0][1] =  0.0260;  mat[0][2] = -0.0121;
  mat[1][0] =  0.0044;  mat[1][1] =  1.0198;  mat[1][2] = -0.0242;
  mat[2][0] =  0.0132;  mat[2][1] = -0.1264;  mat[2][2] =  1.1132;

  static const vector< double, 3 > gamma_exponent_4 = exp;
  static const matrix< double, 3 > profile_matrix_4 = mat;

  static const std::map< std::string, const vector< double, 3 > >
    gamma_exponent = boost::assign::map_list_of
    ("PX-M7050",   gamma_exponent_1)
    ("PX-M7050FX", gamma_exponent_1)
    ("PX-M860F",   gamma_exponent_1)
    ("WF-6590",    gamma_exponent_1)
    //
    ("PID 1112",   gamma_exponent_2)
    //
    ("PID 1125",   gamma_exponent_3)
    ("PID 1127",   gamma_exponent_3)
    //
    ("PID 1126",   gamma_exponent_4)
    ;

  static const std::map< std::string, const matrix< double, 3 > >
    profile_matrix = boost::assign::map_list_of
    ("PX-M7050",   profile_matrix_1)
    ("PX-M7050FX", profile_matrix_1)
    ("PX-M860F",   profile_matrix_1)
    ("WF-6590",    profile_matrix_1)
    //
    ("PID 1112",   profile_matrix_2)
    //
    ("PID 1125",   profile_matrix_3)
    ("PID 1127",   profile_matrix_3)
    //
    ("PID 1126",   profile_matrix_4)
    ;

  try {
    exp = gamma_exponent.at (info.product_name ());
    mat = profile_matrix.at (info.product_name ());

    vector< double, 3 >& ge
      (const_cast< vector< double, 3 >& > (gamma_exponent_));
    ge = exp;

    matrix< double, 3 >& pm
      (const_cast< matrix< double, 3 >& > (profile_matrix_));
    pm = mat;
  }
  catch (const std::out_of_range&) {}
}

void
PX_Mxxxx::configure ()
{
  compound_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

DS_530_570W::DS_530_570W (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res (from< range > ()
                           -> bounds (50, 600)
                           -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (adf_res_x_) = res;
      if (caps.rss)
        {
          const_cast< constraint::ptr& > (adf_res_y_) = res;
        }
    }

  // Assume people prefer brighter colors over B/W
  defs.col = code_token::parameter::col::C024;
  defs.gmm = code_token::parameter::gmm::UG18;

  // Boost USB I/O throughput
  defs.bsz = 1024 * 1024;
  caps.bsz = capabilities::range (1, *defs.bsz);

  // Color correction parameters

  vector< double, 3 >& exp
    (const_cast< vector< double, 3 >& > (gamma_exponent_));

  exp[0] = 1.012;
  exp[1] = 0.994;
  exp[2] = 0.994;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  1.0229;
  mat[0][1] =  0.0009;
  mat[0][2] = -0.0238;
  mat[1][0] =  0.0031;
  mat[1][1] =  1.0287;
  mat[1][2] = -0.0318;
  mat[2][0] =  0.0044;
  mat[2][1] = -0.1150;
  mat[2][2] =  1.1106;
}

void
DS_530_570W::configure ()
{
  compound_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

DS_16x0::DS_16x0 (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  // Both resolution settings need to be identical
  caps.rss = boost::none;

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr fb_res (from< range > ()
                              -> bounds (50, 1200)
                              -> default_value (*defs.rsm));
      constraint::ptr adf_res (from< range > ()
                               -> bounds (50, 600)
                               -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (fb_res_x_) = fb_res;
      const_cast< constraint::ptr& > (adf_res_x_) = adf_res;
      if (caps.rss)
        {
          const_cast< constraint::ptr& > (fb_res_y_) = fb_res;
          const_cast< constraint::ptr& > (adf_res_y_) = adf_res;
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

  exp[0] = 1.011;
  exp[1] = 0.990;
  exp[2] = 1.000;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  0.9883;
  mat[0][1] =  0.0242;
  mat[0][2] = -0.0125;
  mat[1][0] =  0.0013;
  mat[1][1] =  1.0046;
  mat[1][2] = -0.0059;
  mat[2][0] =  0.0036;
  mat[2][1] = -0.0620;
  mat[2][2] =  1.0584;
}

void
DS_16x0::configure ()
{
  compound_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

EP_30VA::EP_30VA (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res (from< range > ()
                           -> bounds (50, 2400)
                           -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (fb_res_x_) = res;

      if (caps.rss)
        {
          const_cast< constraint::ptr& > (fb_res_y_) = res;
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

  exp[0] = 1.014;
  exp[1] = 0.990;
  exp[2] = 0.997;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  0.9803;
  mat[0][1] =  0.0341;
  mat[0][2] = -0.0144;
  mat[1][0] =  0.0080;
  mat[1][1] =  1.0308;
  mat[1][2] = -0.0388;
  mat[2][0] =  0.0112;
  mat[2][1] = -0.1296;
  mat[2][2] =  1.1184;
}

void
EP_30VA::configure ()
{
  compound_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

EP_879A::EP_879A (const connexion::ptr& cnx)
  : compound_scanner (cnx)
{
  capabilities& caps (const_cast< capabilities& > (caps_));
  parameters&   defs (const_cast< parameters& > (defs_));

  if (HAVE_MAGICK)              /* enable resampling */
    {
      constraint::ptr res (from< range > ()
                           -> bounds (50, 1200)
                           -> default_value (*defs.rsm));
      const_cast< constraint::ptr& > (fb_res_x_) = res;

      if (caps.rss)
        {
          const_cast< constraint::ptr& > (fb_res_y_) = res;
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

  exp[0] = 1.010;
  exp[1] = 0.997;
  exp[2] = 0.993;

  matrix< double, 3 >& mat
    (const_cast< matrix< double, 3 >& > (profile_matrix_));

  mat[0][0] =  0.9864;
  mat[0][1] =  0.0248;
  mat[0][2] = -0.0112;
  mat[1][0] =  0.0021;
  mat[1][1] =  1.0100;
  mat[1][2] = -0.0121;
  mat[2][0] =  0.0139;
  mat[2][1] = -0.1249;
  mat[2][2] =  1.1110;
}

void
EP_879A::configure ()
{
  compound_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
