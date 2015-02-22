<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template name="transformProgress">
    <xsl:param name="progress"/>
    <xsl:choose>
      <xsl:when test="$progress=-1">-</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$progress"/>%
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformPrinterStatus">
    <xsl:param name="status"/>
    <xsl:choose>
      <!-- 1:待机-->
      <xsl:when test="$status=1">
        <img style="height:72px; width:64px" src="images/PRT-2.jpg" />
      </xsl:when>
      <!-- 2:正在打印, 3:等待墨干, 4:正在清洁, 6:正在传送, 7:停止 -->
      <xsl:when test="$status=2 or $status=3 or $status=4 or $status=6 or $status=7">
        <img style="height:72px; width:64px" src="images/PRT-1.jpg" />
      </xsl:when>
      <!-- 5:错误 -->
      <xsl:otherwise>
        <img style="height:72px; width:64px" src="images/PRT-3.jpg" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformDriverStatus">
    <xsl:param name="driverId"/>
    <xsl:param name="status"/>
    <xsl:param name="progress"/>
    <xsl:param name="life"/>
    <xsl:choose>
      <!-- 1:待机-->
      <xsl:when test="$status=1">
        <img style="height:72px; width:64px"><xsl:attribute name="src">images/CD<xsl:value-of select="$driverId" />-2.jpg</xsl:attribute></img>
      </xsl:when>
      <!-- 2:正在写入, 3:正在校验, 6:正在传送, 7:停止 -->
      <xsl:when test="$status=2 or $status=3 or $status=6 or $status=7">
        <img style="height:72px; width:64px"><xsl:attribute name="src">images/CD<xsl:value-of select="$driverId" />-1.jpg</xsl:attribute></img>
      </xsl:when>
      <!-- 4:未使用, 5:错误 -->
      <xsl:otherwise>
        <img style="height:72px; width:64px"><xsl:attribute name="src">images/CD<xsl:value-of select="$driverId" />-3.jpg</xsl:attribute></img>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="/tdb_status">
    <div id="logo" style="display:inline-block">
      <img src="images/logo.jpg" align="left" style="height:72px; width:360px" />
    </div>
    <div id="colorGroup" style="display:block; float:right; margin-left:4px">
      <!-- div class="vertical-spliter" style="width:1em; font-size:1.2em; vertical-align:top">墨水</div -->
      <div id="color1" style="display:inline-block">
        <div id="colorRemainC" style="display:inline-block; width:7em; height:1em; margin-bottom:4px"><div class="progress-label">青(C)<xsl:value-of select="PUBLISHER1/INK_C"/>%</div></div><br />
        <div id="colorRemainLC" style="display:inline-block; width:7em; height:1em"><div class="progress-label">浅青(LC)<xsl:value-of select="PUBLISHER1/INK_LC"/>%</div></div>
      </div>
      <xsl:text disable-output-escaping="yes"><![CDATA[&nbsp;]]></xsl:text>
      <div id="color2" style="display:inline-block">
        <div id="colorRemainM" style="display:inline-block; width:7em; height:1em; margin-bottom:4px"><div class="progress-label">品红(M)<xsl:value-of select="PUBLISHER1/INK_M"/>%</div></div><br />
        <div id="colorRemainLM" style="display:inline-block; width:7em; height:1em"><div class="progress-label">浅品红(LM)<xsl:value-of select="PUBLISHER1/INK_LM"/>%</div></div>
      </div>
      <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
      <div id="color3" style="display:inline-block">
        <div id="colorRemainY" style="display:inline-block; width:7em; height:1em; margin-bottom:4px"><div class="progress-label">黄(Y)<xsl:value-of select="PUBLISHER1/INK_Y"/>%</div></div><br />
        <div id="colorRemainK" style="display:inline-block; width:7em; height:1em"><div class="progress-label" style="color:orange">黑(K)<xsl:value-of select="PUBLISHER1/INK_B"/>%</div></div>
      </div>
      <hr style="margin:2px" />
      <div>
        <!-- div style="display:inline-block; width:1em; font-size:1.2em; margin-right:4px; margin-left:4px; vertical-align:top">光盘</div -->
        <div id="stackerUsage" style="display:inline-block">
          <div id="stacker1" style="display:inline-block; width:7em; height:1em">
            <div class="progress-label">
              盘仓(1)<xsl:value-of select="PUBLISHER1/STACKER1"/>%
            </div>
          </div>
          <xsl:text disable-output-escaping="yes">&amp;nbsp;</xsl:text>
          <div id="stacker2" style="display:inline-block; width:7em; height:1em">
            <div class="progress-label">
              盘仓(2)<xsl:value-of select="PUBLISHER1/STACKER2"/>%
            </div>
          </div>
        </div>
        <div style="float:right">
          <xsl:attribute name="title">总发行数:<xsl:value-of select="PUBLISHER1/PRINTED_COPIES" /></xsl:attribute>
          授权数:<xsl:value-of select="LICENSE_COUNTER" />
        </div>
      </div>
    </div>
    <div id="iconGroup" style="display:block; float:right">
      <div style="display:inline-block">
        <xsl:call-template name="transformDriverStatus">
          <xsl:with-param name="driverId" select="1" />
          <xsl:with-param name="status" select="PUBLISHER1/DRIVE1_STATUS"/>
          <xsl:with-param name="progress" select="PUBLISHER1/DRIVE1_PROGRESS"/>
          <xsl:with-param name="life" select="PUBLISHER1/DRIVE1_LIFE"/>
        </xsl:call-template>
        <xsl:call-template name="transformDriverStatus">
          <xsl:with-param name="driverId" select="2" />
          <xsl:with-param name="status" select="PUBLISHER1/DRIVE2_STATUS"/>
          <xsl:with-param name="progress" select="PUBLISHER1/DRIVE2_PROGRESS"/>
          <xsl:with-param name="life" select="PUBLISHER1/DRIVE2_LIFE"/>
        </xsl:call-template>
      </div>
      <div style="display:inline-block">
        <xsl:call-template name="transformPrinterStatus">
          <xsl:with-param name="status" select="PUBLISHER1/PRINTER_STATUS"/>
        </xsl:call-template>
      </div>
    </div>
  </xsl:template>
  <xsl:template match="/">
    <xsl:apply-templates select="/tdb_status"/>
  </xsl:template>
</xsl:stylesheet>
