<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet
   version="1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
   xmlns:html="http://www.w3.org/1999/xhtml">

  <xsl:output method="xml" standalone="yes" encoding="utf-8"/>

  <xsl:variable name="nl"><xsl:text>
</xsl:text></xsl:variable>

  <xsl:variable name="apos"><xsl:text>'</xsl:text></xsl:variable>
  <xsl:variable name="gt"><xsl:text>&gt;</xsl:text></xsl:variable>
  <xsl:variable name="lt"><xsl:text>&lt;</xsl:text></xsl:variable>

  <xsl:variable name="tables"
                select="//table[contains(@class,'w3-table-all')]" />


  <xsl:template match="/">
    <stepone>
      <xsl:apply-templates select="$tables" />
    </stepone>
  </xsl:template>

  <xsl:template match="html:table">
    <xsl:apply-templates select="tr" />
  </xsl:template>

  <xsl:template match="tr">
    <xsl:variable name="count" select="count(th)" />

    <xsl:if test="$count=0">
      <xsl:variable name="property" select="normalize-space(string(td[1]))" />
      <xsl:variable name="level" select="normalize-space(string(td[3]))" />

      <xsl:element name="property">
        <xsl:attribute name="name">
          <xsl:value-of select="$property" />
        </xsl:attribute>
        <xsl:attribute name="level">
          <xsl:value-of select="$level" />
        </xsl:attribute>
      </xsl:element>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
