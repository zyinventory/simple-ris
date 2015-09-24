function getQueryString(name)
{
	var reg = new RegExp("(^|&)" + name + "=([^&]*)(&|$)", "i");
	var r = window.location.search.substr(1).match(reg);
	if (r != null) return unescape(r[2]);
	return null;
}

function hashCode(str, seed)
{
	var hash = 0;
	var strKey = '';
	if(typeof(str) == 'string')
		strKey = str;
	
	if(strKey != null && strKey != '') {
		for (var i = 0; i < strKey.length; i++) {
			hash = (hash * seed + strKey.charCodeAt(i)) & 0xFFFFFFFF;
		}
    }
    if (hash < 0)
        return hash >>> 0;
    else
        return hash;
}

function hashConcat(param) {
    var h31 = hashCode(param, 31);
    var h131 = hashCode(param, 131);
    h31 *= 512; // <<= 9
    h131 >>= 23;
    h131 &= 0x1FF;
    return h31 + h131;
}

function toHashPath(param) {
	hashStr = hashConcat(param).toString(36).toUpperCase();
	var len = 8 - hashStr.length;
	var str = '';
	for(var i = 0; i < len; ++i) str += '0';
	str += hashStr;
	return str.substring(0, 2) + '/' + str.substring(2, 4) + '/' + str.substring(4, 6) + '/' + str.substring(6, 8);
}

var picker, xslDoc, xslDoc2, xslDocCommon;
var mode = getQueryString('mode');
if (!mode) mode = 'receive';

function viewStudy(event) {
    var studyUid = $('td.studyUID', event.currentTarget).attr('title');
    var studyDate = $('td.studyDate', event.currentTarget).text();
    var link = location.href;
    link = link.replace(/\/[^\/]*$/, '/indexdir/0020000d/' + toHashPath(studyUid) + '/' + studyUid + '.xml');
    if (getQueryString('debug') == '1') alertMessage(link);
    //$('applet#socketProxy')[0].setText(link);
}

function toggleHighlight(event) {
    $('td', event.currentTarget).toggleClass('highlight');
}

function alertMessage(targetObject) {
    var MaxLine = 10;
    var console = $('#console');
    var leftstr = '<div><span class="ui-icon ui-icon-alert" style="float:left; margin-right:0.5em;"></span><span>';
    var myDate = new Date();
    leftstr += ("0" + myDate.getHours()).slice(-2) + ':' + ("0" + myDate.getMinutes()).slice(-2) + ':' + ("0" + myDate.getSeconds()).slice(-2) + ' | ';
    var rightstr = '</span></div>';
    if (typeof (targetObject) == 'string' || typeof (targetObject) == 'boolean' || typeof (targetObject) == 'number') {
        $('#console').prepend(leftstr + targetObject + rightstr);
    }
    else {
        var attrs = '';
        for (var i in targetObject) {
            attrs += i;
            attrs += ':';
            var tagtype = typeof (targetObject[i]);
            if (tagtype == 'number' || tagtype == 'string' || tagtype == 'boolean')
                attrs += targetObject[i];
            else
                attrs += tagtype;
            attrs += '<br/>';
        }
        $('#console').prepend(leftstr + attrs + rightstr);
    }
    var consoleDivs = $('div', console);
    if (consoleDivs.size() > MaxLine) $('#console div:gt(' + --MaxLine + ')').remove();
}

function xslt(xml, xsl, outputDOM) {
    if (window.ActiveXObject)    // IE
    {
        if (outputDOM) {
            var outputXml = new ActiveXObject('Msxml2.DOMDocument.3.0');
            outputXml.async = false;
            xml.transformNodeToObject(xsl, outputXml);
            return outputXml;
        }
        else
            return xml.transformNode(xsl);
    }
    else if ("ActiveXObject" in window) {
        var xslt = new ActiveXObject("Msxml2.XSLTemplate");
        var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
        var xmlszer = new XMLSerializer();
        xslDoc.loadXML(xmlszer.serializeToString(xsl));
        if (!xslDoc.documentElement.getAttribute('xmlns:sd'))  // if XMLSerializer lost attribute xmlns:sd, append it 
            xslDoc.documentElement.setAttribute('xmlns:sd', 'http://www.weasis.org/xsd');
        xslt.stylesheet = xslDoc;
        var xslProc = xslt.createProcessor();
        xslProc.input = xml;
        xslProc.transform();
        if (outputDOM) {
            var dom = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
            dom.loadXML(xslProc.output);
            return dom;
        }
        else
            return xslProc.output;
    }
    else    // FireFox， Chrome
    {
        try {
            var xsltProcessor = new XSLTProcessor();
            xsltProcessor.importStylesheet(xsl);
            var xmlszer = new XMLSerializer();
            var content = xsltProcessor.transformToDocument(xml);
            if (outputDOM)
                return content;
            else
                return xmlszer.serializeToString(content);
        } catch (ex) {
            alertMessage(ex);
        }
    }
}

function transform(xmlDoc) {
    var body = $('#containerBody');
    body.html(xslt(xmlDoc, xslDoc));
    $('tbody tr').mouseover(toggleHighlight).mouseout(toggleHighlight); //.click(viewStudy);
}

function displayContent(textValue, pickerWrapper) {
    var result = $.get('indexdir/' + mode + '/' + textValue + '.xml', {}, transform, 'xml');
    if (result.status != 200) {
        $('#containerBody').html("没有检查");
        if (getQueryString('debug') == '1') alertMessage(textValue + ' 没有检查, ' + result.statusText);
    }
    $('form.confirmDelete').submit(function () {
        this.mode.value = mode;
        if (mode == 'receive') this.receiveDate.value = textValue;
        return confirm('是否删除检查？');
    });
}

function displayPaitent(textValue) {
    var result = $.get('indexdir/' + mode + '/' + textValue + '.xml', {}, function (xmlDoc) {
        transform(xslt(xmlDoc, xslDoc2, true));
    }, 'xml');
    if (result.status != 200) {
        $('#containerBody').html("没有检查");
        if (getQueryString('debug') == '1') alertMessage(textValue + ' 没有检查, ' + result.statusText);
    }
    $('form.confirmDelete').submit(function () {
        return confirm('是否删除检查？');
    });
}

function setColorBar(colorBar, color) {
    $(colorBar).each(function (i, elm) {
        var colorPercent = $(elm).children('div.progress-label')[0].textContent;
        var colorRemain = parseInt(colorPercent.substring(colorPercent.indexOf(')') + 1, colorPercent.indexOf('%')));
        $(elm).progressbar({ value: colorRemain, disable: false });
        $(elm).children('.ui-progressbar-value').css('background', color);
    });
}
