<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
	xmlns:sd="http://www.weasis.org/xsd">
<xsl:output method="xml" version="1.0" encoding="gbk" indent="yes"/>

<xsl:key name="distinctModalityPerStudy" match="/sd:wado_query/sd:Patient/sd:Study/sd:Series" use="concat(../@StudyInstanceUID, @Modality)" />

<xsl:template name="concatModalities">
	<xsl:param name="study"/>
	<xsl:for-each select="$study/sd:Series[generate-id()=generate-id(key('distinctModalityPerStudy', concat(../@StudyInstanceUID, @Modality)))]">
		<xsl:value-of select="./@Modality" />
		<xsl:if test="position() != last()">,</xsl:if>
	</xsl:for-each>
</xsl:template>

<xsl:template match="/">
<Collection>
	<xsl:for-each select="/sd:wado_query/sd:Patient/sd:Study">
		<Study>
			<xsl:attribute name="PatientID"><xsl:value-of select="../@PatientID" /></xsl:attribute>
			<xsl:attribute name="PatientName"><xsl:value-of select="../@PatientName" /></xsl:attribute>
			<xsl:attribute name="PatientSex"><xsl:value-of select="../@PatientSex" /></xsl:attribute>
			<xsl:attribute name="PatientBirthDate"><xsl:value-of select="../@PatientBirthDate" /></xsl:attribute>
			<xsl:attribute name="AccessionNumber"><xsl:value-of select="./@AccessionNumber" /></xsl:attribute>
			<xsl:attribute name="StudyDescription"><xsl:value-of select="./@StudyDescription" /></xsl:attribute>
			<xsl:attribute name="StudyDate">
				<xsl:value-of select="concat(substring(./@StudyDate, 1, 4), '/', substring(./@StudyDate, 5, 2), '/', substring(./@StudyDate, 7, 2))" />
			</xsl:attribute>
			<xsl:attribute name="Modality">
				<xsl:call-template name="concatModalities">
					<xsl:with-param name="study" select="."/>
				</xsl:call-template>
			</xsl:attribute>
			<xsl:attribute name="SeriesCount"><xsl:value-of select="count(./sd:Series)" /></xsl:attribute>
			<xsl:attribute name="InstanceCount"><xsl:value-of select="count(.//sd:Instance)" /></xsl:attribute>
			<xsl:attribute name="CallingAE"><xsl:value-of select="/sd:wado_query/sd:httpTag[@key='callingAE']/@value" /></xsl:attribute>
			<xsl:value-of select="@StudyInstanceUID" />
		</Study>
	</xsl:for-each>
</Collection>
</xsl:template>
</xsl:stylesheet>
