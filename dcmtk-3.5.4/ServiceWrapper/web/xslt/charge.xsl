<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template match="/charge_status">
    <html xmlns="http://www.w3.org/1999/xhtml">
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=gbk" />
        <style type="text/css">
          ul li{ list-style-type:decimal; }
        </style>
        <title>
          可刻录光盘数量:<xsl:value-of select="license_counter"/>
        </title>
      </head>
      <body>
        <h1>
          可刻录光盘数量:<xsl:value-of select="license_counter"/>
        </h1>
        <form  method="POST" action="cgi-bin/getindex.exe">
          <label for="charge">密码:</label><input id="charge" type="text" name="charge" maxlength="20"></input>
          <input type="submit" value="充值" />
        </form>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
