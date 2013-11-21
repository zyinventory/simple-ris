<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template match="/charge_status/key">
    <li xmlns="http://www.w3.org/1999/xhtml">
      密码:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status/old_counter">
    <li xmlns="http://www.w3.org/1999/xhtml">
      充值前数量:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status/increase">
    <li xmlns="http://www.w3.org/1999/xhtml">
      充值数:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status/error_infos">
    <li xmlns="http://www.w3.org/1999/xhtml">
      错误:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status">
    <html xmlns="http://www.w3.org/1999/xhtml">
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=gbk" />
        <link rel="stylesheet" href="../styles/smoothness/jquery-ui-1.9.1.custom.min.css" />
        <script src="../scripts/jquery-1.8.2.min.js"></script>
        <script src="../scripts/jquery-ui-1.9.1.custom.min.js"></script>
        <script type="text/javascript">
        //<![CDATA[
          function addValue(){
            var pro = $( "#progressbar1" );
            var newValue = pro.progressbar("value") +10;
            pro.progressbar("value",newValue); //设置新值
            if( newValue >= 100) {return;}    //超过100时，返回
            setTimeout( addValue, 600); //延迟500毫秒重复调用自己
          }
          function startBar(){
            var pro = $( "#progressbar1" );
            pro.css('display', 'block');
            addValue();
          }
          jQuery(document).ready(function () {
            $("#progressbar1").progressbar({ value:0, disable:false });
          });
        //]]>
        </script>
        <style type="text/css">
          .failed { color:red; }
          ul li{ list-style-type:disc; }
        </style>
        <title>
          <xsl:value-of select="license_counter"/>
        </title>
      </head>
      <body>
        <h1>
          可刻录光盘数量:<xsl:value-of select="license_counter"/>
        </h1>
        <p>系统编号:<xsl:value-of select="lock_number"/></p>
        <form  method="POST" action="getindex.exe">
          <label for="charge">序列号:</label><input id="seq" type="text" name="seq" size="5" maxlength="20"></input>，
          <label for="charge">密码:</label><input id="password" type="text" name="password" maxlength="20"></input>
          <input id="charge" type="hidden" name="charge" value="charge"></input>
          <input type="submit" value="充值" onclick="startBar()"/>
        </form>
        <div id="progressbar1" style="display:none">
          <div class="ui-progressbar-value" style="background-color:transparent; float:left;">请稍等...</div>
        </div>
        <p>
          <xsl:if test="error_infos">
            <xsl:attribute name="class">failed</xsl:attribute>
            操作失败
          </xsl:if>
          <xsl:if test="old_counter">
            操作成功
          </xsl:if>
        </p>
        <ul>
          <xsl:if test="error_infos">
            <xsl:attribute name="class">failed</xsl:attribute>
          </xsl:if>
          <xsl:apply-templates select="key"/>
          <xsl:apply-templates select="old_counter"/>
          <xsl:apply-templates select="increase"/>
          <xsl:apply-templates select="error_infos"/>
        </ul>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
