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

  <xsl:variable name="table" select="/html/body/table" />
  <xsl:variable name="headrow" select="$table/thead" />

  <xsl:variable name="proprows" select="$table/tbody/tr | $table/tr" />

  <xsl:variable name="suitable">
    <xsl:if test="$headrow">
      <xsl:variable name="col_name" select="$headrow/tr/th[1][.='Name']" />
      <xsl:variable name="col_values" select="$headrow/tr/th[2][.='Values']" />

      <xsl:value-of select="count($col_name) + count($col_values)" />
    </xsl:if>
  </xsl:variable>

  <xsl:template match="/">
    <xsl:choose>
      <xsl:when test="$suitable=2">
        <xsl:processing-instruction name="xml-stylesheet">
          <xsl:text>type="text/xsl" href="propcss.xsl"</xsl:text>
        </xsl:processing-instruction>
        <xsl:value-of select="$nl" />
        <xsl:apply-templates select="$table" />
      </xsl:when>
      <xsl:otherwise>
        <error>
          <xsl:text>
            Unexpectedly renamed or missing 'Name' and 'Values'
            columns.  The stylsheet must be fixed.
          </xsl:text>
        </error>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="html">
    <xsl:processing-instruction name="source">
      Source: https://www.w3.org/TR/CSS2/propidx.html
    </xsl:processing-instruction>
    <xsl:apply-templates select="body/table" />
  </xsl:template>

  <xsl:template match="table">
    <properties>
      <xsl:value-of select="$nl" />
      <xsl:apply-templates select="$proprows" mode="show_props" />
    </properties>
  </xsl:template>

  <xsl:template name="add_value">
    <xsl:param name="value" />
    <xsl:element name="value"><xsl:value-of select="$value" /></xsl:element>
    <xsl:value-of select="$nl" />
  </xsl:template>


  <xsl:template name="parse_values">
    <xsl:param name="values" />

    <xsl:variable name="after" select="normalize-space(substring-after($values,' '))" />
    
    <xsl:variable name="value">
      <xsl:choose>
        <xsl:when test="string-length($after)">
          <xsl:value-of select="substring-before($values,' ')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$values" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>


    <xsl:if test="string-length($value) &gt; 1">
      <xsl:variable name="first" select="substring($value,1,1)" />
      <xsl:if test="$first!=$apos and $first!=$lt">
        <xsl:if test="not(contains($value,'{'))">
          <xsl:if test="not(contains($value,'attr('))">
            <xsl:if test="not(number($value)=$value)">
              <xsl:element name="value">
                <xsl:attribute name="name">
                  <xsl:value-of select="$value" />
                </xsl:attribute>
              </xsl:element>
              <xsl:value-of select="$nl" />
            </xsl:if>
          </xsl:if>
        </xsl:if>
      </xsl:if>
    </xsl:if>

    <xsl:if test="string-length($after)">
      <xsl:call-template name="parse_values">
        <xsl:with-param name="values" select="$after" />
      </xsl:call-template>
    </xsl:if>
    
  </xsl:template>


  <xsl:template match="span[contains(@class,'propinst-')]" mode="show_prop">
    <xsl:param name="values" />
    <xsl:variable name="name" select="substring-before(substring(.,2),$apos)" />
    <xsl:element name="property">
      <xsl:attribute name="name"><xsl:value-of select="$name" /></xsl:attribute>
      <xsl:value-of select="$nl" />
      <xsl:call-template name="parse_values">
        <xsl:with-param name="values" select="$values" />
      </xsl:call-template>
    </xsl:element>
  <xsl:value-of select="$nl" />
</xsl:template>

  <xsl:template match="tr" mode="show_props">
    <xsl:variable name="values"
                  select="translate(string(td[2]),',[]=?|','-     ')" />

    <xsl:apply-templates select="td[1]//span" mode="show_prop">
      <xsl:with-param name="values" select="normalize-space($values)" />
    </xsl:apply-templates>
  </xsl:template>

</xsl:stylesheet>
