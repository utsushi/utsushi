//  deskew.cpp -- reorient images to become level
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

#include "deskew.hpp"

#include <utsushi/range.hpp>
#include <utsushi/run-time.hpp>

#include <boost/lexical_cast.hpp>

namespace utsushi {
namespace _flt_ {

using boost::lexical_cast;

const streamsize PNM_HEADER_SIZE = 50;

deskew::deskew ()
  : shell_pipe (run_time ().exec_file (run_time::pkg, "doc-locate"))
{
  option_->add_options ()
    ("lo-threshold", (from< range > ()  // percentage
                      -> lower (  0.0)
                      -> upper (100.0)
                      -> default_value (45.0)))
    ("hi-threshold", (from< range > ()  // percentage
                      -> lower (  0.0)
                      -> upper (100.0)
                      -> default_value (55.0)))
    ;
  freeze_options ();   // initializes option tracking member variables
}

void
deskew::freeze_options ()
{
  quantity threshold;

  threshold = value ((*option_)["lo-threshold"]);
  lo_threshold_ = threshold.amount< double > ();
  threshold = value ((*option_)["hi-threshold"]);
  hi_threshold_ = threshold.amount< double > ();
}

std::string
deskew::arguments (const context& ctx)
{
  using std::string;

  string argv;

  // Set up input data characteristics

  argv += " " + lexical_cast< string > (lo_threshold_ / 100);
  argv += " " + lexical_cast< string > (hi_threshold_ / 100);
  argv += " deskew";
  argv += " " + lexical_cast< string > (ctx.octets_per_image ()
                                        + PNM_HEADER_SIZE);

  return argv;
}

}       // namespace _flt_
}       // namespace utsushi
