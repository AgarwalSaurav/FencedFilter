<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="text" encoding="utf-8"/>

  <xsl:variable name="indent"><xsl:text>   </xsl:text></xsl:variable>
  <xsl:variable name="nl"><xsl:text>
</xsl:text></xsl:variable>

  <xsl:key name="vkey" match="/properties/property/value" use="@name" />
  

  <xsl:template match="/">
    <xsl:value-of select="concat('!ht', $nl)" />
    <xsl:value-of select="concat('!ci', $nl)" />
    
    <xsl:apply-templates select="*" />
  </xsl:template>

  <xsl:template match="properties">
    <xsl:value-of select="concat('properties : span.keywordflow', $nl)" />
    <xsl:apply-templates select="*" />
    <xsl:value-of select="concat($nl, 'values : span.keyword', $nl)" />
    <xsl:apply-templates select="property/value" />
  </xsl:template>

  <xsl:template match="property">
    <xsl:value-of select="concat($indent,@name,$nl)" />
  </xsl:template>

  <xsl:template match="value">
    <xsl:variable name="first" select="generate-id(key('vkey', @name))" />
    <xsl:if test="$first = generate-id()">
      <xsl:value-of select="concat($indent,@name,$nl)" />
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
