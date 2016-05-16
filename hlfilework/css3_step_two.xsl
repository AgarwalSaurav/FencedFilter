<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="text" encoding="utf-8"/>

  <xsl:variable name="indent"><xsl:text>   </xsl:text></xsl:variable>
  <xsl:variable name="nl"><xsl:text>
</xsl:text></xsl:variable>

  <xsl:variable name="apos"><xsl:text>'</xsl:text></xsl:variable>
  <xsl:variable name="gt"><xsl:text>&gt;</xsl:text></xsl:variable>
  <xsl:variable name="lt"><xsl:text>&lt;</xsl:text></xsl:variable>

  <xsl:key name="level" match="/stepone/property" use="@level" />

  <xsl:template match="/">
    <xsl:value-of select="concat('!ht', $nl)" />
    <xsl:value-of select="concat('!ci', $nl)" />

    <xsl:value-of select="concat($nl, 'level_one : span.keyword', $nl)" />
    <xsl:apply-templates select="key('level', '1')" />
    
    <xsl:value-of select="concat($nl, 'level_two : span.keywordtype', $nl)" />
    <xsl:apply-templates select="key('level', '2')" />
    
    <xsl:value-of select="concat($nl, 'level_three : span.keywordflow', $nl)" />
    <xsl:apply-templates select="key('level', '3')" />
  </xsl:template>

  <xsl:template match="property">
    <xsl:value-of select="concat($indent, @name, $nl)" />
  </xsl:template>

</xsl:stylesheet>
