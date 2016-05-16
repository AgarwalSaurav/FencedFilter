<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="text" encoding="utf-8"/>

  <xsl:variable
      name="content"
      select="html/body/div[@id='content']/div[@id='bodyContent']/div[@id='mw-content-text']" />

  <xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
  <xsl:variable name="lowercase" select="'abcdefghijklmnopqrstuvwxyz'" />

  <xsl:variable name="indent"><xsl:text>   </xsl:text></xsl:variable>
  <xsl:variable name="nl"><xsl:text>
</xsl:text></xsl:variable>


  <xsl:template match="/">
    <xsl:apply-templates select="$content" />
  </xsl:template>

  <xsl:template match="div[@id='mw-content-text']">
    <xsl:apply-templates select="table[contains(@class,'wikitable')]" />
  </xsl:template>


  <xsl:template match="tr" mode="confirm_col_three">
    <xsl:param name="expected" />
    <xsl:variable name="col_three" select="normalize-space(string(th[3]))" />
    <xsl:value-of select="$expected=$col_three" />
  </xsl:template>

  <xsl:template match="table[contains(@class,'wikitable')][1]">
    <xsl:variable name="confirm">
      <xsl:apply-templates select="tr[1]" mode="confirm_col_three">
        <xsl:with-param name="expected" select="'Element'" />
      </xsl:apply-templates>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$confirm='true'">
        <xsl:text>!ci</xsl:text>
        <xsl:value-of select="$nl" />
        <xsl:text>elements : span.keywordflow</xsl:text>
        <xsl:value-of select="$nl" />
        
        <xsl:apply-templates select="tr" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>
          The format of the source content has changed.
          The stylesheet must be redone.
        </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="tr">
    <xsl:if test="not(contains(@style,'display:none')) and count(th)=0">
      <xsl:variable name="mixed" select="normalize-space(string(td[3]))" />
      <xsl:variable name="val" select="translate($mixed,$uppercase,$lowercase)" />
      <xsl:value-of select="concat($indent,$val,$nl)" />
    </xsl:if>
  </xsl:template>

  <xsl:template match="*"></xsl:template>

</xsl:stylesheet>
