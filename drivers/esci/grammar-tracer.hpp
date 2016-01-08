//  grammar-tracer.hpp -- support for logging and debugging
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2001-2011  Hartmut Kaiser
//  Copyright (C) 2001-2011  Joel de Guzman
//
//  License: BSL-1.0
//  Author : EPSON AVASYS CORPORATION
//  Origin : Boost.Spirit
//
//  This file is part of the 'Utsushi' package.
//  While the package as a whole is free software distributed under
//  the terms of the GNU General Public License (version 3 or later),
//  this file is distributed under the terms of the Boost Software
//  License, Version 1.0.
//
//  You ought to have received a copy of the Boost Software License
//  along along with this package.
//  If not, see <http://www.boost.org/LICENSE_1_0.txt>.

#ifndef drivers_esci_grammar_tracer_hpp_
#define drivers_esci_grammar_tracer_hpp_

#include <iterator>
#include <string>

#include <boost/fusion/include/empty.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/support_attributes.hpp>

#if !defined (ESCI_GRAMMAR_TRACE_INDENT)
//! Default number of spaces to use for each level of indentation
#define ESCI_GRAMMAR_TRACE_INDENT 2
#endif

#if !defined (ESCI_GRAMMAR_TRACE_CUTOFF)
//! Default limit for parser input and generator output display
/*! A negative value can be used to display all of the parser's input
 *  and all of the generator's output.
 */
#define ESCI_GRAMMAR_TRACE_CUTOFF 40
#endif

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Grammar tracer formatting aspects
/*! This class provides the low-level formatting support used by both
 *  the decoding::grammar_tracer and encoding::grammar_tracer.  These
 *  tracers create pretty-printed, XML-based output.
 */
struct grammar_tracer_formatter
{
  enum tag_type {
    empty,
    start,
    end,
  };

  //! Initializes a formatter for grammar traces
  /*! All trace information will be sent to the \a os output stream.
   *  The trace will use \a indent spaces for each indentation level
   *  and show at most \a cutoff elements of the parser input and
   *  generator output.  These values are constant for the life-time
   *  of the object.
   */
  grammar_tracer_formatter (std::ostream& os, int indent, int cutoff)
    : os_(os)
    , indent_(indent)
    , cutoff_(cutoff)
  {}

  //! Current indentation level
  /*! This returns a reference to the level so user can increment and
   *  decrement it as necessary.
   */
  int&
  level () const
  {
    static int level = 0;
    return level;
  }

  //! Produce whitespace for the current indentation \a level
  void
  indent (int level) const
  {
    for (int i = 0; i != level * indent_; ++i)
      os_ << ' ';
  }

  //! Produce the trace's starting content
  void
  pre (const std::string& rule) const
  {
    indent (level ()++);
    tag (rule, start) << '\n';
  }

  //! Produce the trace's ending content
  void
  post (const std::string& rule) const
  {
    indent (--level ());
    tag (rule, end) << '\n';
  }

  //! Produce a formatted tag for an element
  /*! Formatting for starting, ending and empty elements can be
   *  selected by a \a type argument.
   */
  std::ostream&
  tag (const std::string& elem, tag_type type = empty) const
  {
    switch (type)
      {
      case empty:
        os_ << '<' << elem << "/>";
        break;
      case start:
        os_ << '<' << elem << '>';
        break;
      case end:
        os_ << "</" << elem << '>';
        break;
      }
    return os_;
  }

  //! Stream parser input onto the tracer's stream
  template< typename Iterator >
  void
  tag (const std::string& elem, Iterator head, const Iterator& tail) const
  {
    int i = 0;

    indent (level ());
    if (head == tail)
      {
        tag (elem) << '\n';
        return;
      }

    tag (elem, start);
    while (head != tail && i != cutoff_ && head != Iterator ())
      {
        os_ << *head;
        ++i, ++head;
      }
    tag (elem, end) << '\n';
  }

  //! Stream generator output onto the tracer's stream
  template< typename Buffer >
  void
  tag (const std::string& elem, const Buffer& buffer) const
  {
    indent (level ());
    tag (elem, start);
    {
      std::ostreambuf_iterator< char > os (os_);
      buffer.buffer_copy_to (os, cutoff_);
    }
    tag (elem, end) << '\n';
  }

  //! Output a rule's attributes to the tracer's stream
  template< typename Context >
  void
  attributes (const Context& context) const
  {
    indent (level ());
    tag ("attributes", start);
    boost::spirit::traits::print_attribute (os_, context.attributes);
    tag ("attributes", end) << '\n';
  }

  //! Output a rule's local variables to the tracer's stream
  template< typename Context >
  void
  locals (const Context& context) const
  {
    if (!boost::fusion::empty (context.locals))
      {
        indent (level ());
        tag ("locals", start);
        os_ << context.locals;
        tag ("locals", end) << '\n';
      }
  }

  std::ostream& os_;
  const int indent_;
  const int cutoff_;
};

namespace decoding {

namespace qi = boost::spirit::qi;

//! Create input parser traces
/*! These function objects may be used by the parser framework at
 *  various stages in the parsing process.  Simply associate an
 *  instance with any rule that you want to debug().
 */
class grammar_tracer
  : protected grammar_tracer_formatter
{
public:
  grammar_tracer (std::ostream& os,
                  int indent = ESCI_GRAMMAR_TRACE_INDENT,
                  int cutoff = ESCI_GRAMMAR_TRACE_CUTOFF)
    : grammar_tracer_formatter (os, indent, cutoff)
  {};

  //! Produce trace content for the current parser \a state
  /*! The parser's \a rule_name and the \a head and \a tail of its
   *  input are available, together with a parser \a context that can
   *  be used to access its attributes and local variables, if any.
   */
  template< typename Iterator, typename Context, typename State >
  void
  operator() (const Iterator& head, const Iterator& tail,
              const Context& context, State state,
              const std::string& rule_name) const
  {
    switch (state)
      {
      case qi::pre_parse:
        {
          pre (rule_name);
          tag ("attempt", head, tail);
          break;
        }
      case qi::successful_parse:
        {
          tag ("success", head, tail);
          attributes (context);
          locals (context);
          post (rule_name);
          break;
        }
      case qi::failed_parse:
        {
          indent (level ());
          tag ("failure") << '\n';
          post (rule_name);
          break;
        }
      }
  }
};

}       // namespace decoding

namespace encoding {

namespace karma = boost::spirit::karma;

//! Create output generator traces
/*! These function objects may be used by the generator framework at
 *  various stages in the generating process.  Just associate a rule
 *  with a grammar tracer and you are ready to debug().
 */
class grammar_tracer
  : protected grammar_tracer_formatter
{
public:
  grammar_tracer (std::ostream& os,
                  int indent = ESCI_GRAMMAR_TRACE_INDENT,
                  int cutoff = ESCI_GRAMMAR_TRACE_CUTOFF)
    : grammar_tracer_formatter (os, indent, cutoff)
  {};

  //! Produce trace output for the current generator \a state
  /*! In addition to the generator \a rule_name, \a context is made
   *  available to refer to attributes and local variables, if any.
   *  A \a buffer with the output generated so far is provided too.
   */
  template< typename OutputIterator, typename Context, typename State
          , typename Buffer >
  void
  operator() (OutputIterator& sink, const Context& context,
              State state, const std::string& rule_name,
              const Buffer& buffer) const
  {
    switch (state)
      {
      case karma::pre_generate:
        {
          pre (rule_name);
          indent (level ()++);
          tag ("attempt", start) << '\n';
          attributes (context);
          locals (context);
          indent (--level ());
          tag ("attempt", end) << '\n';
          break;
        }
      case karma::successful_generate:
        {
          indent (level ()++);
          tag ("success", start) << '\n';
          tag ("result", buffer);
          locals (context);
          indent (--level ());
          tag ("success", end) << '\n';
          post (rule_name);
          break;
        }
      case karma::failed_generate:
        {
          indent (level ());
          tag ("failure") << '\n';
          post (rule_name);
          break;
        }
      }
  }
};

}       // namespace encoding

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_grammar_tracer_hpp_ */
