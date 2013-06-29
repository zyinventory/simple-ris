function getQueryString(name)
{
	var reg = new RegExp("(^|&)" + name + "=([^&]*)(&|$)", "i");
	var r = window.location.search.substr(1).match(reg);
	if (r != null) return unescape(r[2]);
	return null;
}

function hashCode(str)
{
	var hash = 0;
	var strKey = '';
	if(typeof(str) == 'string')
		strKey = str;
	
	if(strKey != null && strKey != '') {
		for (var i = 0; i < strKey.length; i++) {
			hash = (hash * 31 + strKey.charCodeAt(i)) & 0xFFFFFFFF;
		}
	}
	return hash;
}

function toHex(hash) {
	hash &= 0xFFFFFFFF;
	if(hash < 0)
		return (hash >>> 0).toString(16);
	else
		return hash.toString(16);
}

function toHashPath(param) {
	hashStr = toHex(hashCode(param)).toUpperCase();
	var len = 8 - hashStr.length;
	var str = '';
	for(var i = 0; i < len; ++i) str += '0';
	str += hashStr;
	return str.substring(0, 2) + '/' + str.substring(2, 4) + '/' + str.substring(4, 6) + '/' + str.substring(6, 8);
}
