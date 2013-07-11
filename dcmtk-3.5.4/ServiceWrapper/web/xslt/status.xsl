<?xml version="1.0" encoding="gb2312"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template name="transformStackMediaType">
    <xsl:param name="media"/>
    <xsl:choose>
      <xsl:when test="$media=1">CD</xsl:when>
      <xsl:when test="$media=4">DVD</xsl:when>
      <xsl:when test="$media=7">双层DVD</xsl:when>
      <xsl:when test="$media=8">蓝光盘</xsl:when>
      <xsl:when test="$media=9">双层蓝光盘</xsl:when>
      <xsl:when test="$media=100">源</xsl:when>
      <xsl:otherwise>未知</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformStackUsageType">
    <xsl:param name="usage"/>
    <xsl:choose>
      <xsl:when test="$usage=0">输出舱</xsl:when>
      <xsl:when test="$usage=99">未使用</xsl:when>
      <xsl:otherwise>未知</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformMode">
    <xsl:param name="mode"/>
    <xsl:choose>
      <xsl:when test="$mode=1">标准模式</xsl:when>
      <xsl:when test="$mode=2">外部输出模式</xsl:when>
      <xsl:when test="$mode=3">批量模式</xsl:when>
      <xsl:when test="$mode=4">错误率度量模式</xsl:when>
      <xsl:otherwise>未知</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="/tdb_status/PUBLISHER1">
    <dt>设备信息</dt>
    <dd>ID：<xsl:value-of select="ID"/></dd>
    <dd>名称：<xsl:value-of select="NAME"/></dd>
    <dd>序列号：<xsl:value-of select="SERIAL_NUMBER"/></dd>
    <dd>模式：<xsl:call-template name="transformMode"><xsl:with-param name="mode" select="MODE"/></xsl:call-template></dd>
    <dd>
      盘舱1介质：<xsl:call-template name="transformStackMediaType">
        <xsl:with-param name="media" select="STACKER1_SETTING"/>
      </xsl:call-template>
    </dd>
    <dd>
        盘舱2介质：<xsl:call-template name="transformStackMediaType">
          <xsl:with-param name="media" select="STACKER2_SETTING"/>
        </xsl:call-template>
    </dd>
    <dd>
      盘舱3用途：<xsl:call-template name="transformStackUsageType">
        <xsl:with-param name="usage" select="STACKER3_SETTING"/>
      </xsl:call-template>
    </dd>
    <dd>
      盘舱4用途：<xsl:call-template name="transformStackUsageType">
        <xsl:with-param name="usage" select="STACKER4_SETTING"/>
      </xsl:call-template>
    </dd>
  </xsl:template>
  <xsl:template match="/tdb_status/TDB_INFO">
    <dt>设备接口</dt>
    <dd>ID：<xsl:value-of select="TDB_ID/text()"/></dd>
    <dd>版本：<xsl:value-of select="VERSION/text()"/></dd>
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
        <title>设备状态</title>
      </head>
      <body>
        <dl>
          <xsl:apply-templates select="/tdb_status"/>
        </dl>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
