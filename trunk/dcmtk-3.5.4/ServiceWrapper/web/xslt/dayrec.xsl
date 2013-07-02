<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
<xsl:template name="calculateAge">
	<xsl:param name="afterYear" />
	<xsl:if test="string-length(@PatientBirthDate)>0"><xsl:value-of select="$afterYear - round(@PatientBirthDate div 10000)" /></xsl:if>
</xsl:template>
<xsl:template name="displaySex">
	<xsl:choose>
		<xsl:when test="@PatientSex='M'">男</xsl:when>
		<xsl:when test="@PatientSex='F'">女</xsl:when>
		<xsl:otherwise>-</xsl:otherwise></xsl:choose>
</xsl:template>
<xsl:template name="replaceFunc">
	<xsl:param name="text"/>
	<xsl:param name="replace"/>
	<xsl:param name="by"/>
	<xsl:choose>
		<xsl:when test="contains($text,$replace)">
			<xsl:value-of select="substring-before($text,$replace)"/>
			<xsl:value-of select="$by"/>
			<xsl:call-template name="replaceFunc">
				<xsl:with-param name="text" select="substring-after($text,$replace)"/>
				<xsl:with-param name="replace" select="$replace"/>
				<xsl:with-param name="by" select="$by"/>
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$text"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>
<xsl:template match="/">
<!-- html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=gbk" />
	<style type="text/css">
		table	{ border-style:solid; border-width:1px; margin:1px; }
		thead	{ background-color:#aaa; }
		tr:nth-child(even)	{ background-color:#ccc; }
	</style>
	<title>Test Table</title>
</head>
<body -->
	<table class="dynamicLoad">
		<thead>
			<tr>
				<th>患者ID</th>
				<th>姓名</th>
				<th>年龄</th>
				<th>性别</th>
				<th>设备</th>
				<th>检查日期</th>
				<th>系列</th>
				<th>图像</th>
				<th title="Accession Number">受理号</th>
				<th>AE Title</th>
				<th>检查UID</th>
        <th>刻录</th>
			</tr>
		</thead>
		<tbody>
			<xsl:for-each select="Collection/Study">
			<tr>
				<td class="patientID">
          <a>
            <xsl:attribute name="href">
              cgi-bin/getindex.exe?jnlp=1&amp;patientID=<xsl:value-of select="@PatientID" />
            </xsl:attribute>
            <xsl:value-of select="@PatientID" />
          </a>
        </td>
				<td class="patientName"><xsl:value-of select="@PatientName" /></td>
				<td class="age">
					<xsl:attribute name="title"><xsl:value-of select="@PatientBirthDate" /></xsl:attribute>
					<xsl:call-template name="calculateAge">
						<xsl:with-param name="afterYear" select="substring(@StudyDate, 1, 4)"/>
					</xsl:call-template>
				</td>
				<td class="sex"><xsl:call-template name="displaySex" /></td>
				<td class="modality"><xsl:value-of select="@Modality" /></td>
				<td class="studyDate"><xsl:value-of select="@StudyDate" /></td>
				<td class="seriesCount"><xsl:value-of select="@SeriesCount" /></td>
				<td class="instanceCount"><xsl:value-of select="@InstanceCount" /></td>
				<td class="accessionNumber"><xsl:value-of select="@AccessionNumber" /></td>
				<td class="callingAE"><xsl:value-of select="@CallingAE" /></td>
				<td class="studyUID">
					<xsl:attribute name="title"><xsl:value-of select="./text()" /></xsl:attribute>
					<a>
						<xsl:attribute name="href">cgi-bin/getindex.exe?jnlp=1&amp;studyUID=<xsl:value-of select="./text()" /></xsl:attribute>
						<xsl:call-template name="replaceFunc">
							<xsl:with-param name="text" select="text()"/>
							<xsl:with-param name="replace" select="'.'"/>
							<xsl:with-param name="by" select="'.&#x200B;'"/>
						</xsl:call-template>
					</a>
        </td>
        <td>
          <form target="_blank" method="POST" action="cgi-bin/getindex.exe">
            <input type="hidden" name="media" value="CD" />
            <input type="hidden" name="studyUID">
              <xsl:attribute name="value"><xsl:value-of select="./text()" /></xsl:attribute>
            </input>
            <input type="submit" value="刻录" />
          </form>
        </td>
			</tr>
			</xsl:for-each>
		</tbody>
	</table>
<!-- /body>
</html -->
</xsl:template>
</xsl:stylesheet>
