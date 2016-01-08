//  file.cpp -- unit tests for the utsushi::file API
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/file.hpp"
#include "utsushi/test/memory.hpp"
#include "utsushi/test/null.hpp"

#include <boost/filesystem.hpp>

using namespace utsushi;

using boost::filesystem::file_size;
using boost::filesystem::path;
using boost::filesystem::remove;

BOOST_AUTO_TEST_CASE (logical_non_existence)
{
  path_generator gen;
  BOOST_CHECK (!gen);
}

BOOST_AUTO_TEST_CASE (logical_existence)
{
  path_generator gen ("%i");
  BOOST_CHECK (gen);
}

BOOST_AUTO_TEST_CASE (no_formatter_pattern)
{
  BOOST_CHECK_EQUAL (path_generator (), path_generator ("i"));
}

BOOST_AUTO_TEST_CASE (percent_i)
{
  path_generator gen ("%i");

  BOOST_CHECK_EQUAL (gen (), path ("0"));
  BOOST_CHECK_EQUAL (gen (), path ("1"));
  BOOST_CHECK_EQUAL (gen (), path ("2"));
  BOOST_CHECK_EQUAL (gen (), path ("3"));
}

BOOST_AUTO_TEST_CASE (percent_i_extension)
{
  path_generator gen ("%i.out");

  BOOST_CHECK_EQUAL (gen (), path ("0.out"));
  for (int i = 0; i < 2; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("3.out"));
  BOOST_CHECK_EQUAL (gen (), path ("4.out"));
  BOOST_CHECK_EQUAL (gen (), path ("5.out"));
}

BOOST_AUTO_TEST_CASE (percent_i_postfix)
{
  path_generator gen ("%i-postfix");

  BOOST_CHECK_EQUAL (gen (), path ("0-postfix"));
  for (int i = 0; i < 3; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("4-postfix"));
  for (int i = 0; i < 3; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("8-postfix"));
}

BOOST_AUTO_TEST_CASE (prefix_percent_i)
{
  path_generator gen ("prefix-%i");

  BOOST_CHECK_EQUAL (gen (), path ("prefix-0"));
  for (int i = 0; i < 2; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("prefix-3"));
  for (int i = 0; i < 3; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("prefix-7"));
}

BOOST_AUTO_TEST_CASE (prefix_percent_i_extension)
{
  path_generator gen ("prefix%i.ext");

  BOOST_CHECK_EQUAL (gen (), path ("prefix0.ext"));
  for (int i = 0; i < 8; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("prefix9.ext"));
}

BOOST_AUTO_TEST_CASE (prefix_percent_i_postfix)
{
  path_generator gen ("prefix.%ipost");

  BOOST_CHECK_EQUAL (gen (), path ("prefix.0post"));
  for (int i = 0; i < 3; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("prefix.4post"));
  BOOST_CHECK_EQUAL (gen (), path ("prefix.5post"));
}

BOOST_AUTO_TEST_CASE (percent_escaping)
{
  path_generator gen ("%%%%%i%%%%");

  BOOST_CHECK_EQUAL (gen (), path ("%%0%%"));
  for (int i = 0; i < 4; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("%%5%%"));
}

BOOST_AUTO_TEST_CASE (field_width)
{
  path_generator gen ("prefix-%3i.ext");

  BOOST_CHECK_EQUAL (gen (), "prefix-000.ext");
  BOOST_CHECK_EQUAL (gen (), "prefix-001.ext");
  BOOST_CHECK_EQUAL (gen (), "prefix-002.ext");
}

BOOST_AUTO_TEST_CASE (zero_padded_field_width)
{
  path_generator gen ("prefix-%03i.ext");

  BOOST_CHECK_EQUAL (gen (), "prefix-000.ext");
  BOOST_CHECK_EQUAL (gen (), "prefix-001.ext");
  BOOST_CHECK_EQUAL (gen (), "prefix-002.ext");
}

BOOST_AUTO_TEST_CASE (zeroes_padded_field_width)
{
  path_generator gen ("prefix-%0000003i.ext");

  BOOST_CHECK_EQUAL (gen (), "prefix-000.ext");
  BOOST_CHECK_EQUAL (gen (), "prefix-001.ext");
  BOOST_CHECK_EQUAL (gen (), "prefix-002.ext");
}

BOOST_AUTO_TEST_CASE (default_series)
{
  path_generator gen ("abc%3i");

  BOOST_CHECK_EQUAL (gen (), path ("abc000"));
  BOOST_CHECK_EQUAL (gen (), path ("abc001"));
  BOOST_CHECK_EQUAL (gen (), path ("abc002"));
}

BOOST_AUTO_TEST_CASE (series_with_rollover)
{
  path_generator gen ("./%2i");

  for (int i = 0; i < 99; ++i)
    gen ();
  BOOST_CHECK_EQUAL (gen (), path ("./99"));
  BOOST_CHECK_EQUAL (gen (), path ("./100"));
  BOOST_CHECK_EQUAL (gen (), path ("./101"));
}

BOOST_AUTO_TEST_CASE (series_with_extension)
{
  path_generator gen ("/tmp/prefix-%5i.ps");

  BOOST_CHECK_EQUAL (gen (), path ("/tmp/prefix-00000.ps"));
  for (int i = 0; i < 13; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("/tmp/prefix-00014.ps"));
  BOOST_CHECK_EQUAL (gen (), path ("/tmp/prefix-00015.ps"));
}

BOOST_AUTO_TEST_CASE (series_with_dotted_extension)
{
  path_generator gen ("../cjkv-%4i.tiff");

  for (int i = 0; i < 751; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("../cjkv-0751.tiff"));
  for (int i = 0; i < 123; ++i) gen ();
  BOOST_CHECK_EQUAL (gen (), path ("../cjkv-0875.tiff"));
  BOOST_CHECK_EQUAL (gen (), path ("../cjkv-0876.tiff"));
}

struct fixture
{
  const std::string name_;
  file_odevice odev_;

  fixture () : name_("file.out"), odev_(name_) {}
  ~fixture () { remove (name_); }
};

/*!  Create files for various amounts of image data.
 */
static void
ofilesize (streamsize size)
{
  fixture f;
  rawmem_idevice idev (size);
  idev | f.odev_;

  BOOST_CHECK_EQUAL (size, file_size (f.name_));
}

/*!  Create files with varying numbers of images.
 */
static
void
multi_image (unsigned count)
{
  fixture f;

  rawmem_idevice idev (1 << 10, count);
  idev | f.odev_;

  BOOST_CHECK_EQUAL ((1 << 10) * count, file_size (f.name_));
}

/*!  Create multiple files during a single scan sequence
 */
BOOST_AUTO_TEST_CASE (multi_ofile)
{
  const streamsize octets = 1 << 10;
  const unsigned   images = 4;

  path_generator gen ("file-multi-%3i.out");
  file_odevice odev (gen);

  rawmem_idevice idev (octets, images);

  idev | odev;
  for (unsigned i = 0; i < images; ++i) {
    path p = gen ();
    BOOST_CHECK_EQUAL (octets, file_size (p));
    remove (p);
  }
}

/*!  Read files with varying amounts of data.
 */
static void
ifilesize (streamsize size)
{
  fixture f;
  {                             // create input file for test
    rawmem_idevice idev (size);
    idev | f.odev_;

    BOOST_REQUIRE_EQUAL (size, file_size (f.name_));
  }

  file_idevice idev (f.name_);
  streamsize   rv = idev.marker ();

  BOOST_CHECK_EQUAL (traits::bos (), rv);
  rv = idev.marker ();
  BOOST_CHECK_EQUAL (traits::boi (), rv);

  octet *buffer = new octet[idev.buffer_size ()];
  streamsize count = 0;

  while (traits::eoi () != rv) {
    if (0 < rv) count += rv;
    rv = idev.read (buffer, idev.buffer_size ());
  }
  delete [] buffer;

  BOOST_CHECK_EQUAL (size, count);
}

/*!  Test whether all images of a multi-file input device are read
 */
BOOST_AUTO_TEST_CASE (multi_ifile)
{
  const streamsize octets = 1 << 10;
  const unsigned   images = 4;

  path_generator generator ("file-multi-%3i.in");
  {                             // create input files for test
    path_generator gen = generator;
    file_odevice odev (gen);

    rawmem_idevice idev (octets, images);

    idev | odev;
    for (unsigned i = 0; i < images; ++i) {
      path p = gen ();
      BOOST_CHECK_EQUAL (octets, file_size (p));
    }
  }
  {
    path_generator gen = generator;
    file_idevice idev (gen);

    null_odevice odev;

    unsigned   count = 0;
    streamsize rv    = idev.marker ();
    BOOST_CHECK_EQUAL (traits::bos (), rv);
    while (traits::eos () != rv) {
      rv = (idev >> odev);
      if (traits::eoi () == rv)
        ++count;
      remove (gen ());
    }
    BOOST_CHECK_EQUAL (images, count);
  }
}

struct named_file_fixture
{
  const streamsize  octet_count;
  const unsigned    image_count;
  const unsigned    sequence_count;
  const std::string name;

  file_idevice   idev;
  null_odevice   odev;

  named_file_fixture ()
    : octet_count (40 * 8192), image_count (1), sequence_count (9),
      name ("named-file-"), idev (name)
  {
    rawmem_idevice idev_gen (octet_count, image_count);
    file_odevice   odev_gen (name);
    idev_gen | odev_gen;
  }

  ~named_file_fixture ()
  {
      remove (name);
  }
};

BOOST_FIXTURE_TEST_SUITE (named_file, named_file_fixture);

/*! Tests that the sequence contains only a single image
 */
BOOST_AUTO_TEST_CASE (single_sequence_single_file)
{
  streamsize rv = idev.marker ();
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  rv = idev >> odev;
  BOOST_CHECK_EQUAL (traits::eoi (), rv);
  rv = idev.marker ();
  BOOST_REQUIRE_EQUAL (traits::eos (), rv);
}

/*! Tests that a sequence can be read correctly multiple
 *  times from a file idevice with a single named file
 */
BOOST_AUTO_TEST_CASE (multi_sequence_single_file)
{
  for (unsigned int i=0; i<sequence_count; ++i) {
    unsigned count = 0;
    streamsize rv = 0;

    rv = idev.marker ();
    BOOST_CHECK_EQUAL (traits::bos (), rv);
    while (count < image_count) {
      rv = idev >> odev;
      BOOST_CHECK_EQUAL (traits::eoi (), rv);
      count++;
    }
    rv = idev.marker ();
    BOOST_CHECK_EQUAL (traits::eos (), rv);
    BOOST_CHECK_EQUAL (image_count, count);
  }
}

BOOST_AUTO_TEST_SUITE_END ();

struct gen_file_fixture
{
  const streamsize octet_count;
  const unsigned   image_count;
  const unsigned   sequence_count;

  path_generator gen;
  file_idevice   idev;
  null_odevice   odev;

  rawmem_idevice idev_gen;
  file_odevice   odev_gen;

  gen_file_fixture ()
    : octet_count (40 * 8192), image_count (3), sequence_count (9),
      gen ("gen-file-%3i"), idev (gen),
      idev_gen (octet_count, image_count), odev_gen (gen)
  {
  }

  ~gen_file_fixture ()
  {
    for (unsigned i = 0; i < image_count*sequence_count; ++i) {
      remove (gen ());
    }
  }
};

BOOST_FIXTURE_TEST_SUITE (gen_file, gen_file_fixture);

/*! Tests that a sequence can be read correctly multiple
 *  times from a file idevice with multiple generated files
 */
BOOST_AUTO_TEST_CASE (multi_sequence_multi_file)
{
  for (unsigned int i = 0; i < sequence_count; ++i) {
    unsigned count = 0;
    streamsize rv = 0;

    idev_gen.reset ();
    idev_gen | odev_gen; // generate new files

    rv = idev.marker ();
    BOOST_CHECK_EQUAL (traits::bos (), rv);
    while (count < image_count) {
      rv = idev >> odev;
      BOOST_CHECK_EQUAL (traits::eoi (), rv);
      count++;
    }
    rv = idev.marker ();
    BOOST_CHECK_EQUAL (traits::eos (), rv);
    BOOST_CHECK_EQUAL (image_count, count);
  }
}

BOOST_AUTO_TEST_SUITE_END ();

#define ARRAY_END(a)  ((a) + sizeof (a) / sizeof (a[0]))

bool
init_test_runner ()
{
  namespace but = boost::unit_test;

  streamsize size[] = {
     1, 2, 16, 64, 256, 512,
     8 << 10,                   //   8 KB
    (8 << 10) + 1,
    (8 << 14) - 1,
     8 << 14,                   // 128 KB
  };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (ofilesize, size, ARRAY_END (size)));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (ifilesize, size, ARRAY_END (size)));

  unsigned count[] = { 1, 2, 4, 8, 16, 32 };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (multi_image, count, ARRAY_END (count)));

  return true;
}

#include "utsushi/test/runner.ipp"
