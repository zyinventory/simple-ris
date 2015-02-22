<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output indent="no" method="html" encoding="gbk" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>
  <xsl:template name="transformJobType">
    <xsl:param name="type"/>
    <xsl:choose>
      <xsl:when test="$type=1">仅刻录光盘</xsl:when>
      <xsl:when test="$type=2">仅打印盘面</xsl:when>
      <xsl:when test="$type=3">刻录及打印盘面</xsl:when>
      <xsl:when test="$type=4">错误率度量</xsl:when>
      <xsl:otherwise>未知</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformJobStatus">
    <xsl:param name="status"/>
    状态：<xsl:value-of select ="$status"/>,
    <xsl:choose>
      <xsl:when test="$status=1">待机</xsl:when>
      <xsl:when test="$status=2">处理中</xsl:when>
      <xsl:when test="$status=3">暂停</xsl:when>
      <xsl:when test="$status=4">已完成</xsl:when>
      <xsl:when test="$status=5">异常结束</xsl:when>
      <xsl:when test="$status=6">拒绝</xsl:when>
      <xsl:otherwise>未知</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformJobDetail">
    <xsl:param name="detail"/>
    详细：<xsl:value-of select ="$detail"/>,
    <xsl:choose>
      <xsl:when test="$detail=1">接受中</xsl:when>
      <xsl:when test="$detail=2">等待处理</xsl:when>
      <xsl:when test="$detail=3">处理中</xsl:when>
      <xsl:when test="$detail=4">暂停中</xsl:when>
      <xsl:when test="$detail=5">恢复中</xsl:when>
      <xsl:when test="$detail=6">取消中</xsl:when>
      <xsl:when test="$detail=7">已暂停</xsl:when>
      <xsl:when test="$detail=8">已恢复</xsl:when>
      <xsl:when test="$detail=9">已完成</xsl:when>
      <xsl:when test="$detail=10">已完成(有警告)</xsl:when>
      <xsl:when test="$detail=11">已完成(有警告,盘错误)</xsl:when>
      <xsl:when test="$detail=12">用户取消</xsl:when>
      <xsl:when test="$detail=13">错误中止</xsl:when>
      <xsl:when test="$detail=14">拒绝接受</xsl:when>
      <xsl:otherwise>未知</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="transformCode">
    <xsl:param name="code"/>
    错误码：<xsl:value-of select ="$code"/><xsl:if test="$code">,</xsl:if>
    <xsl:choose>
      <xsl:when test="$code='SYS001'">接受任务失败</xsl:when>
      <xsl:when test="$code='SYS002'">与设备通信失败</xsl:when>
      <xsl:when test="$code='SYS003'">磁盘临时空间不足</xsl:when>
      <xsl:when test="$code='JDF0100'">拒绝接受原因：JOB_ID —〉JOB_ID已存在</xsl:when>
      <xsl:when test="$code='JDF0101'">拒绝接受原因：JOB_ID —〉JOB_ID长度超过40个字符</xsl:when>
      <xsl:when test="$code='JDF0102'">拒绝接受原因：JOB_ID —〉JOB_ID包含无效字符，只能是：字母，数字，_，-</xsl:when>
      <xsl:when test="$code='JDF0200'">拒绝接受原因：PUBLISHER —〉该设备未注册</xsl:when>
      <xsl:when test="$code='JDF0201'">拒绝接受原因：PUBLISHER —〉检测到多个设备，未指定使用哪个设备</xsl:when>
      <xsl:when test="$code='JDF0202'">拒绝接受原因：PUBLISHER —〉设备模式错误</xsl:when>
      <xsl:when test="$code='JDF0203'">拒绝接受原因：PUBLISHER —〉没有检测到已连接设备</xsl:when>
      <xsl:when test="$code='JDF0300'">拒绝接受原因：COPIES —〉复制数量错误</xsl:when>
      <xsl:when test="$code='JDF0400'">拒绝接受原因：OUT_STACKER —〉输出盘仓错误</xsl:when>
      <xsl:when test="$code='JDF0500'">拒绝接受原因：DISC_TYPE —〉指定光盘类型与源盘仓不符</xsl:when>
      <xsl:when test="$code='JDF0501'">拒绝接受原因：DISC_TYPE —〉未指定光盘类型</xsl:when>
      <xsl:when test="$code='JDF0502'">拒绝接受原因：DISC_TYPE —〉不支持的光盘类型，支持的光盘类型：CD、DVD、DVD-DL、BD(仅PP-7050BD)、BD-DL(仅PP-7050BD)</xsl:when>
      <xsl:when test="$code='JDF0600'">拒绝接受原因：WRITING_SPEED —〉写入速度错误</xsl:when>
      <xsl:when test="$code='JDF0700'">拒绝接受原因：COMPARE —〉值必须为YES或NO</xsl:when>
      <xsl:when test="$code='JDF0800'">拒绝接受原因：CLOSE_DISC —〉值必须为YES或NO</xsl:when>
      <xsl:when test="$code='JDF0900'">拒绝接受原因：DATA —〉源路径与目的路径都未指定</xsl:when>
      <xsl:when test="$code='JDF0901'">拒绝接受原因：DATA —〉同一目录下文件名重复</xsl:when>
      <xsl:when test="$code='JDF0902'">拒绝接受原因：DATA —〉目录名或文件名不符合光盘类型(超长或包含无效字符)</xsl:when>
      <xsl:when test="$code='JDF0903'">拒绝接受原因：DATA —〉源文件大小超过光盘容量</xsl:when>
      <xsl:when test="$code='JDF0904'">拒绝接受原因：DATA —〉目的目录深度超过128层</xsl:when>
      <xsl:when test="$code='JDF0905'">拒绝接受原因：DATA —〉存储源文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF0907'">拒绝接受原因：DATA —〉源文件不存在</xsl:when>
      <xsl:when test="$code='JDF0908'">拒绝接受原因：DATA —〉无法读取源文件，权限不足</xsl:when>
      <xsl:when test="$code='JDF0909'">拒绝接受原因：DATA —〉源文件正在使用中</xsl:when>
      <xsl:when test="$code='JDF0910'">拒绝接受原因：DATA —〉源文件清单不存在</xsl:when>
      <xsl:when test="$code='JDF0911'">拒绝接受原因：DATA —〉无法读取源文件清单，权限不足</xsl:when>
      <xsl:when test="$code='JDF0912'">拒绝接受原因：DATA —〉源文件清单正在使用中</xsl:when>
      <xsl:when test="$code='JDF0913'">拒绝接受原因：DATA —〉存储源文件清单中源文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF0914'">拒绝接受原因：DATA —〉文件超过4095M，无法写入</xsl:when>
      <xsl:when test="$code='JDF1000'">拒绝接受原因：VOLUME_LABEL —〉卷标格式错误</xsl:when>
      <xsl:when test="$code='JDF1100'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1101'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1103'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1104'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1105'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1106'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1107'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1108'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1150'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1151'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1152'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1153'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1154'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1155'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1156'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1157'">拒绝接受原因：VIDEO —〉</xsl:when>
      <xsl:when test="$code='JDF1200'">拒绝接受原因：VIDEO_TITLE —〉</xsl:when>
      <xsl:when test="$code='JDF1201'">拒绝接受原因：VIDEO_TITLE —〉</xsl:when>
      <xsl:when test="$code='JDF1300'">拒绝接受原因：IMAGE —〉不支持的图像文件格式</xsl:when>
      <xsl:when test="$code='JDF1301'">拒绝接受原因：IMAGE —〉图像文件大小太大</xsl:when>
      <xsl:when test="$code='JDF1302'">拒绝接受原因：IMAGE —〉图像文件不存在</xsl:when>
      <xsl:when test="$code='JDF1303'">拒绝接受原因：IMAGE —〉无法读取图像文件，权限不足</xsl:when>
      <xsl:when test="$code='JDF1304'">拒绝接受原因：IMAGE —〉图像文件正在使用中</xsl:when>
      <xsl:when test="$code='JDF1305'">拒绝接受原因：IMAGE —〉存储图像文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF1306'">拒绝接受原因：IMAGE —〉与源盘仓的光盘类型不符</xsl:when>
      <xsl:when test="$code='JDF1400'">拒绝接受原因：FORMAT —〉CD光盘格式错误，只能是ISO9660L2、JOLIET或UDF102</xsl:when>
      <xsl:when test="$code='JDF1401'">拒绝接受原因：FORMAT —〉DVD光盘格式错误，只能是UDF102或UDF102_BRIDGE</xsl:when>
      <xsl:when test="$code='JDF1402'">拒绝接受原因：FORMAT —〉蓝光盘格式错误，只能是UDF102或UDF260</xsl:when>
      <xsl:when test="$code='JDF1500'">拒绝接受原因：LABEL —〉打印模板文件格式错误</xsl:when>
      <xsl:when test="$code='JDF1501'">拒绝接受原因：LABEL —〉打印模板文件不存在</xsl:when>
      <xsl:when test="$code='JDF1502'">拒绝接受原因：LABEL —〉无法读取打印模板文件，权限不足</xsl:when>
      <xsl:when test="$code='JDF1503'">拒绝接受原因：LABEL —〉打印模板文件正在使用中</xsl:when>
      <xsl:when test="$code='JDF1504'">拒绝接受原因：LABEL —〉存储打印模板文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF1505'">拒绝接受原因：LABEL —〉打印模板文件中引用的文件不存在</xsl:when>
      <xsl:when test="$code='JDF1506'">拒绝接受原因：LABEL —〉打印模板文件中引用的文件无法读取，权限不足</xsl:when>
      <xsl:when test="$code='JDF1507'">拒绝接受原因：LABEL —〉打印模板文件中引用的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF1600'">拒绝接受原因：REPLACE_FIELD —〉打印值文件不存在</xsl:when>
      <xsl:when test="$code='JDF1601'">拒绝接受原因：REPLACE_FIELD —〉无法读取打印值文件，权限不足</xsl:when>
      <xsl:when test="$code='JDF1602'">拒绝接受原因：REPLACE_FIELD —〉打印值文件正在使用中</xsl:when>
      <xsl:when test="$code='JDF1603'">拒绝接受原因：REPLACE_FIELD —〉存储打印值文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF1604'">拒绝接受原因：REPLACE_FIELD —〉打印值文件解析错误：包含无效字符？字段值长度超过1024个字符？字段数超过255？</xsl:when>
      <xsl:when test="$code='JDF1610'">拒绝接受原因：REPLACE_FIELD —〉条码值不符合条码规范</xsl:when>
      <xsl:when test="$code='JDF1611'">拒绝接受原因：REPLACE_FIELD —〉条码文件不存在</xsl:when>
      <xsl:when test="$code='JDF1612'">拒绝接受原因：REPLACE_FIELD —〉无法读取条码文件，权限不足</xsl:when>
      <xsl:when test="$code='JDF1613'">拒绝接受原因：REPLACE_FIELD —〉条码文件正在使用中</xsl:when>
      <xsl:when test="$code='JDF1614'">拒绝接受原因：REPLACE_FIELD —〉存储条码文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF1615'">拒绝接受原因：REPLACE_FIELD —〉条码的关键字不正确</xsl:when>
      <xsl:when test="$code='JDF1616'">拒绝接受原因：REPLACE_FIELD —〉条码无法打印</xsl:when>
      <xsl:when test="$code='JDF1620'">拒绝接受原因：REPLACE_FIELD —〉图像文件不存在</xsl:when>
      <xsl:when test="$code='JDF1621'">拒绝接受原因：REPLACE_FIELD —〉不支持的图像文件格式</xsl:when>
      <xsl:when test="$code='JDF1622'">拒绝接受原因：REPLACE_FIELD —〉无法读取图像文件，权限不足</xsl:when>
      <xsl:when test="$code='JDF1623'">拒绝接受原因：REPLACE_FIELD —〉图像文件正在使用中</xsl:when>
      <xsl:when test="$code='JDF1624'">拒绝接受原因：REPLACE_FIELD —〉存储图像文件的驱动器不可用</xsl:when>
      <xsl:when test="$code='JDF1625'">拒绝接受原因：REPLACE_FIELD —〉图像关键字无效</xsl:when>
      <xsl:when test="$code='JDF1700'">拒绝接受原因：AUDIO_TITLE —〉</xsl:when>
      <xsl:when test="$code='JDF1701'">拒绝接受原因：AUDIO_TITLE —〉</xsl:when>
      <xsl:when test="$code='JDF1800'">拒绝接受原因：AUDIO_PERFORMER —〉</xsl:when>
      <xsl:when test="$code='JDF1801'">拒绝接受原因：AUDIO_PERFORMER —〉</xsl:when>
      <xsl:when test="$code='JDF1900'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1901'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1910'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1911'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1912'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1913'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1914'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1915'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1916'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1920'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1921'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1930'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1931'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1940'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF1950'">拒绝接受原因：AUDIO_TRACK —〉</xsl:when>
      <xsl:when test="$code='JDF2000'">拒绝接受原因：LABEL_AREA —〉打印区域超出范围，取值范围700—1194(70mm—119.4mm)</xsl:when>
      <xsl:when test="$code='JDF2001'">拒绝接受原因：LABEL_AREA —〉打印区域超出范围，取值范围180—500(18mm—50mm)</xsl:when>
      <xsl:when test="$code='JDF2300'">拒绝接受原因：PRIORITY —〉任务优先级必须为“HIGH”</xsl:when>
      <xsl:when test="$code='JDF2400'">拒绝接受原因：AUDIO_CATALOG_CODE —〉</xsl:when>
      <xsl:when test="$code='JDF2500'">拒绝接受原因：LABEL_TYPE —〉光盘打印类型必须为1(普通)、2(高质量)或3(EPSON认证)</xsl:when>
      <xsl:when test="$code='JDF2501'">拒绝接受原因：LABEL_TYPE —〉当光盘打印类型为3(EPSON认证)时，打印模式必须为1(高质量)</xsl:when>
      <xsl:when test="$code='JDF2600'">拒绝接受原因：PRINT_MODE —〉打印模式必须为1(高质量)、2(快速)或3(最快)</xsl:when>
      <xsl:when test="$code='JDF2601'">拒绝接受原因：PRINT_MODE —〉当打印模式为2(快速)或3(最快)时，光盘打印类型不能为3(EPSON认证，设备默认)</xsl:when>
      <xsl:when test="$code='JDF2602'">拒绝接受原因：PRINT_MODE —〉当打印模式为2(快速)或3(最快)时，光盘打印类型不能为3(EPSON认证)</xsl:when>
      <xsl:when test="$code='JDF2603'">拒绝接受原因：PRINT_MODE —〉只有PP-100AP，打印模式才能为3(最快)</xsl:when>
      <xsl:when test="$code='JDF2700'">拒绝接受原因：IN_STACKER —〉源盘仓只能是1、2或AUTO</xsl:when>
      <xsl:when test="$code='JDF2800'">拒绝接受原因：MEASURE —〉值只能是1或2</xsl:when>
      <xsl:when test="$code='JDF2801'">拒绝接受原因：MEASURE —〉当设备处于错误率度量模式，值只能是1</xsl:when>
      <xsl:when test="$code='JDF2900'">拒绝接受原因：ARCHIVE_DISC_ONLY —〉值只能是YES或NO</xsl:when>
      <xsl:when test="$code='JDF0000'">拒绝接受原因：Others —〉光盘类型为CD，却没有指定数据文件</xsl:when>
      <xsl:when test="$code='JDF0001'">拒绝接受原因：Others —〉光盘类型为DVD，却没有指定数据文件</xsl:when>
      <xsl:when test="$code='JDF0002'">拒绝接受原因：Others —〉没有打印数据</xsl:when>
      <xsl:when test="$code='JDF0003'">拒绝接受原因：Others —〉光盘类型为蓝光盘，却没有指定数据文件</xsl:when>

      <xsl:when test="$code='CAN000'">任务结束原因：Cancel —〉从驱动器或打印机中取出光盘失败<br/>建议：重启设备</xsl:when>
      <xsl:when test="$code='CAN001'">任务结束原因：Cancel —〉光盘滑落<br/>建议：首先关闭设备，人工取出滑落的光盘，然后再开启设备</xsl:when>
      <xsl:when test="$code='CAN002'">任务结束原因：Cancel —〉退出光盘失败<br/>建议：重启设备</xsl:when>
      <xsl:when test="$code='CAN003'">任务结束原因：Cancel —〉无法在光驱或打印机中检测到光盘<br/>建议：首先关闭设备，取出设备中外来物体，然后再开启设备</xsl:when>
      <xsl:when test="$code='CAN004'">任务结束原因：Cancel —〉输出盘仓(盘仓3)已满<br/>可能原因：当设备处于模式3(批量模式)时，开始刻录之前未清空盘仓3<br/>建议：清空盘仓3，将空白光盘放入盘仓1、盘仓2，重新开始任务</xsl:when>
      <xsl:when test="$code='CAN005'">任务结束原因：Cancel —〉机械臂移动错误<br/>建议：首先关闭设备，取出设备中外来物体，然后再开启设备</xsl:when>
      <xsl:when test="$code='CAN006'">任务结束原因：Cancel —〉机械臂马达过热<br/>建议：首先关闭设备，取出设备中外来物体，然后再开启设备</xsl:when>
      <xsl:when test="$code='CAN007'">任务结束原因：Cancel —〉光驱托盘失效<br/>建议：首先关闭设备，取出设备中外来物体，然后再开启设备</xsl:when>
      <xsl:when test="$code='CAN008'">任务结束原因：Cancel —〉光驱错误<br/>建议：重启设备</xsl:when>
      <xsl:when test="$code='CAN009'">任务结束原因：Cancel —〉打印机托盘失效<br/>建议：首先关闭设备，取出设备中外来物体，然后再开启设备</xsl:when>
      <xsl:when test="$code='CAN010'">任务结束原因：Cancel —〉光驱错误<br/>建议：重启设备</xsl:when>
      <xsl:when test="$code='CAN011'">任务结束原因：Cancel —〉与打印机通信失败<br/>建议：检查打印机端口设置</xsl:when>
      <xsl:when test="$code='CAN012'">任务结束原因：Cancel —〉打印机名称错误，EPJ文件指定的打印机没有找到<br/>建议：打印机名称设置</xsl:when>
      <xsl:when test="$code='CAN013'">任务结束原因：Cancel —〉打印机维护错误<br/>建议：与维修服务商联系</xsl:when>
      <xsl:when test="$code='CAN014'">任务结束原因：Cancel —〉废墨垫需要更换<br/>建议：与维修服务商联系</xsl:when>
      <xsl:when test="$code='CAN015'">任务结束原因：Cancel —〉状态错误<br/>建议：重启设备</xsl:when>
      <xsl:when test="$code='CAN016'">任务结束原因：Cancel —〉数据无法读取<br/>建议：检查文件读取权限</xsl:when>
      <xsl:when test="$code='CAN017'">任务结束原因：Cancel —〉不支持的固件版本<br/>建议：升级设备固件</xsl:when>
      <xsl:when test="$code='CAN018'">任务结束原因：Cancel —〉设备模式与任务模式不一致<br/>建议：重新设置设备模式</xsl:when>
      <xsl:when test="$code='CAN019'">任务结束原因：Cancel —〉源文件路径无效<br/>建议：检查源文件路径</xsl:when>
      <xsl:when test="$code='CAN020'">任务结束原因：Cancel —〉与打印机通信失败<br/>建议：检查数据线(USB或网线)是否连接</xsl:when>
      <xsl:when test="$code='CAN021'">任务结束原因：Cancel —〉错误率日志保存失败<br/>建议：检查日志保存路径及其写入权限</xsl:when>

      <xsl:when test="$code='STP000'">任务结束原因：Pause —〉光盘类型错误或到达重试次数上限<br/>建议：检查空白光盘质量</xsl:when>
      <xsl:when test="$code='STP001'">任务结束原因：Pause —〉到达重试次数上限<br/>建议：检查空白光盘质量；如果空白光盘来自正规渠道，请联系维修服务商</xsl:when>
      <xsl:when test="$code='STP002'">任务结束原因：Pause —〉持续错误发生几率超过阈限<br/>建议：检查空白光盘质量；如果空白光盘来自正规渠道，请联系维修服务商</xsl:when>

      <xsl:when test="$code='RTN000'">任务结束原因：Automatic recover —〉源盘仓里没有空白光盘<br/>建议：在源盘仓中放入空白光盘</xsl:when>
      <xsl:when test="$code='RTN001'">任务结束原因：Automatic recover —〉输出盘仓已满<br/>建议：清空输出盘仓</xsl:when>
      <xsl:when test="$code='RTN002'">任务结束原因：Automatic recover —〉设备的光盘盖未关闭<br/>建议：关闭设备的光盘盖</xsl:when>
      <xsl:when test="$code='RTN003'">任务结束原因：Automatic recover —〉设备的墨水盖未关闭<br/>建议：关闭设备的墨水盖</xsl:when>
      <xsl:when test="$code='RTN004'">任务结束原因：Automatic recover —〉墨水不足<br/>建议：更换墨盒</xsl:when>
      <xsl:when test="$code='RTN005'">任务结束原因：Automatic recover —〉未安装墨盒<br/>建议：安装墨盒</xsl:when>
      <xsl:when test="$code='RTN006'">任务结束原因：Automatic recover —〉未安装盘仓<br/>建议：安装盘仓</xsl:when>
      <xsl:when test="$code='RTN007'">任务结束原因：Automatic recover —〉外部输出模式不需要盘仓3<br/>建议：卸下盘仓3</xsl:when>
      <xsl:when test="$code='RTN008'">任务结束原因：Automatic recover —〉盘仓4未关闭<br/>建议：关闭盘仓4</xsl:when>
      <xsl:when test="$code='RTN009'">任务结束原因：Automatic recover —〉多张光盘被传送到光驱<br/>建议：打开光盘盖，取出光驱托盘上的光盘，关闭光盘盖</xsl:when>
      <xsl:when test="$code='RTN010'">任务结束原因：Automatic recover —〉多张光盘被传送到打印机<br/>建议：打开光盘盖，取出光驱托盘上的光盘，关闭光盘盖</xsl:when>
      <xsl:when test="$code='RTN011'">任务结束原因：Automatic recover —〉墨盒无法识别<br/>建议：正确安装墨盒</xsl:when>
      <xsl:when test="$code='RTN012'">任务结束原因：Automatic recover —〉从源光盘仓取光盘失败<br/>建议：检查光盘是否粘连</xsl:when>
      <xsl:when test="$code='RTN013'">任务结束原因：Automatic recover —〉源光盘仓中的光盘数量超过上限<br/>建议：取出多余的光盘</xsl:when>
      <xsl:when test="$code='RTN014'">任务结束原因：Automatic recover —〉维护箱盖未关闭<br/>建议：关闭维护箱盖</xsl:when>
      <xsl:when test="$code='RTN015'">任务结束原因：Automatic recover —〉维护箱过期<br/>建议：更换维护箱</xsl:when>
      <xsl:when test="$code='RTN016'">任务结束原因：Automatic recover —〉未安装维护箱<br/>建议：安装维护箱</xsl:when>
      <xsl:when test="$code='RTN017'">任务结束原因：Automatic recover —〉维护箱无法识别<br/>建议：正确安装维护箱</xsl:when>

      <xsl:when test="$code='OTH000'">任务结束原因：Others —〉无法取得任务状态，请查看 INFORMATION Code</xsl:when>
      <xsl:otherwise></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="dispalyJobImpl">
    <xsl:param name="job"/>
    <dd>
      <xsl:call-template name="transformJobStatus">
        <xsl:with-param name="status" select="$job/STATUS"/>
      </xsl:call-template>
    </dd>
    <dd>
      <xsl:call-template name="transformJobDetail">
        <xsl:with-param name="detail" select="$job/DETAIL_STATUS"/>
      </xsl:call-template>
    </dd>
    <dd>
      <xsl:call-template name="transformCode">
        <xsl:with-param name="code" select="$job/ERROR/text()"/>
      </xsl:call-template>
    </dd>
    <xsl:if test="$job/TYPE">
      <dd>
        类型：<xsl:call-template name="transformJobType"><xsl:with-param name="type" select="$job/TYPE"/></xsl:call-template>
      </dd>
    </xsl:if>
    <xsl:if test="$job/PUBLISHER"><dd>设备：<xsl:value-of select ="$job/PUBLISHER"/></dd></xsl:if>
    <xsl:if test="$job/PUBLICATION_NUMBER"><dd>计划刻录数：<xsl:value-of select ="$job/PUBLICATION_NUMBER"/></dd></xsl:if>
    <xsl:if test="$job/COMPLETION_NUMBER"><dd>完成刻录数：<xsl:value-of select ="$job/COMPLETION_NUMBER"/></dd></xsl:if>
    <xsl:if test="$job/ERRORDISC_NUMBER"><dd>刻录错误数：<xsl:value-of select ="$job/ERRORDISC_NUMBER"/></dd></xsl:if>
    <xsl:if test="$job/IN_STACKER"><dd>源盘仓：<xsl:value-of select ="$job/IN_STACKER"/></dd></xsl:if>
    <xsl:if test="$job/OUT_STACKER"><dd>输出盘仓：<xsl:value-of select ="$job/OUT_STACKER"/></dd></xsl:if>
    <xsl:if test="$job/ESTIMATION_TIME">
      <dd>估算时间：<xsl:value-of select ="floor($job/ESTIMATION_TIME div 60)"/>分<xsl:value-of select ="$job/ESTIMATION_TIME mod 60"/>秒</dd>
    </xsl:if>
    <xsl:if test="$job/ESTIMATION_TIME_REMAIN">
      <dd>估算剩余时间：<xsl:value-of select ="floor($job/ESTIMATION_TIME_REMAIN div 60)"/>分<xsl:value-of select ="$job/ESTIMATION_TIME_REMAIN mod 60"/>秒</dd>
    </xsl:if>
  </xsl:template>
  <xsl:template name="dispalyJob">
    <xsl:param name="jobId"/>
    <xsl:call-template name="dispalyJobImpl">
      <xsl:with-param name="job" select="/tdb_status/JOB_STATUS[@id=$jobId]"/>
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="/tdb_status/ACTIVE_JOB">
    未完成任务：
    <ul>
      <xsl:for-each select="JOB">
        <dt>
          任务ID：<xsl:value-of select="@id"/>
        </dt>
        <xsl:call-template name="dispalyJob">
          <xsl:with-param name="jobId" select="@id"/>
        </xsl:call-template>
      </xsl:for-each>
    </ul>
  </xsl:template>
  <xsl:template match="/tdb_status/COMPLETE_JOB">
    已完成任务：
    <ul>
      <xsl:for-each select="JOB">
        <dt>任务ID：<xsl:value-of select="@id"/></dt>
        <xsl:call-template name="dispalyJob">
          <xsl:with-param name="jobId" select="@id"/>
        </xsl:call-template>
      </xsl:for-each>
    </ul>
  </xsl:template>
</xsl:stylesheet>
