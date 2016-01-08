//  image-skip.hpp -- conditionally suppress images in the output
//  Copyright (C) 2013-2015  SEIKO EPSON CORPORATION
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

#ifndef filters_image_skip_hpp_
#define filters_image_skip_hpp_

#include <utsushi/filter.hpp>
#include <deque>

namespace utsushi {
namespace _flt_ {

struct bucket;

//! Make selected images disappear
/*! When acquiring a large number of images it is often desirable to
 *  remove the "uninteresting" ones.  The definition of uninteresting
 *  may vary by use case but the general mechanism is the same.  This
 *  filter can be used to suppress blank images.
 *
 *  \note The current implementation only works with unpadded raster
 *        images and computes a measure of relative "darkness".  The
 *        image is removed from the output unless that "darkness"
 *        exceeds a configurable threshold.
 *
 *  \todo Generalize the algorithm to allow for setting of an area of
 *        interest (so you can for example ignore the shaded edges of
 *        a document) and easier detection of localized artifacts
 *        (such as page headers, footers and numbers) which tend to be
 *        smothered by the brightness of the remaining white space.
 *        The latter can be achieved by tiling the area of interest
 *        and applying the measure on each tile (rather than a single
 *        tile as done at the moment).
 *  \todo Make the algorithm configurable.
 *  \todo Add an algorithm based on the variance in luminance?
 */
class image_skip
  : public filter
{
public:
  image_skip ();

  streamsize write (const octet *data, streamsize n);

  void mark (traits::int_type c, const context& ctx);

protected:
  void bos (const context& ctx);
  void boi (const context& ctx);
  void eoi (const context& ctx);
  void eos (const context& ctx);
  void eof (const context& ctx);

private:
  bool skip_();
  void process_(shared_ptr< bucket > b);

  double threshold_;
  double darkness_;

  std::deque< shared_ptr< bucket > > pool_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_image_skip_hpp_ */
