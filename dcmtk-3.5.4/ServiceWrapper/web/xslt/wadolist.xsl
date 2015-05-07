<?xml version="1.0" encoding="gb2312"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:sd="http://www.weasis.org/xsd">
	<xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
	
	<xsl:key name="distinctModalityPerStudy" match="/sd:wado_query/sd:Patient/sd:Study/sd:Series" use="concat(../@StudyInstanceUID, @Modality)" />
	<xsl:template name="concatModalities">
		<xsl:param name="study"/>
		<xsl:for-each select="$study/sd:Series[generate-id()=generate-id(key('distinctModalityPerStudy', concat(../@StudyInstanceUID, @Modality)))]">
			<xsl:value-of select="./@Modality" />
			<xsl:if test="position() != last()">,</xsl:if>
		</xsl:for-each>
	</xsl:template>
	
	<xsl:template match="sd:Patient">
		<dt xmlns="http://www.w3.org/1999/xhtml">
			<xsl:value-of select="@PatientID"/>:<xsl:value-of select="@PatientName"/>
		</dt>
		<xsl:for-each select="sd:Study">
			<dd xmlns="http://www.w3.org/1999/xhtml">
				<div>
					检查
					<xsl:call-template name="concatModalities">
						<xsl:with-param name="study" select="."/>
					</xsl:call-template>
					:<xsl:value-of select="@StudyInstanceUID"/>
				</div>
				<xsl:for-each select="sd:Series">
					<div class="inner-study">
						<span>系列<xsl:value-of select="@Modality"/>:<xsl:value-of select="@SeriesInstanceUID"/></span>
						<ul class="instance">
							<xsl:for-each select="sd:Instance">
								<li>
									<a>
										<xsl:attribute name="href">
											<xsl:value-of select="@DirectDownloadFile" />
										</xsl:attribute>
										图像<xsl:value-of select="@SOPInstanceUID" />
									</a>
								</li>
							</xsl:for-each>
						</ul>
					</div>
				</xsl:for-each>
			</dd>
		</xsl:for-each>
	</xsl:template>
	<xsl:template match="/sd:wado_query">
		<html xmlns="http://www.w3.org/1999/xhtml">
			<head>
				<meta http-equiv="Content-Type" content="text/html; charset=gbk" />
				<style type="text/css">
					div.inner-study { margin-left:2em }
					ul.instance { margin-top:0px; margin-bottom: 0px; }
				</style>
				<title>WADO列表</title>
			</head>
			<body>
				<xsl:for-each select="sd:Patient">
					<dl>
						<xsl:apply-templates select="."/>
					</dl>
				</xsl:for-each>
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>
