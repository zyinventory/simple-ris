<?xml version="1.0" encoding="gb2312"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template name="transformStackSetting">
    <xsl:param name="usage"/>
    <xsl:param name="percentage"/>
    <xsl:choose>
      <xsl:when test="$usage=0">
        �����,�̲տռ䣺<xsl:value-of select="100-$percentage"/>%
      </xsl:when>
      <xsl:when test="$usage=1">
        CD,�̲տռ䣺<xsl:value-of select="100-$percentage"/>%
      </xsl:when>
      <xsl:when test="$usage=4">
        DVD,�̲տռ䣺<xsl:value-of select="100-$percentage"/>%
      </xsl:when>
      <xsl:when test="$usage=7">
        ˫��DVD,�̲տռ䣺<xsl:value-of select="100-$percentage"/>%
      </xsl:when>
      <xsl:when test="$usage=8">
        ������,�̲տռ䣺<xsl:value-of select="100-$percentage"/>%
      </xsl:when>
      <xsl:when test="$usage=9">
        ˫��������,�̲տռ䣺<xsl:value-of select="100-$percentage"/>%
      </xsl:when>
      <xsl:when test="$usage=99">δʹ��</xsl:when>
      <xsl:when test="$usage=100">Դ</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformPrinterStatus">
    <xsl:param name="status"/>
    <xsl:choose>
      <xsl:when test="$status=1">����</xsl:when>
      <xsl:when test="$status=2">���ڴ�ӡ</xsl:when>
      <xsl:when test="$status=3">�ȴ�ī��</xsl:when>
      <xsl:when test="$status=4">�������</xsl:when>
      <xsl:when test="$status=5">����</xsl:when>
      <xsl:when test="$status=6">���ڴ���</xsl:when>
      <xsl:when test="$status=7">ֹͣ</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformProgress">
    <xsl:param name="progress"/>
    <xsl:choose>
      <xsl:when test="$progress=-1">-</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$progress"/>%
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformDriverStatus">
    <xsl:param name="status"/>
    <xsl:param name="progress"/>
    <xsl:param name="life"/>
    <xsl:choose>
      <xsl:when test="$status=1">����</xsl:when>
      <xsl:when test="$status=2">����д��</xsl:when>
      <xsl:when test="$status=3">����У��</xsl:when>
      <xsl:when test="$status=4">δʹ��</xsl:when>
      <xsl:when test="$status=5">����</xsl:when>
      <xsl:when test="$status=6">���ڴ���</xsl:when>
      <xsl:when test="$status=7">ֹͣ</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$status=2 or $status=3">
      ...
      <xsl:call-template name="transformProgress"><xsl:with-param name="progress" select="$progress"/></xsl:call-template>
    </xsl:if>
    , ����ʣ�
    <xsl:call-template name="transformProgress">
      <xsl:with-param name="progress" select="$life"/>
    </xsl:call-template>
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
    <dt xmlns="http://www.w3.org/1999/xhtml">�豸��Ϣ</dt>
    <dd xmlns="http://www.w3.org/1999/xhtml">ID��<xsl:value-of select="ID"/></dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">���ƣ�<xsl:value-of select="NAME"/></dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">���кţ�<xsl:value-of select="SERIAL_NUMBER"/></dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">ģʽ��<xsl:call-template name="transformMode"><xsl:with-param name="mode" select="MODE"/></xsl:call-template></dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">��ӡ��״̬��<xsl:call-template name="transformPrinterStatus"><xsl:with-param name="status" select="PRINTER_STATUS"/></xsl:call-template></dd>
    <xsl:if test="PRINTED_COPIES!=-1">
      <dd xmlns="http://www.w3.org/1999/xhtml">�Ѵ�ӡ����<xsl:value-of select="PRINTED_COPIES"/></dd>
    </xsl:if>
    <xsl:if test="PRINTABLE_COPIES!=-1">
      <dd xmlns="http://www.w3.org/1999/xhtml">�ɴ�ӡ��(����)��<xsl:value-of select="PRINTABLE_COPIES"/></dd>
    </xsl:if>
    <dd xmlns="http://www.w3.org/1999/xhtml"></dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">
      <span>�̲գ�</span>
      <ul>
        <li>
          <xsl:call-template name="transformStackSetting">
            <xsl:with-param name="usage" select="STACKER1_SETTING"/>
            <xsl:with-param name="percentage" select="STACKER1"/>
          </xsl:call-template>
        </li>
        <li>
          <xsl:call-template name="transformStackSetting">
            <xsl:with-param name="usage" select="STACKER2_SETTING"/>
            <xsl:with-param name="percentage" select="STACKER2"/>
          </xsl:call-template>
        </li>
        <li>
          <xsl:call-template name="transformStackSetting">
            <xsl:with-param name="usage" select="STACKER3_SETTING"/>
            <xsl:with-param name="percentage" select="STACKER3"/>
          </xsl:call-template>
        </li>
        <li>
          <xsl:call-template name="transformStackSetting">
            <xsl:with-param name="usage" select="STACKER4_SETTING"/>
            <xsl:with-param name="percentage" select="STACKER4"/>
          </xsl:call-template>
        </li>
      </ul>
    </dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">
      <span>īˮ��</span>
      <ul>
        <li>�ࣺ<xsl:value-of select="INK_C"/>%</li>
        <li>Ʒ�죺<xsl:value-of select="INK_M"/>%</li>
        <li>�ƣ�<xsl:value-of select="INK_Y"/>%</li>
        <li>ǳ�ࣺ<xsl:value-of select="INK_LC"/>%</li>
        <li>ǳƷ�죺<xsl:value-of select="INK_LM"/>%</li>
        <li>�ڣ�<xsl:value-of select="INK_B"/>%</li>
      </ul>
    </dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">
      <span>������</span>
      <ul>
        <li>
          <xsl:call-template name="transformDriverStatus">
            <xsl:with-param name="status" select="DRIVE1_STATUS"/>
            <xsl:with-param name="progress" select="DRIVE1_PROGRESS"/>
            <xsl:with-param name="life" select="DRIVE1_LIFE"/>
          </xsl:call-template>
        </li>
        <li>
          <xsl:call-template name="transformDriverStatus">
            <xsl:with-param name="status" select="DRIVE2_STATUS"/>
            <xsl:with-param name="progress" select="DRIVE2_PROGRESS"/>
            <xsl:with-param name="life" select="DRIVE2_LIFE"/>
          </xsl:call-template>
        </li>
      </ul>
    </dd>
  </xsl:template>
  <xsl:template match="/tdb_status/TDB_INFO">
    <dt xmlns="http://www.w3.org/1999/xhtml">�豸�ӿ�</dt>
    <dd xmlns="http://www.w3.org/1999/xhtml">ID��<xsl:value-of select="TDB_ID/text()"/></dd>
    <dd xmlns="http://www.w3.org/1999/xhtml">�汾��<xsl:value-of select="VERSION/text()"/></dd>
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
          ul li{ list-style-type:decimal; }
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
