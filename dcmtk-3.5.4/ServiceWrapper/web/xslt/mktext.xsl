<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:sd="http://www.weasis.org/xsd">
  <xsl:output method="text" version="1.0" encoding="gbk" indent="no"/>
  <xsl:key name="distinctModalityPerStudy" match="/sd:wado_query/sd:Patient/sd:Study/sd:Series" use="concat(../@StudyInstanceUID, @Modality)" />
  <xsl:template name="concatModalities">
    <xsl:param name="study"/>
    <xsl:for-each select="$study/sd:Series[generate-id()=generate-id(key('distinctModalityPerStudy', concat(../@StudyInstanceUID, @Modality)))]">
      <xsl:value-of select="./@Modality" />
      <xsl:if test="position() != last()">,</xsl:if>
    </xsl:for-each>
  </xsl:template>
  <xsl:template name="displaySex">
    <xsl:choose>
      <xsl:when test="@PatientSex='M'">男</xsl:when>
      <xsl:when test="@PatientSex='F'">女</xsl:when>
      <xsl:otherwise></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="calculateAge">
    <xsl:param name="afterYear" />
    <xsl:if test="string-length(@PatientBirthDate)>0">
      <xsl:value-of select="$afterYear - round(@PatientBirthDate div 10000)" />
    </xsl:if>
  </xsl:template>
  <xsl:template match="/sd:wado_query/sd:Patient/sd:Study">
    <xsl:text>StudySize=</xsl:text><xsl:value-of select="./@StudyDescription" /><xsl:text>&#x000A;</xsl:text>
    <xsl:text>StudyDate=</xsl:text><xsl:value-of select="concat(substring(@StudyDate, 1, 4), '/', substring(@StudyDate, 5, 2), '/', substring(@StudyDate, 7, 2))" /><xsl:text>&#x000A;</xsl:text>
    <xsl:text>AccessionNumber=</xsl:text><xsl:value-of select="./@AccessionNumber" /><xsl:text>&#x000A;</xsl:text>
    <xsl:text>Modality=</xsl:text><xsl:call-template name="concatModalities"><xsl:with-param name="study" select="."/></xsl:call-template><xsl:text>&#x000A;</xsl:text>
  </xsl:template>
  <xsl:template match="/sd:wado_query/sd:Patient">
    <xsl:text>PatientID=</xsl:text><xsl:value-of select="@PatientID" /><xsl:text>&#x000A;</xsl:text>
    <xsl:text>PatientName=</xsl:text><xsl:value-of select="@PatientName" /><xsl:text>&#x000A;</xsl:text>
    <xsl:text>Gender=</xsl:text><xsl:call-template name="displaySex" /><xsl:text>&#x000A;</xsl:text>
    <xsl:text>Birthday=</xsl:text><xsl:if test="string-length(@PatientBirthDate)>0"><xsl:value-of select="concat(substring(@PatientBirthDate, 1, 4), '/', substring(@PatientBirthDate, 5, 2), '/', substring(@PatientBirthDate, 7, 2))" /></xsl:if><xsl:text>&#x000A;</xsl:text>
    <xsl:text>Age=</xsl:text><xsl:call-template name="calculateAge">
      <xsl:with-param name="afterYear" select="substring(sd:Study/@StudyDate, 1, 4)"/>
    </xsl:call-template><xsl:text>&#x000A;</xsl:text>
    <xsl:if test="count(./sd:Study)=1"><xsl:apply-templates select="sd:Study" /></xsl:if>
  </xsl:template>
</xsl:stylesheet>
