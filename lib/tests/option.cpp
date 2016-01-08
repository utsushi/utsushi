//  option.cpp -- unit tests for the utsushi::option API
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#include <boost/mpl/transform_view.hpp>

#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/functional.hpp"
#include "utsushi/option.hpp"
#include "utsushi/range.hpp"
#include "utsushi/store.hpp"

#include "value.hpp"

#if __cplusplus >= 201103L
#define NS(bind) std::bind
#define NSPH(_1) std::placeholders::_1
#else
#define NS(bind) bind
#define NSPH(_1) _1
#endif

using namespace utsushi;
namespace mpl = boost::mpl;
using boost::mpl::_;

BOOST_TEST_DONT_PRINT_LOG_VALUE (utsushi::option)

//!  \brief  Create maps with options using various bounded types
/*!  This fixture initializes a option::map instance with options that
 *   cover all bounded types in the \a test_type_list.  Implementation
 *   relies on specialisations of the bounded<T> template defined in
 *   value.hpp to do so.
 */
template <typename test_type_list>
struct test_option_map_fixture
{
  option::map m;

  //  Insert settings for each of the bounded type values
  struct setting_inserter
  {
    template <typename BT> void operator () (option::map& m, const BT& bt)
    {
      for (typename BT::size_type i = 0; i < bt.size (); ++i)
        {
          m.add_options () (bt.key (i), bt.value (i));
        }
    }
  };

  //  Loop over all the types in the test_type_list and add the values
  //  provided via the bounded<T> default constructor to the map with
  //  options.
  test_option_map_fixture ()
  {
    mpl::for_each
      < mpl::transform_view < test_type_list, bounded_type_fixture<_> > >
      (NS(bind)< void > (setting_inserter (), ref (m), NSPH(_1)));
    BOOST_TEST_MESSAGE ("option::map.size () = " << m.size ());
  }
};

BOOST_AUTO_TEST_CASE (access_non_existent_setting)
{
  option::map m;

  BOOST_CHECK_THROW (m["key"], std::out_of_range);
}

BOOST_AUTO_TEST_CASE (insert_setting_with_same_key)
{
  option::map m;

  BOOST_CHECK_THROW (m.add_options ()
                     ("key", "val1")
                     ("key", "val2"), std::logic_error);
}

/*  The idea here is that getting and setting option values should be
 *  easy when working with an option::map.  You should not have to get
 *  the option first explicitly before you can use its value.  That's
 *  something the implementation should and can take care of in a way
 *  that's transparent to the programmer.
 *
 *  Similarly, it should also be easy to get at the value's underlying
 *  or bounded type.  During implementation it became clear that the
 *  explicit construction of a temporary value object is necessary in
 *  some cases (those where the bounded type is used on the left-hand
 *  side (LHS) of an assignment or comparison).  Adding the conversion
 *  operators to option would lead to ambiguous overloads.  As one
 *  would have to resolve these in the application code anyway, it is
 *  likely clearer to use a value object that in turn knows how to
 *  convert to the bounded type.  Codewise, the `value (grp[key])` is
 *  probably more intuitive than the `grp[key].value()` alternative.
 */
BOOST_FIXTURE_TEST_CASE_TEMPLATE (access_and_assignment, T, value::types,
                                  test_option_map_fixture< value::types >)
{
  bounded_type_fixture< T > bt;

  BOOST_REQUIRE_LT (1, bt.size ());

  key k1 = bt.key (0);
  key k2 = bt.key (1);

  T t1 = bt.value (0);
  T t2 = bt.value (1);

  BOOST_REQUIRE_NO_THROW (m[k1]);
  BOOST_REQUIRE_NO_THROW (m[k2]);
  BOOST_REQUIRE_NE (t1, t2);

  //  Assign an option's value to a value object.  Point to note here
  //  is that option::map::operator[], the option accessor, does NOT
  //  return a value object.

  value v = m[k1];

  //  Compare value object with bounded type objects as well as option
  //  accessor returned objects.

  //BOOST_CHECK_EQUAL (v, t1);
  BOOST_CHECK_EQUAL (v, m[k1]);
  //BOOST_CHECK_NE    (v, t2);
  BOOST_CHECK_NE    (v, m[k2]);

  //  Assign option value to bounded type.  The value() intermediate
  //  is required to resolve type conversion ambiguities.

  T t = value (m[k2]);

  //  Compare bounded type object with objects of same type as well as
  //  value typed object.  Note, the value "cast" is required in order
  //  to resolve type conversion ambiguities.

  BOOST_CHECK_EQUAL (t, t2);
  BOOST_CHECK_EQUAL (t, value (m[k2]));
  BOOST_CHECK_NE    (t, t1);
  BOOST_CHECK_NE    (t, value (m[k1]));

  // Swap option values by direct assignment of a bounded type object
  // as well as through explicit intermediate conversion to a value.

  m[k1] = t2;
  m[k2] = value (t1);

  // Flip LHS and RHS in the equality comparisons.  Note that casting
  // to a value object is no longer needed in this case.

  BOOST_CHECK_EQUAL (t1   , v);
  BOOST_CHECK_NE    (m[k1], v);
  BOOST_CHECK_NE    (t2   , v);
  BOOST_CHECK_EQUAL (m[k2], v);

  BOOST_CHECK_EQUAL (t2   , t);
  BOOST_CHECK_NE    (m[k2], t);
  BOOST_CHECK_NE    (t1   , t);
  BOOST_CHECK_EQUAL (m[k1], t);
}

/*  Option providers may want/need to expose options provided by their
 *  constituent objects.  When doing so, this should be transparent to
 *  the user of an option::map.
 */
BOOST_AUTO_TEST_CASE (recursive_option_maps)
{
  option::map m, m_sub1;

  m.add_options ()
    ("key", "val")
    ;
  m_sub1.add_options ()
    ("key", "foo")
    ;

  // FIXME remove preprocessor guard
#ifdef LET_OPTION_MAP_GO_OUT_OF_SCOPE
  {                       // add an option::map that goes out of scope
#endif
    option::map m_sub2;

    m_sub2.add_options ()
      ("key", "bar")
      ;

    option::map::size_type sum = m.size () + m_sub1.size () + m_sub2.size ();

    m.add_option_map ()
      ("sub1", option::map::ptr (&m_sub1, null_deleter ()))
      ("sub2", option::map::ptr (&m_sub2, null_deleter ()))
      ;

    BOOST_CHECK_EQUAL (sum, m.size ());
#ifdef LET_OPTION_MAP_GO_OUT_OF_SCOPE
    }
#endif

  //  Normally one would work with key variables rather than string
  //  literals.  In that case the compound key construction details
  //  will be of no concern to the API user.  Here we need to use a
  //  bit of knowledge about those details.

  BOOST_CHECK_NO_THROW (m[key ("sub1") / "key"] = "val");
  BOOST_CHECK_EQUAL ("val", m_sub1["key"]);

  BOOST_CHECK_NO_THROW (m[key ("sub2") / "key"] = "val");
  BOOST_CHECK_EQUAL ("val", m["sub2/key"]);
}

/*  So called "smart" UI controls may want to update option values in
 *  a batch, for example a scan area selector.  Such "smart" controls
 *  can significantly improve the user experience where tight coupling
 *  of options would cause frequent constraint violations.
 */
BOOST_AUTO_TEST_CASE (multi_assign)
{
  option::map m, m_sub;

  m.add_options ()
    ("key", "val")
    ("foo", "bar")
    ;
  m_sub.add_options ()
    ("bar", "foo")
    ;

  m.add_option_map ()
    ("sub", option::map::ptr (&m_sub, null_deleter ()))
    ;

  value::map vm;
  vm["foo"] = "fuu";
  vm["sub/bar"] = "ber";

  m.assign (vm);

  BOOST_CHECK_EQUAL ("fuu", m["foo"]);
  BOOST_CHECK_EQUAL ("ber", m["sub/bar"]);
}

/*  Attempting to add a option::map to itself is very likely to
 *  trigger an infinite loop.  This loop comes about as the
 *  option::map is iterating towards its end while making itself
 *  bigger with each increment of the iterator.  That means the
 *  current iterator and the option::map's end are and remain a
 *  constant distance away from each other throughout the whole
 *  addition process.
 */
BOOST_AUTO_TEST_CASE (add_option_map_to_self)
{
  option::map m;

  m.add_options ()
    ("key", "val")
    ("foo", "bar")
    ;

  BOOST_CHECK_THROW (m.add_option_map ()
                     ("self", option::map::ptr (&m, null_deleter ())),
                     std::logic_error);
}

BOOST_AUTO_TEST_CASE (value_type_changes)
{
  option::map m;

  m.add_options ()
    ("key", "val")
    ("foo", "bar", constraint::none)
    ;

  BOOST_CHECK_THROW (m["key"] = 5.0, constraint::violation);
  BOOST_CHECK_NO_THROW (m["foo"] = 5.0);
}

bool r (const value::map& vm)
{
  return vm.find("key")->second.type () == vm.find("foo")->second.type ();
}

BOOST_AUTO_TEST_CASE (coupled_options)
{
  option::map m;

  m.add_options ()
    ("key", "val", constraint::none)
    ("foo", "bar", constraint::none)
    ("bar", "fuu")
    ;
  m.impose (r);

  //  Change something that is not involved in the restriction at all
  //  first and then a single value that is.

  BOOST_CHECK_NO_THROW (m["bar"] = "ber");
  BOOST_CHECK_NO_THROW (m["foo"] = "fuu");

  //  Change multiple values at a time that satisfy the restriction.

  value::map vm;
  vm["key"] = toggle (true);
  vm["foo"] = toggle (false);

  BOOST_REQUIRE (r (vm));

  BOOST_CHECK_NO_THROW (m.assign (vm));

  //  Change multiple values at a time that violate the restriction.

  vm["key"] = "val";
  vm["foo"] = 5.0;

  BOOST_REQUIRE (!r (vm));

  BOOST_CHECK_THROW (m.assign (vm), constraint::violation);
}

#define N_                      /* for illustration purposes only */

BOOST_AUTO_TEST_CASE (range_constraint)
{
  option::map m;

  m.add_options ()
    ("resolution", (from< range > ()
                    -> lower (  50.)
                    -> upper (1200.)
                    -> default_value (300.)
                    ),
     attributes (tag::general),
     N_("Resolution"))
    ;

  BOOST_CHECK_EQUAL (m["resolution"], 300.);

  BOOST_CHECK_NO_THROW (m["resolution"] =   50.);
  BOOST_CHECK_NO_THROW (m["resolution"] =  600.);
  BOOST_CHECK_NO_THROW (m["resolution"] = 1200.);

  BOOST_CHECK_THROW (m["resolution"] =   25., constraint::violation);
  BOOST_CHECK_THROW (m["resolution"] = 2400., constraint::violation);
  BOOST_CHECK_THROW (m["resolution"] = -300., constraint::violation);

  BOOST_CHECK_THROW (m["resolution"] = "150dpi", constraint::violation);
}

BOOST_AUTO_TEST_CASE (store_constraint)
{
  option::map m;

  m.add_options ()
    ("format", (from< store > ()
                -> alternative (N_("JPEG"))
                -> alternative (N_("PDF"))
                -> default_value (N_("PNG"))
                ),
     attributes (),
     N_("File Format"),
     N_("Selects output file format.\n"
        "\n"
        "Essay on the pros and cons of the various supported output "
        "file formats to follow later.  This will bore the informed "
        "user to no end, of course, so we will omit it for our unit "
        "tests."
        ))
    ;

  BOOST_CHECK_EQUAL (m["format"], "PNG");

  BOOST_CHECK_NO_THROW (m["format"] = "PDF");
  BOOST_CHECK_THROW    (m["format"] = "pdf", constraint::violation);
  BOOST_CHECK_THROW    (m["format"] = "BMP", constraint::violation);
}

/*  option::maps are intrinsically recursive.  Here we test the basic
 *  recursion functionality by repeatedly adding one option::map to
 *  another and vice versa.  This is a somewhat artificial scenario
 *  but it has the advantage of requiring little code to implement.
 */
void
test_recursion_depth (const int& depth)
{
  BOOST_TEST_MESSAGE (__FUNCTION__ << " (" << depth << ")");

  option::map m1, m2;

  m1.add_options ()
    ("key", "val")
    ("foo", "bar")
    ;
  m2.add_options ()
    ("key", "val")
    ("foo", "bar")
    ("bar", "fuu")
    ;
  option::map::size_type size1 = m1.size ();
  option::map::size_type size2 = m2.size ();

  for (int i = 0; i < depth; ++i)
    {
      std::string d (i, '-');           // use unique name spaces

      m1.add_option_map () (d + "m2", option::map::ptr (&m2, null_deleter ()));
      size1 += size2;
      m2.add_option_map () (d + "m1", option::map::ptr (&m1, null_deleter ()));
      size2 += size1;
    }
  BOOST_CHECK_EQUAL (m1.size (), size1);
  BOOST_CHECK_EQUAL (m2.size (), size2);
}

bool
init_test_runner ()
{
  namespace but =::boost::unit_test;

  //  Note, at a recursion depth of 10 you end up with option::maps
  //  that have 28,657 and 46,368 settings respectively.  You likely
  //  don't want to push that up any further.

  int depths[] = { 1, 2, 3, 4, 5, 10 };

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE
          (test_recursion_depth,
           depths, depths + sizeof (depths) / sizeof (*depths)));

  return true;
}

#include "utsushi/test/runner.ipp"
