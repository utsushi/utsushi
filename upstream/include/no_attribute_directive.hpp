//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(NO_ATTRIBUTE_JAN_19_2011_0939)
#define NO_ATTRIBUTE_JAN_19_2011_0939

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>

///////////////////////////////////////////////////////////////////////////////
// definition the place holder 
namespace custom_generator 
{ 
    BOOST_SPIRIT_TERMINAL(no_attribute);
} 

///////////////////////////////////////////////////////////////////////////////
// implementation the enabler
namespace boost { namespace spirit 
{ 
    // We want custom_generator::no_attribute to be usable as a directive only, 
    // and only for generator expressions (karma::domain).
    template <>
    struct use_directive<karma::domain, custom_generator::tag::no_attribute> 
      : mpl::true_ {}; 
}}

///////////////////////////////////////////////////////////////////////////////
// implementation of the generator
namespace custom_generator
{ 
    template <typename Subject>
    struct no_attribute_directive 
      : boost::spirit::karma::unary_generator<no_attribute_directive<Subject> >
    {
        // Define required output iterator properties
        typedef typename Subject::properties properties;

        // Define the attribute type exposed by this generator component
        // no_attribute always exposes unused_type, that means no attribute
        template <typename Context, typename Iterator>
        struct attribute 
        {
            typedef boost::spirit::unused_type type;
        };

        no_attribute_directive(Subject const& subject)
          : subject(subject) {}

        // This function is called during the actual output generation process.
        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const&) const
        {
            return subject.generate(sink, ctx, d, boost::spirit::unused);
        }

        // This function is called during error handling to create
        // a human readable string for the error context.
        template <typename Context>
        boost::spirit::info what(Context& ctx) const
        {
            return boost::spirit::info("no_attribute", subject.what(ctx));
        }

        Subject subject;
    };
}

///////////////////////////////////////////////////////////////////////////////
// instantiation of the generator
namespace boost { namespace spirit { namespace karma
{
    // This is the factory function object invoked in order to create 
    // an instance of our simple_columns_generator.
    template <typename Subject, typename Modifiers>
    struct make_directive<custom_generator::tag::no_attribute, Subject, Modifiers>
    {
        typedef custom_generator::no_attribute_directive<Subject> result_type;

        result_type operator()(unused_type, Subject const& s, unused_type) const
        {
            return result_type(s);
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<custom_generator::no_attribute_directive<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<custom_generator::no_attribute_directive<Subject>
          , Attribute, Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
