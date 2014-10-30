//  scanner.cpp -- API implementation for an ESC/I driver
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

#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>

#include "compound-scanner.hpp"
#include "compound-tweaks.hpp"
#include "extended-scanner.hpp"
#include "get-identity.hpp"
#include "scanner.hpp"
#include "scanner-inquiry.hpp"

#ifdef HAVE_STANDARD_SCANNER
#include "standard-scanner.hpp"
#endif

namespace utsushi {

extern "C" {

void
libdrv_esci_LTX_scanner_factory (scanner::ptr& rv, connexion::ptr cnx)
{
  using namespace _drv_;

  esci::scanner::ptr sp;

  using namespace esci;

  if (!cnx)
    {
      log::fatal
        ("expected an established connexion");

      return;
    }

  try
    {
      scanner_inquiry FS_Y;
      information info;

      *cnx << FS_Y.get (info);
      *cnx << FS_Y.finish ();

      log::brief ("detected a '%1%'") % info.product_name ();

      /**/ if (info.product_name () == "DS-40")
        {
          sp = make_shared< DS_40 > (cnx);
        }
      else if (   info.product_name () == "DS-510"
               || info.product_name () == "DS-520"
               || info.product_name () == "DS-560"
               )
        {
          sp = make_shared< DS_5x0 > (cnx);
        }
      else if (   info.product_name () == "DS-760"
               || info.product_name () == "DS-860"
               )
        {
          sp = make_shared< DS_760_860 > (cnx);
        }
      else if (   info.product_name () == "DS-5500"
               || info.product_name () == "DS-6500"
               || info.product_name () == "DS-7500"
              )
        {
          sp = make_shared< DS_x500 > (cnx);
        }
      else if (   info.product_name () == "DS-50000"
               || info.product_name () == "DS-60000"
               || info.product_name () == "DS-70000"
              )
        {
          sp = make_shared< DS_x0000 > (cnx);
        }
      else if (   info.product_name () == "PID 08BC"
               || info.product_name () == "PID 08CC")
        {
          sp = make_shared< PX_M7050 > (cnx);
        }
      else
        {
          sp = make_shared< compound_scanner > (cnx);
        }
    }
  catch (const invalid_command& e)
    {
      log::brief
        ("does not appear to be an ESC/I-2 device (%2%)")
        % e.what ();
    }
  catch (const unknown_reply&)
    {
      // log::error
    }

  if (!sp)
    {
      try
        {
          get_identity ESC_I;

          *cnx << ESC_I;
          if (ESC_I.supports_extended_commands ())
            {
              sp = make_shared< extended_scanner > (cnx);
            }
#ifdef HAVE_STANDARD_SCANNER
          else
            {
              sp = make_shared< standard_scanner > (cnx);
            }
#endif
        }
      catch (const invalid_command& e)
        {
          log::brief
            ("does not appear to be an ESC/I device (%2%)")
            % e.what ();
        }
      catch (const unknown_reply&)
        {
          // log::error
        }
      if (!sp) log::error ("not supported");
    }

  sp->configure ();
  rv = sp;
}

} // extern "C"

namespace _drv_ {
namespace esci {

scanner::scanner (const connexion::ptr& cnx)
  : utsushi::scanner (cnx)
  , gamma_exponent_(1)
{
  matrix< double, 3 >& mat
    = const_cast< matrix< double, 3 >& > (profile_matrix_);

  // TODO add a unit or diagonal matrix ctor so we can do this in the
  //      initializer list
  for (size_t i = 0; i < mat.rows (); ++i)
    mat[i][i] = 1;
}

bool
scanner::set_up_sequence ()
{
  val_ = values ();

  set_up_initialize ();

  set_up_doc_source ();
  set_up_image_mode ();

  set_up_gamma_tables ();
  set_up_color_matrices ();

  set_up_auto_area_segmentation ();
  set_up_threshold ();
  set_up_dithering ();

  set_up_sharpness ();
  set_up_brightness ();

  set_up_mirroring ();

  set_up_scan_speed ();
  set_up_scan_count ();

  set_up_resolution ();
  set_up_scan_area ();

  set_up_transfer_size ();

  return set_up_hardware ();
}

void
scanner::set_up_initialize ()
{}

bool
scanner::set_up_hardware ()
{
  return true;
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
