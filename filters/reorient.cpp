//  reorient.cpp -- images to make text face the right way up
//  Copyright (C) 2015  SEIKO EPSON CORPORATION
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

//  Guard against preprocessor symbol confusion.  We only care about the
//  command line utilities here, not about any C++ API.
#ifdef HAVE_GRAPHICS_MAGICK_PP
#undef HAVE_GRAPHICS_MAGICK_PP
#endif
#ifdef HAVE_IMAGE_MAGICK_PP
#undef HAVE_IMAGE_MAGICK_PP
#endif

//  Guard against possible mixups by preferring GraphicsMagick in case
//  both are available.
#if HAVE_GRAPHICS_MAGICK
#if HAVE_IMAGE_MAGICK
#undef  HAVE_IMAGE_MAGICK
#define HAVE_IMAGE_MAGICK 0
#endif
#endif

#include "reorient.hpp"

#include <utsushi/format.hpp>
#include <utsushi/i18n.hpp>
#include <utsushi/log.hpp>
#include <utsushi/regex.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/store.hpp>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <stdexcept>

#include <stdio.h>              /* popen() is not in <cstdio> */
#include <string.h>             /* strverscmp() is not in <cstring> */

namespace utsushi {
namespace _flt_ {

namespace fs = boost::filesystem;

using std::logic_error;

struct bucket
{
  octet     *data_;
  streamsize size_;
  bool       seen_;

  bucket (const octet *data, streamsize size)
    : data_(new octet[size])
    , size_(size)
    , seen_(false)
  {
    traits::copy (data_, data, size);
  }

  ~bucket ()
  {
    delete [] data_;
  }
};

inline void
chomp (char *str)
{
  if (!str) return;
  char *nl = strrchr (str, '\n');
  if (nl) *nl = '\0';
}

bool
tesseract_version_before_(const char *cutoff)
{
  FILE *fp = popen ("tesseract --version 2>&1"
                    "| awk '/^tesseract/{ print $2 }'", "r");
  int errc = errno;

  if (fp)
    {
      char  buf[80];
      char *result = fgets (buf, sizeof (buf), fp);

      pclose (fp);
      chomp (result);

      if (result)
        {
          log::debug ("found tesseract-%1%") % result;

          return (0 > strverscmp (result, cutoff));
        }
    }

  if (errc)
    log::alert ("failure checking tesseract version: %1%")
      % strerror (errc);

  return false;
}

bool
have_tesseract_language_pack_(const char *lang)
{
  std::string cmd("tesseract --list-langs 2>&1"
                  "| sed -n '/^");
  cmd += lang;
  cmd += "$/p'";

  FILE *fp = popen (cmd.c_str (), "r");
  int errc = errno;

  if (fp)
    {
      char  buf[80];
      char *result = fgets (buf, sizeof (buf), fp);

      pclose (fp);
      chomp (result);

      if (result)
        {
          log::debug ("found tesseract %1% language pack") % result;
          return 0 == strcmp (result, lang);
        }
    }

  if (errc)
    log::alert ("failure checking for tesseract language pack: %1%")
      % strerror (errc);

  return false;
}

bool
have_tesseract_()
{
  static int found = -1;

  if (-1 == found)
    {
      found = (!tesseract_version_before_("3.03")
               && have_tesseract_language_pack_("osd"));
      found = (found ? 1 : 0);
    }
  return found;
}

static std::string abs_path_name = std::string ();

bool
have_ocr_engine_()
{
  // The get-text-orientation shell script requires the presence of a
  // *Magick convert utility in order to use the OCR Engine.  If not
  // found at configure time, we disable use of the OCR Engine.

  static int found = (HAVE_MAGICK ? -1 : 0);

  static const std::string type ("ocr-engine-getrotate");

  if (-1 != found) return found;

  // \todo  The code below is a near verbatim copy of similar code in
  //        utsushi::ipc::connexion.  Ideally, utsushi::run_time would
  //        provide API for these kind of "utility" searches.

  run_time rt;

  /*! \todo Work out search path logic.
   *
   *  By default, installed services ought to live in PKGLIBEXECDIR.
   *  During development they can be anywhere in the file system as
   *  most services will probably be developed as out-of-tree, third
   *  party modules.  For unit testing purposes one may also have a
   *  module that is in-tree and lives in the current directory.
   *
   *  For now, we are not interested in supporting services all over
   *  the place yet would like to be able to run in place.
   */
  if (!rt.running_in_place ())  // installed version
    {
      abs_path_name = (fs::path (PKGLIBEXECDIR) / type).string ();
    }
  else                          // development, run-in-place setup
    {
      /*! \todo We may need to cater to several services installed in
       *        different locations during a single run so search path
       *        support is warranted.
       */
      const char *path = getenv (PACKAGE_ENV_VAR_PREFIX "LIBEXECDIR");

      if (!path) path = ".";

      abs_path_name = (fs::path (path) / type).string ();
    }

  if (abs_path_name.empty ())
    {
      found = 0;
      return found;
    }

  // Not sure if this is really necessary.  The access(2) manual page
  // does not encourage the use of the function as there is a window
  // between the check of a file and the file's use.

  if (0 != access (abs_path_name.c_str (), F_OK | X_OK))
    {
      fs::path path (fs::path (PKGLIBEXECDIR)
                     .remove_filename () // == PACKAGE_TARNAME
                     .remove_filename () // host-system triplet?
                     );
      if (   path.filename () == "lib"
          || path.filename () == "lib64"
          || path.filename () == "libexec")
        {
          path /= PACKAGE_TARNAME;
          abs_path_name = (path / type).string ();
        }

      if (0 != access (abs_path_name.c_str (), F_OK | X_OK))
        {
          found = 0;
          return found;
        }
    }

  log::brief ("found %1% as %2%") % type % abs_path_name;
  found = 1;
  return found;
}

const value deg_000 = SEC_N_(  "0 degrees");
const value deg_090 = SEC_N_( "90 degrees");
const value deg_180 = SEC_N_("180 degrees");
const value deg_270 = SEC_N_("270 degrees");
const value automatic = SEC_N_("Auto");

reorient::reorient ()
  : shell_pipe (run_time ().exec_file (run_time::pkg, "get-text-orientation"))
{
  static int found = -1;

  if (-1 == found)
    {
      found = (have_tesseract_()
               || have_ocr_engine_());
      found = (found ? 1 : 0);
    }

  store s;
  s.alternative (deg_000);
  s.alternative (deg_090);
  s.alternative (deg_180);
  s.alternative (deg_270);
  if (found)
    s.alternative (automatic);

  option_->add_options ()
    ("rotate", (from< store > (s)
                -> default_value (s.front ())
                ),
     attributes (tag::enhancement)(level::standard),
     SEC_N_("Rotate")
     );

  if (found)
    {
      if (have_tesseract_())  engine_ = "tesseract";
      if (have_ocr_engine_()) engine_ = abs_path_name;
    }
  freeze_options ();   // initializes option tracking member variables
}

void
reorient::mark (traits::int_type c, const context& ctx)
{
  ctx_ = ctx;
  output::mark (c, ctx_);       // calls our marker handlers
}

streamsize
reorient::write (const octet *data, streamsize n)
{
  if (automatic != reorient_)
    {
      return output_->write (data, n);
    }

  streamsize rv = base::write (data, n);

  if (0 < rv)
    pool_.push_back (make_shared< bucket > (data, rv));

  return rv;
}

void
reorient::bos (const context& ctx)
{
  base::bos (ctx);

  output_->mark (last_marker_, ctx_);
  signal_marker_(last_marker_);
}

void
reorient::boi (const context& ctx)
{
  if (automatic != reorient_)
    {
      ctx_ = estimate (ctx);
      last_marker_ = traits::boi ();
      output_->mark (last_marker_, ctx_);
      signal_marker_(last_marker_);
      return;
    }

  BOOST_ASSERT (pool_.empty ());
  report_.clear ();

  base::boi (ctx);              // starts get-text-orientation process

  // suppress marking on the output_ until we have had a chance to
  // analyze the incoming image
}

void
reorient::eoi (const context& ctx)
{
  if (automatic != reorient_)
    {
      ctx_ = finalize (ctx);
      last_marker_ = traits::eoi ();
      output_->mark (last_marker_, ctx_);
      signal_marker_(last_marker_);
      return;
    }

  base::eoi (ctx);

  // ctx_ now has a best effort estimate for the image's orientation

  last_marker_ = traits::boi ();
  output_->mark (last_marker_, ctx_);
  signal_marker_(last_marker_);

  while (!pool_.empty ())
    {
      shared_ptr< bucket > p = pool_.front ();
      pool_.pop_front ();
      if (p)
        output_->write (p->data_, p->size_);
    }

  last_marker_ = traits::eoi ();
  output_->mark (last_marker_, ctx_);
  signal_marker_(last_marker_);
}

void
reorient::eos (const context& ctx)
{
  base::eos (ctx);
  output_->mark (last_marker_, ctx_);
  signal_marker_(last_marker_);
}

void
reorient::eof (const context& ctx)
{
  if (automatic == reorient_)
    {
      base::eof (ctx);
      pool_.clear ();
    }
  else
    {
      ctx_ = finalize (ctx);
    }

  last_marker_ = traits::eof ();
  output_->mark (last_marker_, ctx);
  signal_marker_(last_marker_);
}

void
reorient::freeze_options ()
{
  reorient_ = (*option_)["rotate"];
}

context
reorient::estimate (const context& ctx)
{
  context rv (ctx);

  if (automatic == reorient_) return rv;

  switch (ctx.direction ())
    {
    case context::bottom_to_top:
      // Add an extra 180 degrees to make images appear as if scanned
      // top-to-bottom.
      /**/ if (deg_000 == reorient_) rv.orientation (context::bottom_right);
      else if (deg_090 == reorient_) rv.orientation (context::left_bottom);
      else if (deg_180 == reorient_) rv.orientation (context::top_left);
      else if (deg_270 == reorient_) rv.orientation (context::right_top);
      else
        BOOST_THROW_EXCEPTION
          (logic_error
           ((format ("unsupported rotation angle: '%1%'") % reorient_).str ()));
      break;
    default:
      log::alert ("assuming top-to-bottom scan direction");
      // and fall through
    case context::top_to_bottom:
      /**/ if (deg_000 == reorient_) rv.orientation (context::top_left);
      else if (deg_090 == reorient_) rv.orientation (context::right_top);
      else if (deg_180 == reorient_) rv.orientation (context::bottom_right);
      else if (deg_270 == reorient_) rv.orientation (context::left_bottom);
      else
        BOOST_THROW_EXCEPTION
          (logic_error
           ((format ("unsupported rotation angle: '%1%'") % reorient_).str ()));
    }

  return rv;
}

context
reorient::finalize (const context& ctx)
{
  if (automatic != reorient_) return estimate (ctx);

  std::stringstream ss (report_);
  std::string line;
  const regex re ("Orientation in degrees: ([0-9]*)");
  smatch      m;

  while (!std::getline (ss, line).eof ()
         && !regex_match (line, m, re))
    {
      // condition does all the processing already
    }

  context rv (ctx);

  if (!m.empty ())
    {
      int degrees = boost::lexical_cast< int > (m.str(1));

      /**/ if (  0 == degrees) rv.orientation (context::top_left);
      else if ( 90 == degrees) rv.orientation (context::right_top);
      else if (180 == degrees) rv.orientation (context::bottom_right);
      else if (270 == degrees) rv.orientation (context::left_bottom);
      else
        log::alert
          (format ("unexpected document orientation: %1% degrees")
           % degrees);
    }
  return rv;
}

std::string
reorient::arguments (const context& ctx)
{
  BOOST_ASSERT (automatic == reorient_);

  return engine_ + " '" MAGICK_CONVERT "'";
}

void
reorient::checked_write (octet *data, streamsize n)
{
  BOOST_ASSERT (0 < n);

  report_.append (data, n);
}

}       // namespace _flt_
}       // namespace utsushi
