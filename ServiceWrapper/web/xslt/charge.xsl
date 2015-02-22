<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template match="/charge_status/key">
    <li xmlns="http://www.w3.org/1999/xhtml">
      ����:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status/old_counter">
    <li xmlns="http://www.w3.org/1999/xhtml">
      ��ֵǰ����:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status/increase">
    <li xmlns="http://www.w3.org/1999/xhtml">
      ��ֵ��:<xsl:value-of select="text()"/>
    </li>
  </xsl:template>
  <xsl:template match="/charge_status/error_infos">
    <li xmlns="http://www.w3.org/1999/xhtml">
      ����:<xsl:value-of select="text()"/>
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
            pro.progressbar("value",newValue); //������ֵ
            if( newValue >= 100) {return;}    //����100ʱ������
            setTimeout( addValue, 600); //�ӳ�500�����ظ������Լ�
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
          �ɿ�¼��������:<xsl:value-of select="license_counter"/>
        </h1>
        <p>ϵͳ���:<xsl:value-of select="lock_number"/></p>
        <form  method="POST" action="getindex.exe">
          <label for="charge">���к�:</label><input id="seq" type="text" name="seq" size="5" maxlength="20"></input>��
          <label for="charge">����:</label><input id="password" type="text" name="password" maxlength="20"></input>
          <input id="charge" type="hidden" name="charge" value="charge"></input>
          <input type="submit" value="��ֵ" onclick="startBar()"/>
        </form>
        <div id="progressbar1" style="display:none">
          <div class="ui-progressbar-value" style="background-color:transparent; float:left;">���Ե�...</div>
        </div>
        <p>
          <xsl:if test="error_infos">
            <xsl:attribute name="class">failed</xsl:attribute>
            ����ʧ��
          </xsl:if>
          <xsl:if test="old_counter">
            �����ɹ�
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
