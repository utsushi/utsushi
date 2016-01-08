<?xml version="1.0" encoding="UTF-8"?>
<!--  boost-test-report.xsl :: stylesheet to create Bitten test reports
      Copyright (C) 2012  EPSON AVASYS CORPORATION

      License: GPL-3.0+
      Author : Olaf Meeuwissen

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

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		version="1.0">
  <xsl:output method="xml" encoding="UTF-8"/>

  <xsl:template match="/">
    <BoostTestReport>
      <xsl:apply-templates/>
    </BoostTestReport>
  </xsl:template>

  <xsl:template match="TestCase">
    <test>
      <status>
	<!--  based on <boost/test/impl/xml_report_formatter.ipp> and
	      bitten.report.testing.TestResultsSummarizer@641.  Note,
	      the corresponding TestResultsChartGenerator lumps error
	      results in with the failures.
	  -->
	<xsl:if test="@result = 'passed'">success</xsl:if>
	<xsl:if test="@result = 'failed'">failure</xsl:if>
	<xsl:if test="@result = 'skipped'">ignore</xsl:if>
	<xsl:if test="@result = 'aborted'">error</xsl:if>
      </status>
      <name>
	<xsl:value-of select="@name"/>
      </name>
      <fixture>
	<xsl:call-template name="fixture"/>
      </fixture>
    </test>
  </xsl:template>

  <xsl:template name="fixture">
    <xsl:for-each select="parent::TestSuite">
      <xsl:call-template name="fixture">
	<xsl:with-param name="parent" select=".."/>
      </xsl:call-template>
      <xsl:if test="parent::TestSuite">
	<xsl:text>::</xsl:text>
      </xsl:if>
      <xsl:value-of select="@name"/>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
