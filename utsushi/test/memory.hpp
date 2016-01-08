//  memory.hpp -- based input devices
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

#ifndef utsushi_test_memory_hpp_
#define utsushi_test_memory_hpp_

#include "../device.hpp"
#include "../filter.hpp"

#include <boost/throw_exception.hpp>

#include <fstream>
#include <stdexcept>

namespace utsushi {

using std::domain_error;

typedef std::basic_filebuf<octet> file;

//!  Devices that care next to nothing about the image data they produce
/*!  A number of test scenarios is primarily concerned with the flow
 *   of image data and do not care about image content a great deal.
 *   This class is well-suited for tests that only care about the fact
 *   there is image data.  It only does the absolute minimal amount of
 *   work needed to mimick the real thing.
 *
 *   \warning This device does \e not initialize image data.  As such
 *            it may trigger spurious warnings and/or errors when used
 *            with memory debugging tools such as \c valgrind.
 *            If that is problematic, use setmem_idevice instead.
 */
class rawmem_idevice : public idevice
{
  const unsigned image_count_;

  streamsize octets_left_;
  unsigned   images_left_;

protected:
  bool is_consecutive () const
  { return 1 < image_count_; }
  bool obtain_media ()
  { return 0 < image_count_   && 0 != ctx_.octets_per_image (); }
  bool set_up_image ()
  { return 0 < images_left_-- && 0 != octets_left_; }
  void finish_image ()
  { if (0 < images_left_) octets_left_ = ctx_.octets_per_image (); }
  streamsize sgetn (octet *data, streamsize n)
  {
    streamsize rv = n;
    if (0 <= octets_left_)      // finite image
      {
        rv = std::min (octets_left_, n);
        octets_left_ -= rv;
      }
    return rv;
  }

public:
  //!  Creates a raw memory image data producer
  /*!  A grand total of \a image_count images will be created, each of
   *   which is made up of \a octet_count octets.
   *
   *   A negative \a octet_count results in an never-ending image.
   */
  rawmem_idevice (streamsize octet_count = -1, unsigned image_count = 1)
    : idevice (context (1, octet_count, context::GRAY8))
    , image_count_(image_count)
  {
    reset ();
  }

  rawmem_idevice (const context& ctx, unsigned image_count = 1)
    : idevice (ctx), image_count_(image_count)
  {
    if (context::unknown_size == ctx_.width ()) {
      BOOST_THROW_EXCEPTION (domain_error ("cannot handle unknown sizes"));
    }
    reset ();
  }

  //!  Resets the object to the same state as after construction
  void reset ()
  {
    last_marker_ = traits::eos();
    octets_left_ = ctx_.octets_per_image ();
    images_left_ = image_count_;
  }
};

//!  Devices that produce controlled octet sequences
/*!  This class is mostly meant for tests that not only care about
 *   there being image data at all but also need some control about
 *   what that data looks like as well.
 */
class setmem_idevice : public rawmem_idevice
{
public:
  //!  Create infinitely long octet sequences
  struct generator
  {
    virtual ~generator () {}
    //!  Produces exactly \a n octets of image \a data
    virtual void operator() (octet *data, streamsize n) = 0;
  };

private:
  shared_ptr<generator> generator_;

public:
  //!  Creates a producer of controlled image data
  /*!  A grand total of \a image_count images will be created, each of
   *   which is made up of \a octet_count octets.  The octets will be
   *   initialised with the help of the \a generator.
   *
   *   A negative \a octet_count results in an never-ending image.
   */
  setmem_idevice (shared_ptr<generator> generator,
                  streamsize octet_count = -1, unsigned image_count = 1)
    : rawmem_idevice (octet_count, image_count), generator_(generator)
  {}

  setmem_idevice (shared_ptr<generator> generator,
                  const context& ctx, unsigned image_count = 1)
    : rawmem_idevice (ctx, image_count), generator_(generator)
  {}

  /*!  \note  Produces image \a data from generated octets.
   */
  streamsize read (octet *data, streamsize n)
  {
    streamsize rv = rawmem_idevice::read (data, n);
    if (0 < rv) (*generator_)(data, rv);
    return rv;
  }
};

//!  Generate octets with the same \a value over and over again
class const_generator : public setmem_idevice::generator
{
  const octet value_;

public:
  const_generator (octet value = 0x00) : value_(value) {}
  void operator() (octet *data, streamsize n)
  { traits::assign (data, n, value_); }
};

//!  Generate random octets
class random_generator : public setmem_idevice::generator
{
  file file_;
public:
  random_generator ()
  { file_.open ("/dev/urandom", std::ios_base::binary | std::ios_base::in); }
  void operator() (octet *data, streamsize n)
  { file_.sgetn (data, n); }
};

//!  Filters that %output their %input unchanged
class thru_filter : public filter
{
public:
  streamsize write (const octet *data, streamsize n)
  { return output_->write (data, n); }
};

}       // namespace utsushi

#endif  /* utsushi_test_memory_hpp_ */
