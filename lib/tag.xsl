<?xml version="1.0" encoding="UTF-8"?>
<!--  tag.xsl :: converts XML tag data to other formats
      Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION

      License: GPL-3.0+
      Author : EPSON AVASYS CORPORATION

      This file is part of the 'Utsushi' package.
      This package is free software: you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation, either version 3 of the License or, at
      your option, any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

      You ought to have received a copy of the GNU General Public License
      along with this package.  If not, see <http://www.gnu.org/licenses/>.
  -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:template match="tags">

    <xsl:choose>

      <xsl:when test="$format='hpp'">
//  Automatically generated from lib/tag.xml using lib/tag.xsl.

#ifndef utsushi_tag_hpp_
#define utsushi_tag_hpp_

#include &lt;set&gt;

#include &lt;boost/operators.hpp&gt;

#include "key.hpp"
#include "string.hpp"

namespace utsushi {

struct tag
{
  //! Collect options and groups by the aspects they affect
  /*! A %tag::symbol is a string-like key that can be used to indicate
   *  which aspects are affected by an option or group.  An option may
   *  specify zero or more %tag symbols.
   *
   *  Similar to a descriptor, every %tag::symbol provides name() and
   *  text() accessors describing its purpose.  A user interface may
   *  use this information to provide a more human-oriented and mostly
   *  self-documenting view of tags.
   *
   *  \note  The UI is responsible for the translation of name() and
   *         text() as well as any formatting of the text().
   *
   *  \sa  descriptor::name(), descriptor::text()
   */
  class symbol
    : boost::totally_ordered&lt; symbol &gt;
  {
  public:
    symbol (const key&amp; key,
            const string&amp; name, const string&amp; text);

    //! \copybrief descriptor::name
    const string&amp; name () const;
    //! \copybrief descriptor::text
    const string&amp; text () const;

    bool operator== (const symbol&amp; ts) const;
    bool operator&lt; (const symbol&amp; ts) const;

    operator key () const;

  private:
    key key_;
    string name_;
    string text_;
  };

  static const symbol none;<xsl:for-each select="tag">
  static const symbol <xsl:value-of select="@key"/>;</xsl:for-each>
};

class tags
{
private:
  typedef std::set&lt; tag::symbol &gt; container_type;
  static container_type set_;
  static void initialize ();

public:
  typedef container_type::size_type size_type;
  typedef container_type::const_iterator const_iterator;

  static size_type count ();
  static const_iterator begin ();
  static const_iterator end ();
};

}       // namespace utsushi

#endif  /* utsushi_tag_hpp_ */
<xsl:text/>
      </xsl:when>

      <xsl:when test="$format='cpp'">
//  Automatically generated from tag.xml using tag.xsl.

#ifdef HAVE_CONFIG_H
#include &lt;config.h&gt;
#endif

#include "utsushi/i18n.hpp"
#include "utsushi/tag.hpp"

namespace utsushi {

tag::symbol::symbol (const key&amp; key,
                     const string&amp; name, const string&amp; text)
  : key_(key), name_(name), text_(text)
{}

const string&amp;
tag::symbol::name () const
{
  return name_;
}

const string&amp;
tag::symbol::text () const
{
  return text_;
}

bool
tag::symbol::operator== (const symbol&amp; ts) const
{
  return key_ == ts.key_;
}

bool
tag::symbol::operator&lt; (const tag::symbol&amp; ts) const
{
  return key_ &lt; ts.key_;
}

tag::symbol::operator key () const
{
  return key_;
}

<xsl:for-each select="tag">
const tag::symbol tag::<xsl:value-of select="@key"/> (
  "<xsl:number format="01"/>_<xsl:value-of select="@key"/>",
  SEC_N_("<xsl:value-of select="normalize-space(name)"/>"),
  CCB_N_("<xsl:value-of select="normalize-space(text)"/>")
);</xsl:for-each>

tags::container_type tags::set_;

void
tags::initialize ()
{
  container_type::iterator hint = set_.begin ();

<xsl:for-each select="tag">  hint = set_.insert (hint, tag::<xsl:value-of select="@key"/>);
</xsl:for-each>}

tags::size_type
tags::count ()
{
  if (set_.empty ())
    {
      initialize ();
    }
  return set_.size ();
}

tags::const_iterator
tags::begin ()
{
  if (set_.empty ())
    {
      initialize ();
    }
  return set_.begin ();
}

tags::const_iterator
tags::end ()
{
  if (set_.empty ())
    {
      initialize ();
    }
  return set_.end ();
}

}       // namespace utsushi
<xsl:text/>
      </xsl:when>

    </xsl:choose>

  </xsl:template>

</xsl:stylesheet>
