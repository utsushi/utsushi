//  bounding-box.hpp -- class template
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_bounding_box_hpp_
#define drivers_esci_bounding_box_hpp_

#include "point.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//!  Bounding boxes
/*!  A bounding box is a rectangular area aligned with the image
 *   coordinate system's axes.  It circumscribes the image area of
 *   interest.
 */
template< typename T >
class bounding_box
  : boost::equality_comparable< bounding_box< T > >
{
  point< T > tl_;               //!<  top left corner
  point< T > br_;               //!<  bottom right corner

public:

  //!  Creates a bounding box encompassing the vector from \a p1 to \a p2
  bounding_box (const point< T >& p1, const point< T >& p2)
  {
    tl_ = top_left     (p1, p2);
    br_ = bottom_right (p1, p2);
  }

  //!  Creates a bounding box encompassing the vector to \a p
  bounding_box (const point< T >& p)
  {
    tl_ = top_left     (point< T > (), p);
    br_ = bottom_right (point< T > (), p);
  }

  bool operator== (const bounding_box& b) const
  {
    return (this->tl_ == b.tl_ && this->br_ == b.br_);
  }

  //!  Distance from the left edge to the right edge
  /*!  The return value is guaranteed to be non-negative.
   */
  T width () const
  {
    return br_.x () - tl_.x ();
  }

  //!  Distance from the top edge to the bottom edge
  /*!  The return value is guaranteed to be non-negative.
   */
  T height () const
  {
    return br_.y () - tl_.y ();
  }

  //!  Vector to the top left point of the bounding box
  /*!  This is an alias of top_left() that goes with extent().
   */
  point< T > offset () const
  {
    return tl_;
  }

  //!  Vector from the offset to the bottom right point of the bounding box
  /*!  The x() and y() components are guaranteed to be non-negative.
   *
   *   \sa offset, bottom_right
   */
  point< T > extent () const
  {
    return br_ - tl_;
  }

  //!  Point where the top and left edges meet
  point< T > top_left () const
  {
    return tl_;
  }

  //!  Point where the top and right edges meet
  point< T > top_right () const
  {
    return point< T > (br_.x (), tl_.y ());
  }

  //!  Point where the bottom and left edges meet
  point< T > bottom_left () const
  {
    return point< T > (tl_.x (), br_.y ());
  }

  //!  Point where the bottom and right edges meet
  point< T > bottom_right () const
  {
    return br_;
  }

  static point< T > top_left (const point< T >& p1, const point< T >& p2)
  {
    return point< T > (std::min< T > (p1.x (), p2.x ()),
                       std::min< T > (p1.y (), p2.y ()));
  }

  static point< T > top_right (const point< T >& p1, const point< T >& p2)
  {
    return point< T > (std::min< T > (p1.x (), p2.x ()),
                       std::max< T > (p1.y (), p2.y ()));
  }

  static point< T > bottom_left (const point< T >& p1, const point< T >& p2)
  {
    return point< T > (std::max< T > (p1.x (), p2.x ()),
                       std::min< T > (p1.y (), p2.y ()));
  }

  static point< T > bottom_right (const point< T >& p1, const point< T >& p2)
  {
    return point< T > (std::max< T > (p1.x (), p2.x ()),
                       std::max< T > (p1.y (), p2.y ()));
  }
};

} // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_bounding_box_hpp_ */
