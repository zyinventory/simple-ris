<?xml version="1.0" encoding="gbk"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template name="transformJobStatus">
    <xsl:param name="status"/>
    ����״̬��<xsl:value-of select ="$status"/>,
    <xsl:choose>
      <xsl:when test="$status=1">����</xsl:when>
      <xsl:when test="$status=2">������</xsl:when>
      <xsl:when test="$status=3">��ͣ</xsl:when>
      <xsl:when test="$status=4">�����</xsl:when>
      <xsl:when test="$status=5">�쳣����</xsl:when>
      <xsl:when test="$status=6">�ܾ�</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformJobDetail">
    <xsl:param name="detail"/>
    ��ϸ��<xsl:value-of select ="$detail"/>,
    <xsl:choose>
      <xsl:when test="$detail=1">������</xsl:when>
      <xsl:when test="$detail=2">�ȴ�����</xsl:when>
      <xsl:when test="$detail=3">������</xsl:when>
      <xsl:when test="$detail=4">��ͣ��</xsl:when>
      <xsl:when test="$detail=5">�ָ���</xsl:when>
      <xsl:when test="$detail=6">ȡ����</xsl:when>
      <xsl:when test="$detail=7">����ͣ</xsl:when>
      <xsl:when test="$detail=8">�ѻָ�</xsl:when>
      <xsl:when test="$detail=9">�����</xsl:when>
      <xsl:when test="$detail=10">�����(�о���)</xsl:when>
      <xsl:when test="$detail=11">�����(�о���,�̴���)</xsl:when>
      <xsl:when test="$detail=12">�û�ȡ��</xsl:when>
      <xsl:when test="$detail=13">������ֹ</xsl:when>
      <xsl:when test="$detail=14">�ܾ�����</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformCode">
    <xsl:param name="code"/>
    �����룺<xsl:value-of select ="$code"/>,
    <xsl:choose>
      <xsl:when test="$code='SYS001'">��������ʧ��</xsl:when>
      <xsl:when test="$code='SYS002'">���豸ͨ��ʧ��</xsl:when>
      <xsl:when test="$code='SYS003'">������ʱ�ռ䲻��</xsl:when>
      <xsl:when test="$code='JDF0100'">�ܾ�����ԭ��JOB_ID ����JOB_ID�Ѵ���</xsl:when>
      <xsl:when test="$code='JDF0101'">�ܾ�����ԭ��JOB_ID ����JOB_ID���ȳ���40���ַ�</xsl:when>
      <xsl:when test="$code='JDF0102'">�ܾ�����ԭ��JOB_ID ����JOB_ID������Ч�ַ���ֻ���ǣ���ĸ�����֣�_��-</xsl:when>
      <xsl:when test="$code='JDF0200'">�ܾ�����ԭ��PUBLISHER �������豸δע��</xsl:when>
      <xsl:when test="$code='JDF0201'">�ܾ�����ԭ��PUBLISHER ������⵽����豸��δָ��ʹ���ĸ��豸</xsl:when>
      <xsl:when test="$code='JDF0202'">�ܾ�����ԭ��PUBLISHER �����豸ģʽ����</xsl:when>
      <xsl:when test="$code='JDF0203'">�ܾ�����ԭ��PUBLISHER ����û�м�⵽�������豸</xsl:when>
      <xsl:when test="$code='JDF0300'">�ܾ�����ԭ��COPIES ����������������</xsl:when>
      <xsl:when test="$code='JDF0400'">�ܾ�����ԭ��OUT_STACKER ��������ִ̲���</xsl:when>
      <xsl:when test="$code='JDF0500'">�ܾ�����ԭ��DISC_TYPE ����ָ������������Դ�ֲ̲���</xsl:when>
      <xsl:when test="$code='JDF0501'">�ܾ�����ԭ��DISC_TYPE ����δָ����������</xsl:when>
      <xsl:when test="$code='JDF0502'">�ܾ�����ԭ��DISC_TYPE ������֧�ֵĹ������ͣ�֧�ֵĹ������ͣ�CD��DVD��DVD-DL��BD(��PP-7050BD)��BD-DL(��PP-7050BD)</xsl:when>
      <xsl:when test="$code='JDF0600'">�ܾ�����ԭ��WRITING_SPEED ����д���ٶȴ���</xsl:when>
      <xsl:when test="$code='JDF0700'">�ܾ�����ԭ��COMPARE ����ֵ����ΪYES��NO</xsl:when>
      <xsl:when test="$code='JDF0800'">�ܾ�����ԭ��CLOSE_DISC ����ֵ����ΪYES��NO</xsl:when>
      <xsl:when test="$code='JDF0900'">�ܾ�����ԭ��DATA ����Դ·����Ŀ��·����δָ��</xsl:when>
      <xsl:when test="$code='JDF0901'">�ܾ�����ԭ��DATA ����ͬһĿ¼���ļ����ظ�</xsl:when>
      <xsl:when test="$code='JDF0902'">�ܾ�����ԭ��DATA ����Ŀ¼�����ļ��������Ϲ�������(�����������Ч�ַ�)</xsl:when>
      <xsl:when test="$code='JDF0903'">�ܾ�����ԭ��DATA ����Դ�ļ���С������������</xsl:when>
      <xsl:when test="$code='JDF0904'">�ܾ�����ԭ��DATA ����Ŀ��Ŀ¼��ȳ���128��</xsl:when>
      <xsl:when test="$code='JDF0905'">�ܾ�����ԭ��DATA �����洢Դ�ļ���������������</xsl:when>
      <xsl:when test="$code='JDF0907'">�ܾ�����ԭ��DATA ����Դ�ļ�������</xsl:when>
      <xsl:when test="$code='JDF0908'">�ܾ�����ԭ��DATA �����޷���ȡԴ�ļ���Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF0909'">�ܾ�����ԭ��DATA ����Դ�ļ�����ʹ����</xsl:when>
      <xsl:when test="$code='JDF0910'">�ܾ�����ԭ��DATA ����Դ�ļ��嵥������</xsl:when>
      <xsl:when test="$code='JDF0911'">�ܾ�����ԭ��DATA �����޷���ȡԴ�ļ��嵥��Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF0912'">�ܾ�����ԭ��DATA ����Դ�ļ��嵥����ʹ����</xsl:when>
      <xsl:when test="$code='JDF0913'">�ܾ�����ԭ��DATA �����洢Դ�ļ��嵥��Դ�ļ���������������</xsl:when>
      <xsl:when test="$code='JDF0914'">�ܾ�����ԭ��DATA �����ļ�����4095M���޷�д��</xsl:when>
      <xsl:when test="$code='JDF1000'">�ܾ�����ԭ��VOLUME_LABEL ��������ʽ����</xsl:when>
      <xsl:when test="$code='JDF1100'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1101'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1103'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1104'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1105'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1106'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1107'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1108'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1150'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1151'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1152'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1153'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1154'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1155'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1156'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1157'">�ܾ�����ԭ��VIDEO ����</xsl:when>
      <xsl:when test="$code='JDF1200'">�ܾ�����ԭ��VIDEO_TITLE ����</xsl:when>
      <xsl:when test="$code='JDF1201'">�ܾ�����ԭ��VIDEO_TITLE ����</xsl:when>
      <xsl:when test="$code='JDF1300'">�ܾ�����ԭ��IMAGE ������֧�ֵ�ͼ���ļ���ʽ</xsl:when>
      <xsl:when test="$code='JDF1301'">�ܾ�����ԭ��IMAGE ����ͼ���ļ���С̫��</xsl:when>
      <xsl:when test="$code='JDF1302'">�ܾ�����ԭ��IMAGE ����ͼ���ļ�������</xsl:when>
      <xsl:when test="$code='JDF1303'">�ܾ�����ԭ��IMAGE �����޷���ȡͼ���ļ���Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF1304'">�ܾ�����ԭ��IMAGE ����ͼ���ļ�����ʹ����</xsl:when>
      <xsl:when test="$code='JDF1305'">�ܾ�����ԭ��IMAGE �����洢ͼ���ļ���������������</xsl:when>
      <xsl:when test="$code='JDF1306'">�ܾ�����ԭ��IMAGE ������Դ�ֵ̲Ĺ������Ͳ���</xsl:when>
      <xsl:when test="$code='JDF1400'">�ܾ�����ԭ��FORMAT ����CD���̸�ʽ����ֻ����ISO9660L2��JOLIET��UDF102</xsl:when>
      <xsl:when test="$code='JDF1401'">�ܾ�����ԭ��FORMAT ����DVD���̸�ʽ����ֻ����UDF102��UDF102_BRIDGE</xsl:when>
      <xsl:when test="$code='JDF1402'">�ܾ�����ԭ��FORMAT ���������̸�ʽ����ֻ����UDF102��UDF260</xsl:when>
      <xsl:when test="$code='JDF1500'">�ܾ�����ԭ��LABEL ������ӡģ���ļ���ʽ����</xsl:when>
      <xsl:when test="$code='JDF1501'">�ܾ�����ԭ��LABEL ������ӡģ���ļ�������</xsl:when>
      <xsl:when test="$code='JDF1502'">�ܾ�����ԭ��LABEL �����޷���ȡ��ӡģ���ļ���Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF1503'">�ܾ�����ԭ��LABEL ������ӡģ���ļ�����ʹ����</xsl:when>
      <xsl:when test="$code='JDF1504'">�ܾ�����ԭ��LABEL �����洢��ӡģ���ļ���������������</xsl:when>
      <xsl:when test="$code='JDF1505'">�ܾ�����ԭ��LABEL ������ӡģ���ļ������õ��ļ�������</xsl:when>
      <xsl:when test="$code='JDF1506'">�ܾ�����ԭ��LABEL ������ӡģ���ļ������õ��ļ��޷���ȡ��Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF1507'">�ܾ�����ԭ��LABEL ������ӡģ���ļ������õ�������������</xsl:when>
      <xsl:when test="$code='JDF1600'">�ܾ�����ԭ��REPLACE_FIELD ������ӡֵ�ļ�������</xsl:when>
      <xsl:when test="$code='JDF1601'">�ܾ�����ԭ��REPLACE_FIELD �����޷���ȡ��ӡֵ�ļ���Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF1602'">�ܾ�����ԭ��REPLACE_FIELD ������ӡֵ�ļ�����ʹ����</xsl:when>
      <xsl:when test="$code='JDF1603'">�ܾ�����ԭ��REPLACE_FIELD �����洢��ӡֵ�ļ���������������</xsl:when>
      <xsl:when test="$code='JDF1604'">�ܾ�����ԭ��REPLACE_FIELD ������ӡֵ�ļ��������󣺰�����Ч�ַ����ֶ�ֵ���ȳ���1024���ַ����ֶ�������255��</xsl:when>
      <xsl:when test="$code='JDF1610'">�ܾ�����ԭ��REPLACE_FIELD ��������ֵ����������淶</xsl:when>
      <xsl:when test="$code='JDF1611'">�ܾ�����ԭ��REPLACE_FIELD ���������ļ�������</xsl:when>
      <xsl:when test="$code='JDF1612'">�ܾ�����ԭ��REPLACE_FIELD �����޷���ȡ�����ļ���Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF1613'">�ܾ�����ԭ��REPLACE_FIELD ���������ļ�����ʹ����</xsl:when>
      <xsl:when test="$code='JDF1614'">�ܾ�����ԭ��REPLACE_FIELD �����洢�����ļ���������������</xsl:when>
      <xsl:when test="$code='JDF1615'">�ܾ�����ԭ��REPLACE_FIELD ��������Ĺؼ��ֲ���ȷ</xsl:when>
      <xsl:when test="$code='JDF1616'">�ܾ�����ԭ��REPLACE_FIELD ���������޷���ӡ</xsl:when>
      <xsl:when test="$code='JDF1620'">�ܾ�����ԭ��REPLACE_FIELD ����ͼ���ļ�������</xsl:when>
      <xsl:when test="$code='JDF1621'">�ܾ�����ԭ��REPLACE_FIELD ������֧�ֵ�ͼ���ļ���ʽ</xsl:when>
      <xsl:when test="$code='JDF1622'">�ܾ�����ԭ��REPLACE_FIELD �����޷���ȡͼ���ļ���Ȩ�޲���</xsl:when>
      <xsl:when test="$code='JDF1623'">�ܾ�����ԭ��REPLACE_FIELD ����ͼ���ļ�����ʹ����</xsl:when>
      <xsl:when test="$code='JDF1624'">�ܾ�����ԭ��REPLACE_FIELD �����洢ͼ���ļ���������������</xsl:when>
      <xsl:when test="$code='JDF1625'">�ܾ�����ԭ��REPLACE_FIELD ����ͼ��ؼ�����Ч</xsl:when>
      <xsl:when test="$code='JDF1700'">�ܾ�����ԭ��AUDIO_TITLE ����</xsl:when>
      <xsl:when test="$code='JDF1701'">�ܾ�����ԭ��AUDIO_TITLE ����</xsl:when>
      <xsl:when test="$code='JDF1800'">�ܾ�����ԭ��AUDIO_PERFORMER ����</xsl:when>
      <xsl:when test="$code='JDF1801'">�ܾ�����ԭ��AUDIO_PERFORMER ����</xsl:when>
      <xsl:when test="$code='JDF1900'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1901'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1910'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1911'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1912'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1913'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1914'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1915'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1916'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1920'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1921'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1930'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1931'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1940'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF1950'">�ܾ�����ԭ��AUDIO_TRACK ����</xsl:when>
      <xsl:when test="$code='JDF2000'">�ܾ�����ԭ��LABEL_AREA ������ӡ���򳬳���Χ��ȡֵ��Χ700��1194(70mm��119.4mm)</xsl:when>
      <xsl:when test="$code='JDF2001'">�ܾ�����ԭ��LABEL_AREA ������ӡ���򳬳���Χ��ȡֵ��Χ180��500(18mm��50mm)</xsl:when>
      <xsl:when test="$code='JDF2300'">�ܾ�����ԭ��PRIORITY �����������ȼ�����Ϊ��HIGH��</xsl:when>
      <xsl:when test="$code='JDF2400'">�ܾ�����ԭ��AUDIO_CATALOG_CODE ����</xsl:when>
      <xsl:when test="$code='JDF2500'">�ܾ�����ԭ��LABEL_TYPE �������̴�ӡ���ͱ���Ϊ1(��ͨ)��2(������)��3(EPSON��֤)</xsl:when>
      <xsl:when test="$code='JDF2501'">�ܾ�����ԭ��LABEL_TYPE ���������̴�ӡ����Ϊ3(EPSON��֤)ʱ����ӡģʽ����Ϊ1(������)</xsl:when>
      <xsl:when test="$code='JDF2600'">�ܾ�����ԭ��PRINT_MODE ������ӡģʽ����Ϊ1(������)��2(����)��3(���)</xsl:when>
      <xsl:when test="$code='JDF2601'">�ܾ�����ԭ��PRINT_MODE ��������ӡģʽΪ2(����)��3(���)ʱ�����̴�ӡ���Ͳ���Ϊ3(EPSON��֤���豸Ĭ��)</xsl:when>
      <xsl:when test="$code='JDF2602'">�ܾ�����ԭ��PRINT_MODE ��������ӡģʽΪ2(����)��3(���)ʱ�����̴�ӡ���Ͳ���Ϊ3(EPSON��֤)</xsl:when>
      <xsl:when test="$code='JDF2603'">�ܾ�����ԭ��PRINT_MODE ����ֻ��PP-100AP����ӡģʽ����Ϊ3(���)</xsl:when>
      <xsl:when test="$code='JDF2700'">�ܾ�����ԭ��IN_STACKER ����Դ�̲�ֻ����1��2��AUTO</xsl:when>
      <xsl:when test="$code='JDF2800'">�ܾ�����ԭ��MEASURE ����ֵֻ����1��2</xsl:when>
      <xsl:when test="$code='JDF2801'">�ܾ�����ԭ��MEASURE �������豸���ڴ����ʶ���ģʽ��ֵֻ����1</xsl:when>
      <xsl:when test="$code='JDF2900'">�ܾ�����ԭ��ARCHIVE_DISC_ONLY ����ֵֻ����YES��NO</xsl:when>
      <xsl:when test="$code='JDF0000'">�ܾ�����ԭ��Others ������������ΪCD��ȴû��ָ�������ļ�</xsl:when>
      <xsl:when test="$code='JDF0001'">�ܾ�����ԭ��Others ������������ΪDVD��ȴû��ָ�������ļ�</xsl:when>
      <xsl:when test="$code='JDF0002'">�ܾ�����ԭ��Others ����û�д�ӡ����</xsl:when>
      <xsl:when test="$code='JDF0003'">�ܾ�����ԭ��Others ������������Ϊ�����̣�ȴû��ָ�������ļ�</xsl:when>

      <xsl:when test="$code='CAN000'">�������ԭ��Cancel ���������������ӡ����ȡ������ʧ��<br/>���飺�����豸</xsl:when>
      <xsl:when test="$code='CAN001'">�������ԭ��Cancel �������̻���<br/>���飺���ȹر��豸���˹�ȡ������Ĺ��̣�Ȼ���ٿ����豸</xsl:when>
      <xsl:when test="$code='CAN002'">�������ԭ��Cancel �����˳�����ʧ��<br/>���飺�����豸</xsl:when>
      <xsl:when test="$code='CAN003'">�������ԭ��Cancel �����޷��ڹ������ӡ���м�⵽����<br/>���飺���ȹر��豸��ȡ���豸���������壬Ȼ���ٿ����豸</xsl:when>
      <xsl:when test="$code='CAN004'">�������ԭ��Cancel ��������̲�(�̲�3)����<br/>����ԭ�򣺵��豸����ģʽ3(����ģʽ)ʱ����ʼ��¼֮ǰδ����̲�3<br/>���飺����̲�3�����հ׹��̷����̲�1���̲�2�����¿�ʼ����</xsl:when>
      <xsl:when test="$code='CAN005'">�������ԭ��Cancel ������е���ƶ�����<br/>���飺���ȹر��豸��ȡ���豸���������壬Ȼ���ٿ����豸</xsl:when>
      <xsl:when test="$code='CAN006'">�������ԭ��Cancel ������е��������<br/>���飺���ȹر��豸��ȡ���豸���������壬Ȼ���ٿ����豸</xsl:when>
      <xsl:when test="$code='CAN007'">�������ԭ��Cancel ������������ʧЧ<br/>���飺���ȹر��豸��ȡ���豸���������壬Ȼ���ٿ����豸</xsl:when>
      <xsl:when test="$code='CAN008'">�������ԭ��Cancel ������������<br/>���飺�����豸</xsl:when>
      <xsl:when test="$code='CAN009'">�������ԭ��Cancel ������ӡ������ʧЧ<br/>���飺���ȹر��豸��ȡ���豸���������壬Ȼ���ٿ����豸</xsl:when>
      <xsl:when test="$code='CAN010'">�������ԭ��Cancel ������������<br/>���飺�����豸</xsl:when>
      <xsl:when test="$code='CAN011'">�������ԭ��Cancel �������ӡ��ͨ��ʧ��<br/>���飺����ӡ���˿�����</xsl:when>
      <xsl:when test="$code='CAN012'">�������ԭ��Cancel ������ӡ�����ƴ���EPJ�ļ�ָ���Ĵ�ӡ��û���ҵ�<br/>���飺��ӡ����������</xsl:when>
      <xsl:when test="$code='CAN013'">�������ԭ��Cancel ������ӡ��ά������<br/>���飺��ά�޷�������ϵ</xsl:when>
      <xsl:when test="$code='CAN014'">�������ԭ��Cancel ������ī����Ҫ����<br/>���飺��ά�޷�������ϵ</xsl:when>
      <xsl:when test="$code='CAN015'">�������ԭ��Cancel ����״̬����<br/>���飺�����豸</xsl:when>
      <xsl:when test="$code='CAN016'">�������ԭ��Cancel ���������޷���ȡ<br/>���飺����ļ���ȡȨ��</xsl:when>
      <xsl:when test="$code='CAN017'">�������ԭ��Cancel ������֧�ֵĹ̼��汾<br/>���飺�����豸�̼�</xsl:when>
      <xsl:when test="$code='CAN018'">�������ԭ��Cancel �����豸ģʽ������ģʽ��һ��<br/>���飺���������豸ģʽ</xsl:when>
      <xsl:when test="$code='CAN019'">�������ԭ��Cancel ����Դ�ļ�·����Ч<br/>���飺���Դ�ļ�·��</xsl:when>
      <xsl:when test="$code='CAN020'">�������ԭ��Cancel �������ӡ��ͨ��ʧ��<br/>���飺���������(USB������)�Ƿ�����</xsl:when>
      <xsl:when test="$code='CAN021'">�������ԭ��Cancel ������������־����ʧ��<br/>���飺�����־����·������д��Ȩ��</xsl:when>

      <xsl:when test="$code='STP000'">�������ԭ��Pause �����������ʹ���򵽴����Դ�������<br/>���飺���հ׹�������</xsl:when>
      <xsl:when test="$code='STP001'">�������ԭ��Pause �����������Դ�������<br/>���飺���հ׹�������������հ׹���������������������ϵά�޷�����</xsl:when>
      <xsl:when test="$code='STP002'">�������ԭ��Pause �����������������ʳ�������<br/>���飺���հ׹�������������հ׹���������������������ϵά�޷�����</xsl:when>

      <xsl:when test="$code='RTN000'">�������ԭ��Automatic recover ����Դ�̲���û�пհ׹���<br/>���飺��Դ�̲��з���հ׹���</xsl:when>
      <xsl:when test="$code='RTN001'">�������ԭ��Automatic recover ��������̲�����<br/>���飺�������̲�</xsl:when>
      <xsl:when test="$code='RTN002'">�������ԭ��Automatic recover �����豸�Ĺ��̸�δ�ر�<br/>���飺�ر��豸�Ĺ��̸�</xsl:when>
      <xsl:when test="$code='RTN003'">�������ԭ��Automatic recover �����豸��īˮ��δ�ر�<br/>���飺�ر��豸��īˮ��</xsl:when>
      <xsl:when test="$code='RTN004'">�������ԭ��Automatic recover ����īˮ����<br/>���飺����ī��</xsl:when>
      <xsl:when test="$code='RTN005'">�������ԭ��Automatic recover ����δ��װī��<br/>���飺��װī��</xsl:when>
      <xsl:when test="$code='RTN006'">�������ԭ��Automatic recover ����δ��װ�̲�<br/>���飺��װ�̲�</xsl:when>
      <xsl:when test="$code='RTN007'">�������ԭ��Automatic recover �����ⲿ���ģʽ����Ҫ�̲�3<br/>���飺ж���̲�3</xsl:when>
      <xsl:when test="$code='RTN008'">�������ԭ��Automatic recover �����̲�4δ�ر�<br/>���飺�ر��̲�4</xsl:when>
      <xsl:when test="$code='RTN009'">�������ԭ��Automatic recover �������Ź��̱����͵�����<br/>���飺�򿪹��̸ǣ�ȡ�����������ϵĹ��̣��رչ��̸�</xsl:when>
      <xsl:when test="$code='RTN010'">�������ԭ��Automatic recover �������Ź��̱����͵���ӡ��<br/>���飺�򿪹��̸ǣ�ȡ�����������ϵĹ��̣��رչ��̸�</xsl:when>
      <xsl:when test="$code='RTN011'">�������ԭ��Automatic recover ����ī���޷�ʶ��<br/>���飺��ȷ��װī��</xsl:when>
      <xsl:when test="$code='RTN012'">�������ԭ��Automatic recover ������Դ���̲�ȡ����ʧ��<br/>���飺�������Ƿ�ճ��</xsl:when>
      <xsl:when test="$code='RTN013'">�������ԭ��Automatic recover ����Դ���̲��еĹ���������������<br/>���飺ȡ������Ĺ���</xsl:when>
      <xsl:when test="$code='RTN014'">�������ԭ��Automatic recover ����ά�����δ�ر�<br/>���飺�ر�ά�����</xsl:when>
      <xsl:when test="$code='RTN015'">�������ԭ��Automatic recover ����ά�������<br/>���飺����ά����</xsl:when>
      <xsl:when test="$code='RTN016'">�������ԭ��Automatic recover ����δ��װά����<br/>���飺��װά����</xsl:when>
      <xsl:when test="$code='RTN017'">�������ԭ��Automatic recover ����ά�����޷�ʶ��<br/>���飺��ȷ��װά����</xsl:when>

      <xsl:when test="$code='OTH000'">�������ԭ��Others �����޷�ȡ������״̬����鿴 INFORMATION Code</xsl:when>
      <xsl:otherwise>δ֪</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformJob">
    <xsl:param name="job"/>
    <dd><xsl:call-template name="transformJobStatus"><xsl:with-param name="status" select="$job/STATUS"/></xsl:call-template></dd>
    <dd><xsl:call-template name="transformJobDetail"><xsl:with-param name="detail" select="$job/DETAIL_STATUS"/></xsl:call-template></dd>
    <dd><xsl:call-template name="transformCode"><xsl:with-param name="code" select="$job/ERROR"/></xsl:call-template></dd>
  </xsl:template>
  <xsl:template  match="/tdb_status/COMPLETE_JOB">
    ���������
    <ul>
      <xsl:for-each select="JOB">
        <dt>����ID��<xsl:value-of select="@id"/></dt>
        <xsl:call-template name="transformJob">
          <xsl:with-param name="job" select="/tdb_status/JOB_STATUS[@id=@id]"/>
        </xsl:call-template>
      </xsl:for-each>
    </ul>
  </xsl:template>
</xsl:stylesheet>
