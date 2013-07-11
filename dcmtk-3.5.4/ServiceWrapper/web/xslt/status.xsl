<?xml version="1.0" encoding="gb2312"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template name="transformStackMediaType">
    <xsl:param name="media"/>
    <xsl:choose>
      <xsl:when test="$media=1">CD</xsl:when>
      <xsl:when test="$media=4">DVD</xsl:when>
      <xsl:when test="$media=7">˫��DVD</xsl:when>
      <xsl:when test="$media=8">������</xsl:when>
      <xsl:when test="$media=9">˫��������</xsl:when>
      <xsl:when test="$media=100">Դ</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformStackUsageType">
    <xsl:param name="usage"/>
    <xsl:choose>
      <xsl:when test="$usage=0">�����</xsl:when>
      <xsl:when test="$usage=99">δʹ��</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformMode">
    <xsl:param name="mode"/>
    <xsl:choose>
      <xsl:when test="$mode=1">��׼ģʽ</xsl:when>
      <xsl:when test="$mode=2">�ⲿ���ģʽ</xsl:when>
      <xsl:when test="$mode=3">����ģʽ</xsl:when>
      <xsl:when test="$mode=4">�����ʶ���ģʽ</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="/tdb_status/PUBLISHER1">
    <dt>�豸��Ϣ</dt>
    <dd>ID��<xsl:value-of select="ID"/></dd>
    <dd>���ƣ�<xsl:value-of select="NAME"/></dd>
    <dd>���кţ�<xsl:value-of select="SERIAL_NUMBER"/></dd>
    <dd>ģʽ��<xsl:call-template name="transformMode"><xsl:with-param name="mode" select="MODE"/></xsl:call-template></dd>
    <dd>
      �̲�1���ʣ�<xsl:call-template name="transformStackMediaType">
        <xsl:with-param name="media" select="STACKER1_SETTING"/>
      </xsl:call-template>
    </dd>
    <dd>
        �̲�2���ʣ�<xsl:call-template name="transformStackMediaType">
          <xsl:with-param name="media" select="STACKER2_SETTING"/>
        </xsl:call-template>
    </dd>
    <dd>
      �̲�3��;��<xsl:call-template name="transformStackUsageType">
        <xsl:with-param name="usage" select="STACKER3_SETTING"/>
      </xsl:call-template>
    </dd>
    <dd>
      �̲�4��;��<xsl:call-template name="transformStackUsageType">
        <xsl:with-param name="usage" select="STACKER4_SETTING"/>
      </xsl:call-template>
    </dd>
  </xsl:template>
  <xsl:template match="/tdb_status/TDB_INFO">
    <dt>�豸�ӿ�</dt>
    <dd>ID��<xsl:value-of select="TDB_ID/text()"/></dd>
    <dd>�汾��<xsl:value-of select="VERSION/text()"/></dd>
  </xsl:template>
  <xsl:template match="/tdb_status">
    <xsl:apply-templates select="PUBLISHER1"/>
    <xsl:apply-templates select="TDB_INFO"/>
  </xsl:template>
  <xsl:template match="/">
    <html xmlns="http://www.w3.org/1999/xhtml">
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=gbk" />
        <style type="text/css">
          table	{ border-style:solid; border-width:1px; margin:1px; }
          thead	{ background-color:#aaa; }
          tr:nth-child(even)	{ background-color:#ccc; }
        </style>
        <title>�豸״̬</title>
      </head>
      <body>
        <dl>
          <xsl:apply-templates select="/tdb_status"/>
        </dl>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
