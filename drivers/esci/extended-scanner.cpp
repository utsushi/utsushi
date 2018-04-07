//  extended-scanner.cpp -- devices that handle extended commands
//  Copyright (C) 2013, 2014  Olaf Meeuwissen
//  Copyright (C) 2012, 2015, 2016  SEIKO EPSON CORPORATION
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

#include <time.h>

#include <boost/assign/list_inserter.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bimap.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/exception.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/media.hpp>
#include <utsushi/range.hpp>
#include <utsushi/store.hpp>

#include "action.hpp"
#include "code-point.hpp"
#include "exception.hpp"
#include "extended-scanner.hpp"
#include "initialize.hpp"
#include "get-identity.hpp"
#include "set-color-matrix.hpp"
#include "set-dither-pattern.hpp"
#include "set-gamma-table.hpp"
#include "capture-scanner.hpp"
#include "release-scanner.hpp"

#define for_each BOOST_FOREACH

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace {

  typedef boost::bimap< byte, std::string > dictionary;

  store *
  store_from (dictionary *dict)
  {
    store *rv = new store;

    for (dictionary::right_const_iterator it = dict->right.begin ();
         it != dict->right.end (); ++it)
      rv->alternative (it->first);

    return rv;
  }

  // \todo  Support per instance dictionary
  dictionary *film_type = nullptr;

  store * film_types ()
  {
    if (!film_type)
      {
        film_type = new dictionary;
        boost::assign::insert (film_type->left)
          (POSITIVE_FILM, CCB_N_("Positive Film"))
          (NEGATIVE_FILM, CCB_N_("Negative Film"))
          ;
      }
    return store_from (film_type);
  }

  // \todo  Support per instance dictionary
  dictionary *gamma_correction = nullptr;

  store * gamma_corrections ()
  {
    if (!gamma_correction)
      {
        gamma_correction = new dictionary;
        boost::assign::insert (gamma_correction->left)
          (BI_LEVEL_CRT     , CCB_N_("Bi-level CRT"))
          (MULTI_LEVEL_CRT  , CCB_N_("Multi-level CRT"))
          (HI_DENSITY_PRINT , CCB_N_("High Density Print"))
          (LO_DENSITY_PRINT , CCB_N_("Low Density Print"))
          (HI_CONTRAST_PRINT, CCB_N_("High Contrast Print"))
          (CUSTOM_GAMMA_A   , CCB_N_("Custom (Base Gamma = 1.0)"))
          (CUSTOM_GAMMA_B   , CCB_N_("Custom (Base Gamma = 1.8)"))
          ;
      }
    return store_from (gamma_correction);
  }

  // \todo  Support per instance dictionary
  dictionary *color_correction = nullptr;

  store * color_corrections ()
  {
    if (!color_correction)
      {
        color_correction = new dictionary;
        boost::assign::insert (color_correction->left)
          (UNIT_MATRIX       , SEC_N_("None"))
        //(USER_DEFINED      , CCB_N_("User Defined"))
          (DOT_MATRIX_PRINTER, CCB_N_("Dot Matrix Printer"))
          (THERMAL_PRINTER   , CCB_N_("Thermal Printer"))
          (INKJET_PRINTER    , CCB_N_("Inkjet Printer"))
          (CRT_DISPLAY       , CCB_N_("CRT Display"))
          ;
      }
    return store_from (color_correction);
  }

  // \todo  Support per instance dictionary
  dictionary *dither_pattern = nullptr;

  store * dither_patterns ()
  {
    if (!dither_pattern)
      {
        dither_pattern = new dictionary;
        boost::assign::insert (dither_pattern->left)
          (BI_LEVEL       , CCB_N_("Bi-level"))
          (TEXT_ENHANCED  , CCB_N_("Text Enhanced"))
          (HARD_TONE      , CCB_N_("Hard Tone"))
          (SOFT_TONE      , CCB_N_("Soft Tone"))
          (NET_SCREEN     , CCB_N_("Net Screen"))
          (BAYER_4_4      , CCB_N_("Bayer 4x4"))
          (SPIRAL_4_4     , CCB_N_("Spiral 4x4"))
          (NET_SCREEN_4_4 , CCB_N_("Net Screen 4x4"))
          (NET_SCREEN_8_4 , CCB_N_("Net Screen 8x4"))
        //(CUSTOM_DITHER_A, CCB_N_("Value"))
        //(CUSTOM_DITHER_B, CCB_N_("Table"))
          ;
      }
    return store_from (dither_pattern);
  }

  //! \todo Make delay time interval configurable
  bool delay_elapsed ()
  {
    struct timespec t = { 0, 100000000 /* ns */ };

    return 0 == nanosleep (&t, 0);
  }

  inline
  quantity::integer_type
  int_cast (const uint32_t& i)
  {
    return boost::numeric_cast< quantity::integer_type > (i);
  }

  system_error::error_code
  status_to_error_code (const get_scanner_status& stat)
  {
    if (   stat.main_cover_open ()
        || stat.adf_cover_open ()
        || stat.tpu_cover_open (TPU1)
        || stat.tpu_cover_open (TPU2)) return system_error::cover_open;
    if (   stat.main_media_out ()
        || stat.adf_media_out ()) return system_error::media_out;
    if (   stat.main_media_jam ()
        || stat.adf_media_jam ()) return system_error::media_jam;
    if (!stat.is_ready ()) return system_error::permission_denied;

    return system_error::unknown_error;
  }

  std::string
  fallback_message (const get_scanner_status& stat)
  {
    return CCB_("Unknown device error");
  }

  std::string
  create_adf_message (const get_scanner_status& stat)
  {
    if (stat.adf_media_out ())
      return SEC_("Please load the document(s) into the Automatic Document"
                  " Feeder.");
    if (stat.adf_media_jam ())
      return SEC_("A paper jam occurred.\n"
               "Open the Automatic Document Feeder and remove any paper.\n"
               "If there are any documents loaded in the ADF, remove them"
               " and load them again.");
    if (stat.adf_cover_open ())
      return SEC_("The Automatic Document Feeder is open.\n"
                  "Please close it.");
    if (stat.adf_double_feed ())
      return SEC_("A multi page feed occurred in the auto document feeder. "
                  "Open the cover, remove the documents, and then try again."
                  " If documents remain on the tray, remove them and then"
                  " reload them.");
    if (stat.adf_error ())
      return CCB_("A fatal ADF error has occurred.\n"
                  "Resolve the error condition and try again.  You may have "
                  "to restart the scan dialog or application in order to be "
                  "able to scan.");

    return fallback_message (stat);
  }

  std::string
  create_fb_message (const get_scanner_status& stat)
  {
    return fallback_message (stat);
  }

  std::string
  create_tpu_message (const get_scanner_status& stat)
  {
    return fallback_message (stat);
  }

  std::string
  create_message (const get_scanner_status& stat)
  {
    if (   stat.adf_enabled ())     return create_adf_message (stat);
    if (   stat.tpu_enabled (TPU1)
        || stat.tpu_enabled (TPU2)) return create_tpu_message (stat);

    return create_fb_message (stat);
  }
}       // namespace

extended_scanner::extended_scanner (const connexion::ptr& cnx)
  : scanner (cnx)
  , caps_(true)
  , defs_(true)
  , acquire_(true)
  , stat_(true)
  , min_area_width_(0.05)
  , min_area_height_(0.05)
  , read_back_(true)
  , cancelled_(false)
  , locked_(false)
{
  initialize init;

  lock_scanner ();

  *cnx_ << init
        << const_cast< get_extended_identity& > (caps_)
        << const_cast< get_scan_parameters&   > (defs_)
        << stat_;

  unlock_scanner ();

  // increase default buffer size
  buffer_size_ = 256 * 1024;
}

static quantity
nearest_(const quantity& q, const constraint::ptr cp)
{
  /**/ if (dynamic_cast< range * > (cp.get ()))
    {
      range *rp = dynamic_cast< range * > (cp.get ());

      /**/ if (q < rp->lower ()) return rp->lower ();
      else if (q > rp->upper ()) return rp->upper ();
      else return q;
    }
  else if (dynamic_cast< store * > (cp.get ()))
    {
      store *sp = dynamic_cast< store * > (cp.get ());

      store::const_iterator it = sp->begin ();
      store::const_iterator rv = sp->begin ();
      quantity diff;

      while (sp->end () != it)
        {
          if (sp->begin () == it)
            {
              diff = abs (q - *it);
              rv   = it;
            }
          else
            {
              quantity d = abs (q - *it);
              if (d < diff)
                {
                  diff = d;
                  rv = it;
                }
            }
          ++it;
        }
      if (sp->end () != sp->begin ()) return *rv;
    }

  log::error ("no nearest value found, returning as is");

  return q;
}

void
extended_scanner::configure ()
{
  configure_doc_source_options ();
  add_resolution_options ();

  {
    add_options ()
      ("image-type", (from< store > ()
                      -> alternative (SEC_N_("Monochrome"))
                      -> alternative (SEC_N_("Grayscale"))
                      -> default_value (SEC_N_("Color"))
                ),
       attributes (tag::general)(level::standard),
       SEC_N_("Image Type")
       )
      ("speed", toggle (HI_SPEED == defs_.scan_mode ()),
       attributes (),
       CCB_N_("Speed")
       )
      ("line-count", (from< range > ()
                      -> lower (std::numeric_limits< uint8_t >::min ())
                      -> upper (std::numeric_limits< uint8_t >::max ())
                      -> default_value (defs_.line_count ())),
       attributes (),
       CCB_N_("Line Count"),
       CCB_N_("Specify how many scan lines to move from the device to the "
              "software in one transfer.  Note that 0 will use the maximum "
              "usable value.  Values larger than the maximum usable value "
              "are clamped to the maximum.")
       );
  }

  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("gamma-correction",
       (gamma_corrections ()
        -> default_value (gamma_correction
                          -> left.at (defs_.gamma_correction ()))),
       attributes (tag::enhancement),
       CCB_N_("Gamma Correction")
       );
  else
    add_options ()
      ("gamma", (from< store > ()
                 -> alternative ("1.0")
                 -> default_value ("1.8")),
       attributes (),
       CCB_N_("Gamma")
       );
  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("color-correction",
       (color_corrections ()
        -> default_value (color_correction
                          -> left.at (defs_.color_correction ()))),
       attributes (tag::enhancement),
       CCB_N_("Color Correction")
       );
  else
    configure_color_correction ();
  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("auto-area-segmentation", toggle (defs_.auto_area_segmentation ()),
       attributes (tag::enhancement)(level::standard),
       CCB_N_("Auto Area Segmentation"),
       CCB_N_("Threshold text regions and apply half-toning to photo/image"
              " areas.")
       );
  {
    add_options ()
      ("threshold", (from< range > ()
                     -> lower (std::numeric_limits< uint8_t >::min ())
                     -> upper (std::numeric_limits< uint8_t >::max ())
                     -> default_value (defs_.threshold ())
                     ),
       attributes (tag::enhancement)(level::standard),
       SEC_N_("Threshold")
       );
  }
  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("dither-pattern",
       (dither_patterns ()
        -> default_value (dither_pattern
                          -> left.at (defs_.halftone_processing ()))),
       attributes (tag::enhancement),
       CCB_N_("Dither Pattern")
       );
  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("sharpness", (from < range > ()
                     -> lower (int8_t (SMOOTHER))
                     -> upper (int8_t (SHARPER))
                     -> default_value (defs_.sharpness ())
                     ),
       attributes (tag::enhancement)(level::standard),
       CCB_N_("Sharpness"),
       CCB_N_("Emphasize the edges in an image more by choosing a larger"
              " value, less by selecting a smaller value.")
       );
  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("brightness", (from< range > ()
                      -> lower (int8_t (DARKEST))
                      -> upper (int8_t (LIGHTEST))
                      -> default_value (defs_.brightness ())
                      ),
       attributes (tag::enhancement)(level::standard),
       CCB_N_("Brightness"),
       CCB_N_("Make images look lighter with a larger value or darker with"
              " a smaller value.")
       );
  if (caps_.command_level ().at (0) != 'D')
    add_options ()
      ("mirror", toggle (defs_.mirroring ()),
       attributes (tag::enhancement)(level::standard),
       CCB_N_("Mirror")
       );

  //! \todo Remove transfer-format work-around for scan-cli utility
  add_options ()                // \todo Keep out of sight
    ("transfer-format", (from< store > ()
                         ->default_value ("RAW")
                         ),
     attributes (level::standard),
     SEC_N_("Transfer Format")
     )
    ;

  /*! \todo Remove this ugly hack.  It is only here to allow scan-cli
   *        to process all the options that might possibly be given on
   *        the command-line.  Its option parser only does a single
   *        pass on the options and chokes if there's anything that
   *        wasn't recognized (e.g. a `--duplex` option when given in
   *        combination with `--doc-source=ADF`).  At least, with the
   *        hack below all options are added to the CLI option parser.
   *        The content of the first added map takes precedence and
   *        later maps only add what is not there yet.
   */
  if (caps_.is_flatbed_type ()) insert (flatbed_);
  if (stat_.adf_detected ())    insert (adf_);
  if (stat_.tpu_detected ())    insert (tpu_);

  if (!validate (values ()))
    {
      BOOST_THROW_EXCEPTION
        (logic_error
         ("esci::extended_scanner(): internal inconsistency"));
    }
  finalize (values ());
}

bool
extended_scanner::is_single_image () const
{
  bool result = true;
  try
    {
      const string& s = *values_["doc-source"];

      result = (s != "ADF");
    }
  catch (const std::out_of_range&)
    {}
  return result;
}

bool
extended_scanner::is_consecutive () const
{
  return stat_.adf_enabled ();
}

bool
extended_scanner::obtain_media ()
{
  if (is_consecutive ()
      && !caps_.adf_is_auto_form_feeder ()
      && caps_.adf_is_page_type ())
    {
      try
        {
          load_media load;
          *cnx_ << load;
        }
      catch (const invalid_command& e)
        {
          log::alert (e.what ());
        }
      catch (const unknown_reply& e)
        {
          log::alert (e.what ());
        }
    }

  bool media_out = false;       // be optimistic

  *cnx_ << stat_;
  if (stat_.fatal_error ())
    {
      unlock_scanner ();

      media_out = stat_.adf_media_out () || stat_.main_media_out ();

      if (!media_out            // something else went wrong
          || (media_out && !images_started_))
        {
          BOOST_THROW_EXCEPTION
            (system_error
             (status_to_error_code (stat_), create_message (stat_)));
        }
    }
  return !media_out;
}

bool
extended_scanner::set_up_image ()
{
  chunk_  = chunk ();
  offset_ = 0;

  if (cancelled_)
    {
      unlock_scanner ();
      return false;
    }

  // \todo
  // need to recompute the scan area when FS_F_.media_value() returns
  // non-zero values and the user has selected scan-area == "Auto Detect"

  if (!set_up_hardware ())
    {
      unlock_scanner ();
      return false;
    }

  ctx_ = context (pixel_width (), pixel_height (), pixel_type ());
  ctx_.resolution (parm_.resolution ().x (), parm_.resolution ().y ());

  do
    {
      // \todo Allow cancellation if supported by device
      *cnx_ << stat_;
    }
  while (stat_.is_warming_up ()
         && delay_elapsed ());

  *cnx_ << acquire_;
  if (acquire_.detected_fatal_error ())
    {
      // "lazy" devices may only start warming up *after* they get a
      // request to start scanning
      do
        {
          // \todo Allow cancellation if supported by device
          *cnx_ << stat_;
        }
      while (stat_.is_warming_up ()
             && delay_elapsed ());

      *cnx_ << acquire_;
    }

  bool rv = acquire_.is_ready () && !acquire_.detected_fatal_error ();
  if (!rv)
    {
      *cnx_ << stat_;
      unlock_scanner ();

      BOOST_THROW_EXCEPTION
        (system_error
         (status_to_error_code (stat_), create_message (stat_)));
    }
  ++images_started_;
  return rv;
}

void
extended_scanner::finish_image ()
{
  if (is_consecutive ()
      && (!caps_.adf_is_auto_form_feeder ()
          || cancelled_ ) )
    {
      try
        {
          eject_media eject;
          *cnx_ << eject;
        }
      catch (const invalid_command& e)
        {
          log::alert (e.what ());
        }
      catch (const unknown_reply& e)
        {
          log::alert (e.what ());
        }
    }

  if (!is_consecutive ()
      || cancelled_)
    {
      unlock_scanner ();
    }
}

streamsize
extended_scanner::sgetn (octet *data, streamsize n)
{
  bool do_cancel = cancel_requested ();

  if (offset_ == chunk_.size ())
    {
      if (do_cancel) acquire_.cancel ();

      chunk_  = ++acquire_;
      offset_ = 0;

      cancelled_ = (!chunk_
                    && (do_cancel || acquire_.is_cancel_requested ()));
      if (cancelled_)
        {
          cancel ();            // notify idevice::read()
          return traits::eof ();
        }
    }

  streamsize rv = std::min (chunk_.size () - offset_, n);

  traits::copy (data, reinterpret_cast<const octet *>
                (chunk_.get () + offset_), rv);
  offset_ += rv;

  return rv;
}

void
extended_scanner::set_up_initialize ()
{
  parm_ = defs_;

  cancelled_ = false;
  images_started_ = 0;

  lock_scanner ();

  if (val_.count ("scan-area")
      && value ("Auto Detect") == val_["scan-area"])
    {
      media size = probe_media_size_(val_["doc-source"]);
      update_scan_area_(size, val_);
      option::map::finalize (val_);
    }
}

bool
extended_scanner::set_up_hardware ()
{
  try
    {
      *cnx_ << parm_;
      if (read_back_)
        {
          get_scan_parameters parm;
          *cnx_ << parm;
          if (parm != parm_)
            log::alert ("scan parameters may not be set as requested");
        }
    }
  catch (const invalid_parameter& e)
    {
      log::alert (e.what ());
      unlock_scanner ();
      return false;
    }
  return true;
}

void
extended_scanner::set_up_auto_area_segmentation ()
{
  if (!val_.count ("auto-area-segmentation")) return;

  toggle t = val_["auto-area-segmentation"];
  parm_.auto_area_segmentation (t);
}

void
extended_scanner::set_up_brightness ()
{
  if (!val_.count ("brightness")) return;

  quantity q = val_["brightness"];
  parm_.brightness (q.amount< int8_t > ());
}

void
extended_scanner::set_up_color_matrices ()
{
  if (!val_.count ("color-correction")) return;

  const string& s = val_["color-correction"];
  byte value = color_correction->right.at (s);
  parm_.color_correction (value);

  if (USER_DEFINED != value) return;

  // \todo Only send if changed since last time
  set_color_matrix cm;
  *cnx_ << cm ();
}

void
extended_scanner::set_up_dithering ()
{
  if (!val_.count ("dither-pattern")) return;

  const string& s = val_["dither-pattern"];
  byte value = dither_pattern->right.at (s);
  parm_.halftone_processing (value);

  if (!(   static_cast< byte > (CUSTOM_DITHER_A) == value
        || static_cast< byte > (CUSTOM_DITHER_B) == value)) return;

  // \todo Only send if changed since last time
  set_dither_pattern pattern;
  *cnx_ << pattern (static_cast< byte > (CUSTOM_DITHER_A) == value
                    ? set_dither_pattern::CUSTOM_A
                    : set_dither_pattern::CUSTOM_B);
}

void
extended_scanner::set_up_doc_source ()
{
  if (!val_.count ("doc-source")) return;

  source_value src = NO_SOURCE;

  const string& s = val_["doc-source"];

  if (!src && s == "Document Table") src = MAIN;
  if (!src && s == "ADF"    ) src = ADF;
  if (!src && s == "Transparency Unit"
      && (stat_.tpu_detected (TPU1))) src = TPU1;
  if (!src && s == "Transparency Unit"
      && (stat_.tpu_detected (TPU2))) src = TPU2;
  if (!src && s == "Primary TPU"  ) src = TPU1;
  if (!src && s == "Secondary TPU") src = TPU2;

  /**/ if (MAIN == src)
    {
      parm_.option_unit (MAIN_BODY);
    }
  else if (ADF == src)
    {
      bool do_duplex = (val_.count ("duplex")
                        && (value (toggle (true)) == val_["duplex"]));

      parm_.option_unit (do_duplex
                         ? ADF_DUPLEX
                         : ADF_SIMPLEX);
    }
  else if (TPU1 == src)
    {
      parm_.option_unit (TPU_AREA_1);
      if (val_.count ("film-type"))
        {
          const string& s = val_["film-type"];
          parm_.film_type (film_type->right.at (s));
        }
    }
  else if (TPU2 == src)
    {
      parm_.option_unit (TPU_AREA_2);
      if (val_.count ("film-type"))
        {
          const string& s = val_["film-type"];
          parm_.film_type (film_type->right.at (s));
        }
    }
  else
    {
      BOOST_THROW_EXCEPTION (logic_error ("unsupported scan source"));
    }
}

void
extended_scanner::set_up_gamma_tables ()
{
  if (val_.count ("gamma"))
    {
      const string& s = val_["gamma"];
      byte value;

      /**/ if (s == "1.0") value = CUSTOM_GAMMA_A;
      else if (s == "1.8") value = CUSTOM_GAMMA_B;
      else BOOST_THROW_EXCEPTION (logic_error ("unsupported gamma value"));

      parm_.gamma_correction (value);

      set_gamma_table lut;
      *cnx_ << lut ();

      return;
    }

  if (!val_.count ("gamma-correction")) return;

  const string& s = val_["gamma-correction"];
  byte value = gamma_correction->right.at (s);
  parm_.gamma_correction (value);

  if (!(CUSTOM_GAMMA_A == value || CUSTOM_GAMMA_B == value)) return;

  // \todo Only send if changed since last time
  set_gamma_table lut;
  *cnx_ << lut ();
}

void
extended_scanner::set_up_image_mode ()
{
  if (!val_.count ("image-type")) return;

  const string& mode = val_["image-type"];
  parm_.color_mode (mode == "Color"
                    ? PIXEL_RGB
                    : MONOCHROME);
  parm_.bit_depth (mode == "Monochrome"
                   ? 1 : 8);
}

void
extended_scanner::set_up_mirroring ()
{
  if (!val_.count ("mirror")) return;

  toggle t = val_["mirror"];
  parm_.mirroring (t);
}

void
extended_scanner::set_up_resolution ()
{
  quantity res = val_["resolution"]; // pixels/inch
  parm_.resolution (res.amount< uint32_t > ());
}

void
extended_scanner::set_up_scan_area ()
{
  quantity tl_x = val_["tl-x"];      // inches
  quantity tl_y = val_["tl-y"];
  quantity br_x = val_["br-x"];
  quantity br_y = val_["br-y"];

  if (br_x < tl_x) std::swap (tl_x, br_x);
  if (br_y < tl_y) std::swap (tl_y, br_y);

  tl_x *= 1. * parm_.resolution ().x ();      // pixels
  tl_y *= 1. * parm_.resolution ().y ();
  br_x *= 1. * parm_.resolution ().x ();
  br_y *= 1. * parm_.resolution ().y ();

  point<uint32_t> tl (tl_x.amount< uint32_t > (), tl_y.amount< uint32_t > ());
  point<uint32_t> br (br_x.amount< uint32_t > (), br_y.amount< uint32_t > ());

  if (uint32_t boundary = get_pixel_alignment ())
    {
      br.x () += boundary - 1;
      br.x () -= (br.x () - tl.x ()) % boundary;
    }

  br.x () = clip_to_physical_scan_area_width (tl.x (), br.x ());
  br.x () = clip_to_max_pixel_width (tl.x (), br.x ());

  parm_.scan_area (tl, br);
}

void
extended_scanner::set_up_scan_count ()
{
}

void
extended_scanner::set_up_scan_speed ()
{
  if (!val_.count ("speed")) return;

  toggle speed = val_["speed"];
  parm_.scan_mode (speed ? HI_SPEED : NORMAL_SPEED);
}

void
extended_scanner::set_up_sharpness ()
{
  if (!val_.count ("sharpness")) return;

  quantity q = val_["sharpness"];
  parm_.sharpness (q.amount< int8_t > ());
}

void
extended_scanner::set_up_threshold ()
{
  if (!val_.count ("threshold")) return;

  quantity q = val_["threshold"];
  parm_.threshold (q.amount< uint8_t > ());
}

void
extended_scanner::set_up_transfer_size ()
{
  if (!val_.count ("line-count")) return;

  quantity lc = val_["line-count"];
  parm_.line_count (lc.amount< uint8_t > ());

  {                             // divine a more optimal line count
    uint32_t bytes_per_line = parm_.scan_area ().width ();

    bytes_per_line *= (parm_.color_mode () == PIXEL_RGB ? 3 : 1);

    if (8 < parm_.bit_depth ())
      {
        bytes_per_line *= 2;
      }
    else
      {
        bytes_per_line /= 8 / parm_.bit_depth ();
      }

    uint8_t lc = parm_.line_count ();
    uint8_t min = std::numeric_limits< uint8_t >::min ();
    uint8_t max = (0 == lc
                   ? std::numeric_limits< uint8_t >::max ()
                   : lc);

    if (buffer_size_ / bytes_per_line < max)
      {
        max = buffer_size_ / bytes_per_line;
      }

    while (min != max)
      {
        uint8_t cur = (1 + min + max) / 2;

        try
          {
            *cnx_ << parm_.line_count (cur);
          }
        catch (const invalid_parameter&)
          {
            max /= 2;
            cur = min;
          }
        min = cur;
      }

    if (lc && lc != parm_.line_count ())
      log::error ("line-count: using %2% instead of %1%")
        % uint32_t (lc)
        % uint32_t (parm_.line_count ())
        ;
  }
}

bool
extended_scanner::validate (const value::map& vm) const
{
  const option::map& om (doc_source_options (vm.at ("doc-source")));

  bool satisfied = true;
  for_each (value::map::value_type p, vm)
    {
      option::map::iterator it = const_cast< option::map& > (om).find (p.first);
      if (const_cast< option::map& > (om).end () != it)
        {
          if (it->constraint ())
            {
              value okay = (*it->constraint ()) (p.second);
              satisfied &= (p.second == okay);
            }
        }
      else
        {
          if (constraints_[p.first])
            {
              value okay = (*constraints_[p.first]) (p.second);
              satisfied &= (p.second == okay);
            }
        }
    }

  std::vector< restriction >::const_iterator rit;
  for (rit = restrictions_.begin (); restrictions_.end () != rit; ++rit)
    {
      satisfied &= (*rit) (vm);
    }

  return satisfied;
}

void
extended_scanner::finalize (const value::map& vm)
{
  value::map final_vm (vm);

  if (vm.at ("doc-source") != *values_["doc-source"])
    {
      option::map& old_opts (doc_source_options (*values_["doc-source"]));
      option::map& new_opts (doc_source_options (vm.at ("doc-source")));

      remove (old_opts, final_vm);
      insert (new_opts, final_vm);
    }

  string scan_area = final_vm["scan-area"];
  if (scan_area != "Manual")
    {
      media size = media (length (), length ());

      /**/ if (scan_area == "Maximum")
        {
          size = media (length (), length ());
        }
      else if (scan_area == "Auto Detect")
        {
          size = probe_media_size_(final_vm["doc-source"]);
        }
      else                      // well-known media size
        {
          size = media::lookup (scan_area);
        }
      update_scan_area_(size, final_vm);
    }

  {                             // minimal scan area check
    quantity tl_x = final_vm["tl-x"];
    quantity tl_y = final_vm["tl-y"];
    quantity br_x = final_vm["br-x"];
    quantity br_y = final_vm["br-y"];

    if (br_x < tl_x) std::swap (tl_x, br_x);
    if (br_y < tl_y) std::swap (tl_y, br_y);

    if (br_x - tl_x < min_area_width_ || br_y - tl_y < min_area_height_)
      BOOST_THROW_EXCEPTION
        (constraint::violation
         ((format (CCB_("Scan area too small.\n"
                        "The area needs to be larger than %1% by %2%."))
           % min_area_width_ % min_area_height_).str ()));
  }

  {                             // finalize resolution options
    boost::optional< toggle > resample;
    {
      if (final_vm.count ("enable-resampling"))
        {
          toggle t = final_vm["enable-resampling"];
          resample = t;
        }
    }

    if (resample)
      {
        // Both may be absent but if one exists, the other does not.
        if (final_vm.count ("sw-resolution"))
          {
            descriptors_["sw-resolution"]->read_only (false);
            descriptors_["sw-resolution"]->active (*resample);
          }

        if (final_vm.count ("resolution"))
          {
            descriptors_["resolution"]->active (!*resample);
          }

        if (*resample)          // update device resolutions
          {
            if (final_vm.count ("resolution"))
              {
                quantity q;

                if (final_vm.count ("sw-resolution"))
                  {
                    q = final_vm["sw-resolution"];
                  }

                BOOST_ASSERT (quantity () != q);

                final_vm["resolution"]
                  = nearest_(q, constraints_["resolution"]);
              }
          }
      }

    if (resample)
      {
        if (!*resample)         // follow device resolutions
          {
            if (final_vm.count ("sw-resolution"))
              {
                if (final_vm.count ("resolution"))
                  {
                    final_vm["sw-resolution"]
                      = nearest_(final_vm["resolution"],
                                 constraints_["sw-resolution"]);
                  }
              }
          }
      }
  }

  option::map::finalize (final_vm);
  relink ();

  // Update best effort estimate for the context at time of scan.
  // While not a *hard* requirement, this does make for a better
  // sane_get_parameters() experience.

  val_ = final_vm;
  set_up_image_mode ();
  set_up_resolution ();
  set_up_scan_area ();

  ctx_ = context (pixel_width (), pixel_height (), pixel_type ());
}

option::map&
extended_scanner::doc_source_options (const value& v)
{
  if (v == value ("Document Table")) return flatbed_;
  if (v == value ("ADF"))     return adf_;

  BOOST_THROW_EXCEPTION
    (logic_error ("internal error: no document source"));
}

const option::map&
extended_scanner::doc_source_options (const value& v) const
{
  return const_cast< extended_scanner& > (*this).doc_source_options (v);
}

void
extended_scanner::configure_doc_source_options ()
{
  store s;

  if (caps_.is_flatbed_type ())         // order dependency
    {
      s.alternative (SEC_N_("Document Table"));
      add_scan_area_options (flatbed_, MAIN);
    }

  if (stat_.adf_detected ())
    {
      s.alternative (SEC_N_("ADF"));
      add_scan_area_options (adf_, ADF);

      if (caps_.adf_is_duplex_type ())
        adf_.add_options ()
          ("duplex", toggle (),
           attributes (tag::general)(level::standard),
           SEC_N_("Duplex")
           );
      if (caps_.is_flatbed_type ()) flatbed_.share_values (adf_);
    }

  // \todo  Rethink area handling and add IR support
  if (stat_.tpu_detected (TPU1)
      || stat_.tpu_detected (TPU2))
    {
      if (stat_.tpu_detected (TPU1)
          && stat_.tpu_detected (TPU2))
        {
          s.alternative (CCB_N_("Primary TPU"));
          s.alternative (CCB_N_("Secondary TPU"));
        }
      else
        {
          s.alternative (SEC_N_("Transparency Unit"));
        }

      tpu_.add_options ()
        ("film-type",
         (film_types ()
          -> default_value (film_type
                            -> left.at (defs_.film_type ()))),
         attributes (tag::enhancement)(level::standard),
         CCB_N_("Film Type")
         );

      if (caps_.is_flatbed_type ()) flatbed_.share_values (tpu_);
      if (stat_.adf_detected ()) adf_.share_values (tpu_);
    }

  add_options ()
    ("doc-source", (from< store > (s)
                    -> default_value (s.front ())
                    ),
     attributes (tag::general)(level::standard),
     SEC_N_("Document Source")
     );
  insert (doc_source_options (s.front ()));
}

void
extended_scanner::add_resolution_options ()
{
  uint32_t max = caps_.max_resolution ();

  if (!max)
    max = std::numeric_limits< uint32_t >::max ();

  constraint::ptr cp;

  if (   caps_.product_name () == "GT-S650"
      || caps_.product_name () == "Perfection V19"
      || caps_.product_name () == "Perfection V39")
  {
    constraint::ptr r (from< store > ()
                       -> alternative (300)
                       -> alternative (600)
                       -> alternative (1200)
                       -> alternative (2400)
                       -> alternative (4800)
                       -> default_value (300));
    const_cast< constraint::ptr& > (cp) = r;
  }
  else
  {
    constraint::ptr r (from< range > ()
                       -> bounds (int_cast (caps_.min_resolution ()),
                                  int_cast (caps_.max_resolution ()))
                       -> default_value (quantity (int_cast (defs_.resolution ().x ()))));
    const_cast< constraint::ptr& > (cp) = r;
  }

  if (cp)
  {
    add_options ()
      ("resolution", cp,
       attributes (tag::general)(level::standard),
       SEC_N_("Resolution")
        );
  }
  else
  {
    log::brief ("no hardware resolution options");
  }

  if (res_)
    {
      // repeat the above for software-emulated resolution options

      add_options ()
        ("enable-resampling", toggle (true),
         attributes (tag::general),
         SEC_N_("Enable Resampling"),
         CCB_N_("This option provides the user with a wider range of supported"
                " resolutions.  Resolutions not supported by the hardware will"
                " be achieved through image processing methods.")
          );
      add_options ()
        ("sw-resolution", res_,
         attributes (tag::general)(level::standard).emulate (true),
         SEC_N_("Resolution")
          );
    }
  else
    {
      log::brief ("no software resolution options");
    }
}

void
extended_scanner::add_scan_area_options (option::map& opts,
                                         const source_value& src)
{
  // bbox = (caps_.scan_area (src) / caps_.base_resolution ());
  point<quantity> bbox_tl (1. * caps_.scan_area (src).top_left ().x (),
                           1. * caps_.scan_area (src).top_left ().y ());
  point<quantity> bbox_br (1. * caps_.scan_area (src).bottom_right ().x (),
                           1. * caps_.scan_area (src).bottom_right ().y ());
  bounding_box<quantity> bbox (bbox_tl / (1. * caps_.base_resolution ()),
                               bbox_br / (1. * caps_.base_resolution ()));

  std::list< std::string > areas = media::within (0, 0,
                                                  bbox.width (),
                                                  bbox.height ());
  areas.push_back (SEC_N_("Manual"));
  areas.push_back (SEC_N_("Maximum"));
  if (stat_.supports_size_detection (src))
    areas.push_back (SEC_N_("Auto Detect"));

  opts.add_options ()
    ("scan-area", (from< utsushi::store > ()
                   -> alternatives (areas.begin (), areas.end ())
                   -> default_value ("Manual")),
     attributes (tag::general)(level::standard),
     SEC_N_("Scan Area")
     )
    ("tl-x", (from< range > ()
              -> offset (bbox.offset ().x ())
              -> extent (bbox.width ())
              -> default_value (bbox.top_left ().x ())
              ),
     attributes (tag::geometry)(level::standard),
     SEC_N_("Top Left X")
     )
    ("br-x", (from< range > ()
              -> offset (0.1 + bbox.offset ().x ())
              -> extent (bbox.width ())
              -> default_value (bbox.bottom_right ().x ())
              ),
     attributes (tag::geometry)(level::standard),
     SEC_N_("Bottom Right X")
     )
    ("tl-y", (from< range > ()
              -> offset (bbox.offset ().y ())
              -> extent (bbox.height ())
              -> default_value (bbox.top_left ().y ())
              ),
     attributes (tag::geometry)(level::standard),
     SEC_N_("Top Left Y")
     )
    ("br-y", (from< range > ()
              -> offset (0.1 + bbox.offset ().y ())
              -> extent (bbox.height ())
              -> default_value (bbox.bottom_right ().y ())
              ),
     attributes (tag::geometry)(level::standard),
     SEC_N_("Bottom Right Y")
     );
}

//! \todo Make repeat_count configurable
media
extended_scanner::probe_media_size_(const string& doc_source)
{
  source_value src = NO_SOURCE;
  media size = media (length (), length ());

  /**/ if (doc_source == "Document Table") src = MAIN;
  else if (doc_source == "ADF")     src = ADF;
  else
    {
      log::error
        ("media size probing for %1% not implemented")
        % doc_source;
      return size;
    }

  int repeat_count = 5;
  do
    {
      *cnx_ << stat_;
    }
  while (!stat_.media_size_detected (src)
         && delay_elapsed ()
         && --repeat_count);

  if (stat_.media_size_detected (src))
    {
      size = stat_.media_size (src);
    }
  else
    {
      log::error ("unable to determine media size in allotted time");
    }

  return size;
}

void
extended_scanner::update_scan_area_(const media& size, value::map& vm) const
{
  if (size.width () > 0 && size.height () > 0)
    {
      quantity tl_x (0.0);
      quantity tl_y (0.0);
      quantity br_x = size.width ();
      quantity br_y = size.height ();

      align_document (vm["doc-source"],
                      tl_x, tl_y, br_x, br_y);

      vm["tl-x"] = tl_x;
      vm["tl-y"] = tl_y;
      vm["br-x"] = br_x;
      vm["br-y"] = br_y;
    }
  else
    {
      log::brief ("using default scan-area");
      // This relies on default values being set to lower() values for
      // tl-x and tl-y and upper() values for br-x and br-y.
      // Note that alignment is irrelevant for the maximum size.
      vm["tl-x"] = constraints_["tl-x"]->default_value ();
      vm["tl-y"] = constraints_["tl-y"]->default_value ();
      vm["br-x"] = constraints_["br-x"]->default_value ();
      vm["br-y"] = constraints_["br-y"]->default_value ();
    }
}

void
extended_scanner::align_document (const string& doc_source,
                                  quantity& tl_x, quantity& tl_y,
                                  quantity& br_x, quantity& br_y) const
{
  if (doc_source != "ADF") return;

  byte align = caps_.document_alignment ();
  quantity max_width  (dynamic_cast< range * > (constraints_["br-x"].get ())
                       ->upper ());
  quantity max_height (dynamic_cast< range * > (constraints_["br-y"].get ())
                       ->upper ());

  if (max_width  == 0)          // nothing we can do
    return;
  if (max_height == 0)          // nothing we can do
    return;

  quantity width (br_x - tl_x);
  quantity x_shift;             // for ALIGNMENT_UNKNOWN assume 0
  quantity y_shift;             // no specification, assume 0

  if (ALIGNMENT_LEFT   == align) x_shift =  0.0;
  if (ALIGNMENT_CENTER == align) x_shift = (max_width - width) / 2;
  if (ALIGNMENT_RIGHT  == align) x_shift =  max_width - width;

  tl_x += x_shift;
  tl_y += y_shift;
  br_x += x_shift;
  br_y += y_shift;
}

uint32_t
extended_scanner::get_pixel_alignment ()
{
  uint32_t boundary = 0;

  if (4 >= parm_.bit_depth ())
    boundary = 8;

  if (caps_.product_name () == "ES-H300")
    {
      boundary = (1 == parm_.bit_depth ()
                  ? 32
                  : 4);
    }

  return boundary;
}

uint32_t
extended_scanner::clip_to_physical_scan_area_width (uint32_t tl_x,
                                                    uint32_t br_x)
{
  uint32_t rv (br_x);
  uint32_t scan_area_width (caps_.scan_area ().width ()
                            * parm_.resolution ().x ()
                            / caps_.base_resolution ());

  if (br_x > scan_area_width)
    {
      rv = scan_area_width;

      if (uint32_t boundary = get_pixel_alignment ())
        rv -= (scan_area_width - tl_x) % boundary;
    }

  return rv;
}

uint32_t
extended_scanner::clip_to_max_pixel_width (uint32_t tl_x, uint32_t br_x)
{
  uint32_t rv (br_x);

  if ((br_x - tl_x) > caps_.max_scan_width ())
    {
      log::error ("maximum pixel width exceeded, clipping from %1% to %2%")
        % (br_x - tl_x) % caps_.max_scan_width ();

      rv = tl_x + caps_.max_scan_width ();

      if (uint32_t boundary = get_pixel_alignment ())
        rv -= caps_.max_scan_width () % boundary;
    }

  return rv;
}

void
extended_scanner::configure_color_correction ()
{
  if (caps_.command_level ().at (0) != 'D')
    return;

  matrix< double, 3 > mat;

  mat[0][0] =  1.0782;  mat[0][1] =  0.0135;  mat[0][2] = -0.0917;
  mat[1][0] =  0.0206;  mat[1][1] =  1.0983;  mat[1][2] = -0.1189;
  mat[2][0] =  0.0113;  mat[2][1] = -0.1485;  mat[2][2] =  1.1372;

  static const matrix< double, 3 > profile_matrix_1 = mat;

  mat[0][0] =  1.0567;  mat[0][1] =  0.0415;  mat[0][2] = -0.0982;
  mat[1][0] =  0.0289;  mat[1][1] =  1.1112;  mat[1][2] = -0.1401;
  mat[2][0] =  0.0193;  mat[2][1] = -0.2250;  mat[2][2] =  1.2057;

  static const matrix< double, 3 > profile_matrix_2 = mat;

  mat[0][0] =  0.9803;  mat[0][1] =  0.0341;  mat[0][2] = -0.0144;
  mat[1][0] =  0.0080;  mat[1][1] =  1.0308;  mat[1][2] = -0.0388;
  mat[2][0] =  0.0112;  mat[2][1] = -0.1296;  mat[2][2] =  1.1184;

  static const matrix< double, 3 > profile_matrix_3 = mat;

  mat[0][0] =  1.0027;  mat[0][1] =  0.0005;  mat[0][2] = -0.0032;
  mat[1][0] =  0.0044;  mat[1][1] =  1.0214;  mat[1][2] = -0.0258;
  mat[2][0] =  0.0048;  mat[2][1] = -0.0624;  mat[2][2] =  1.0576;

  static const matrix< double, 3 > profile_matrix_4 = mat;

  mat[0][0] =  1.0824;  mat[0][1] =  0.0085;  mat[0][2] = -0.0909;
  mat[1][0] =  0.0339;  mat[1][1] =  1.1043;  mat[1][2] = -0.1382;
  mat[2][0] =  0.0087;  mat[2][1] = -0.1557;  mat[2][2] =  1.1470;

  static const matrix< double, 3 > profile_matrix_5 = mat;

  mat[0][0] =  0.9864;  mat[0][1] =  0.0248;  mat[0][2] = -0.0112;
  mat[1][0] =  0.0021;  mat[1][1] =  1.0100;  mat[1][2] = -0.0121;
  mat[2][0] =  0.0139;  mat[2][1] = -0.1249;  mat[2][2] =  1.1110;

  static const matrix< double, 3 > profile_matrix_6 = mat;

  static const std::map< std::string, const matrix< double, 3 > >
    profile_matrix = boost::assign::map_list_of
    ("PID 08C0", profile_matrix_1)
    ("PID 08C2", profile_matrix_1)
    ("PID 08D1", profile_matrix_1)
    ("PID 08D2", profile_matrix_1)
    ("PID 08D3", profile_matrix_1)
    ("PID 1101", profile_matrix_1)
    ("PID 1102", profile_matrix_1)
    ("PID 1103", profile_matrix_1)
    ("PID 1104", profile_matrix_1)
    ("PID 1105", profile_matrix_1)
    ("PID 1106", profile_matrix_1)
    ("PID 1107", profile_matrix_1)
    ("PID 110D", profile_matrix_1)
    ("PID 110F", profile_matrix_1)
    ("PID 111C", profile_matrix_1)
    //
    ("PID 08CD", profile_matrix_2)
    ("PID 1108", profile_matrix_2)
    ("PID 1109", profile_matrix_2)
    ("PID 110A", profile_matrix_2)
    ("PID 110B", profile_matrix_2)
    ("PID 110C", profile_matrix_2)
    //
    ("PID 1113", profile_matrix_3)
    ("PID 1117", profile_matrix_3)
    ("PID 1119", profile_matrix_3)
    ("PID 111A", profile_matrix_3)
    //
    ("PID 1114", profile_matrix_4)
    ("PID 1115", profile_matrix_4)
    ("PID 1116", profile_matrix_4)
    ("PID 1118", profile_matrix_4)
    ("PID 111D", profile_matrix_4)
    ("PID 111E", profile_matrix_4)
    ("PID 111F", profile_matrix_4)
    ("PID 1120", profile_matrix_4)
    ("PID 1121", profile_matrix_4)
    ("PID 1122", profile_matrix_4)
    ("PID 113D", profile_matrix_4)
    ("PID 113E", profile_matrix_4)
    ("PID 113F", profile_matrix_4)
    //
    ("GT-S650",        profile_matrix_5)
    ("Perfection V19", profile_matrix_5)
    ("Perfection V39", profile_matrix_5)
    //
    ("PID 1142", profile_matrix_6)
    ("PID 1143", profile_matrix_6)
    ;

  try {
    mat = profile_matrix.at (caps_.product_name ());
    add_options ()
      ("cct-1", quantity (mat[0][0]))
      ("cct-2", quantity (mat[0][1]))
      ("cct-3", quantity (mat[0][2]))
      ("cct-4", quantity (mat[1][0]))
      ("cct-5", quantity (mat[1][1]))
      ("cct-6", quantity (mat[1][2]))
      ("cct-7", quantity (mat[2][0]))
      ("cct-8", quantity (mat[2][1]))
      ("cct-9", quantity (mat[2][2]))
      ("sw-color-correction", toggle (true))
      ;
  }
  catch (const std::out_of_range&) {}
}

context::size_type
extended_scanner::pixel_width() const
{
  return parm_.scan_area ().width ();
}

context::size_type
extended_scanner::pixel_height() const
{
  return parm_.scan_area ().height ();
}

context::_pxl_type_
extended_scanner::pixel_type() const
{
  /**/ if ( 1 == parm_.bit_depth ())
    {
      if (   MONOCHROME == parm_.color_mode ()
          || DROPOUT_R  == parm_.color_mode ()
          || DROPOUT_G  == parm_.color_mode ()
          || DROPOUT_B  == parm_.color_mode ())
        {
          return context::MONO;
        }
    }
  else if ( 8 == parm_.bit_depth ())
    {
      if (PIXEL_RGB == parm_.color_mode ())
        {
          return context::RGB8;
        }
      if (   MONOCHROME == parm_.color_mode ()
          || DROPOUT_R  == parm_.color_mode ()
          || DROPOUT_G  == parm_.color_mode ()
          || DROPOUT_B  == parm_.color_mode ())
        {
          return context::GRAY8;
        }
    }
  else if (16 == parm_.bit_depth ())
    {
      if (PIXEL_RGB == parm_.color_mode ())
        {
          return context::RGB16;
        }
      if (   MONOCHROME == parm_.color_mode ()
          || DROPOUT_R  == parm_.color_mode ()
          || DROPOUT_G  == parm_.color_mode ()
          || DROPOUT_B  == parm_.color_mode ())
        {
          return context::GRAY16;
        }
    }

  return context::unknown_type;
}

void
extended_scanner::lock_scanner ()
{
  if (locked_)
    {
      log::alert ("scanner is already locked");
      return;
    }

  try
    {
      capture_scanner lock;
      *cnx_ << lock;
      locked_ = true;
    }
  catch (const invalid_command& e)
    {}
  catch (const unknown_reply& e)
    {
      log::alert (e.what ());
    }
}

void
extended_scanner::unlock_scanner ()
{
  if (!locked_)
    {
      log::alert ("scanner is not locked yet");
      return;
    }

  try
    {
      release_scanner unlock;
      *cnx_ << unlock;
      locked_ = false;
    }
  catch (const invalid_command& e)
    {}
  catch (const unknown_reply& e)
    {
      log::alert (e.what ());
    }
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
