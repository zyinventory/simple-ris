#include "stdafx.h"
#include <direct.h>
#include "lock.h"

static char pPacsBase[MAX_PATH];
std::ostringstream buffer;
int statusXml(CSimpleIni &ini, const char *statusFlag);

#define CHINESE_LOCAL "chinese"  // full name: Chinese_People's Republic of China.936, posix: zh_CN.GBK
static char patientID[65], studyUID[65], host[65], indexPath[MAX_PATH];
static const char jnlp0[] = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<jnlp spec=\"1.6+\" version=\"1.0.7\" codebase=\"http://";
static const char jnlp1[] =                               "/weasis-1.2.x\" href=\"\">\
<information>\
  <title>Weasis</title>\
  <vendor>H&#244;pitaux Universitaires de Gen&#232;ve (HUG)</vendor>\
  <homepage href=\"docs/help.html\"/>\
  <description>DICOM images viewer</description>\
  <description kind=\"short\">An application to visualize and analyze DICOM images.</description>\
  <description kind=\"one-line\">DICOM images viewer</description>\
  <description kind=\"tooltip\">Weasis</description>\
  <icon href=\"images/logo-button.png\" kind=\"default\" />\
  <icon href=\"images/about.png\" kind=\"splash\" />\
  <shortcut online=\"false\">\
    <desktop/>\
    <menu submenu=\"Weasis\"/>\
  </shortcut>\
</information>\
<security>\
  <all-permissions/>\
</security>\
<resources>\
  <j2se version=\"1.6.0_10+\" href=\"http://java.sun.com/products/autodl/j2se\" initial-heap-size=\"128m\" max-heap-size=\"1024m\" />\
  <j2se version=\"1.6.0_10+\" initial-heap-size=\"128m\" max-heap-size=\"1024m\" />\
  <jar href=\"weasis-launcher.jar\" main=\"true\" />\
  <jar href=\"felix.jar\" />\
  <extension href=\"substance.jnlp\"/>\
  <property name=\"jnlp.packEnabled\" value=\"true\" />\
  <property name=\"felix.config.properties\" value=\"http://";
static const char jnlp2[] =                                   "/weasis-1.2.x/conf/config.properties\" />\
  <property name=\"weasis.codebase.url\" value=\"http://";
static const char jnlp3[] =                               "/weasis-1.2.x\" />\
  <property name=\"gosh.args\" value=\"-sc telnetd -p 17179 start\" />\
  <property name=\"apple.laf.useScreenMenuBar\" value=\"true\" />\
  <property name=\"weasis.i18n\" value=\"http://";
static const char jnlp4[] =                       "/weasis-i18n\" />\
</resources>\
<application-desc main-class=\"org.weasis.launcher.WebstartLauncher\">\
  <argument>$dicom:close -a</argument>";
static const char jnlp5[] =
 "<argument>$dicom:get -w http://";
static const char jnlp6[] =        "/pacs/cgi-bin/getindex.exe?";
static const char jnlp7[] =                                      "</argument>";
static const char jnlp8[] = "\
</application-desc>\
</jnlp>";
using namespace std;

void outputContent(bool error)
{
	string content = buffer.str();
	if(error)
		fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", content.length());
	else
		fprintf(cgiOut, "Content-type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", content.length());
	fprintf(cgiOut, content.c_str());
}

int jnlp(int hostLength)
{
	int indexPathLength = 0;
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
		indexPathLength = sprintf_s(indexPath, MAX_PATH, "studyUID=%s", studyUID);
	else if(cgiFormNotFound != cgiFormString("patientID", patientID, 65) && strlen(patientID) > 0)
		indexPathLength = sprintf_s(indexPath, MAX_PATH, "patientID=%s", patientID);

	int contentLength = sizeof(jnlp0) - 1 + sizeof(jnlp1) - 1 + sizeof(jnlp2) - 1 + sizeof(jnlp3) - 1 + sizeof(jnlp4) - 1 + hostLength * 4
		+ (indexPathLength > 0 ? sizeof(jnlp5) - 1 + sizeof(jnlp6) - 1 + sizeof(jnlp7) - 1 + hostLength + indexPathLength : 0)
		+ sizeof(jnlp8) - 1;
	fprintf(cgiOut, "Content-type: application/x-java-jnlp-file; charset=UTF-8\r\nContent-Length: %d\r\n\r\n", contentLength);
	fprintf(cgiOut, jnlp0);
	fprintf(cgiOut, host);
	fprintf(cgiOut, jnlp1);
	fprintf(cgiOut, host);
	fprintf(cgiOut, jnlp2);
	fprintf(cgiOut, host);
	fprintf(cgiOut, jnlp3);
	fprintf(cgiOut, host);
	fprintf(cgiOut, jnlp4);
	if(indexPathLength > 0)
	{
		fprintf(cgiOut, jnlp5);
		fprintf(cgiOut, host);
		fprintf(cgiOut, jnlp6);
		fprintf(cgiOut, indexPath);
		fprintf(cgiOut, jnlp7);
	}
	fprintf(cgiOut, jnlp8);
	return 0;
}

int queryXml(int hostLength)
{
		ifstream xmlFile;
		if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
		{
			int hashStudy = hashCode(studyUID);
			sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%02X\\%02X\\%02X\\%02X\\%s.xml",
				hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
			xmlFile.open(indexPath, ios_base::in);
		}
		else if(cgiFormNotFound != cgiFormString("patientID", patientID, 65) && strlen(patientID) > 0)
		{
			int hashPatient = hashCode(patientID);
			sprintf_s(indexPath, MAX_PATH, "indexdir\\00100020\\%02X\\%02X\\%02X\\%02X\\%s.xml",
				hashPatient >> 24 & 0xff, hashPatient >> 16 & 0xff, hashPatient >> 8 & 0xff, hashPatient & 0xff, patientID);
			xmlFile.open(indexPath, ios_base::in);
		}
		else
		{
			if(cgiQueryString) buffer << "无效请求:" << cgiQueryString << endl;
			if(pPacsBase) buffer << "PACS_BASE:" << pPacsBase << endl;;
			outputContent(true);
			return 0;
		}

		if(xmlFile.is_open() && xmlFile.good())
		{
			char *buffer, pattern[] = "localhost";
			streamoff filelen = xmlFile.seekg(0, ios_base::end).tellg();
			xmlFile.seekg(0);
			int bufferSize = (int)filelen + 1;
			buffer = new char[bufferSize];
			streamsize count = xmlFile.read(buffer, filelen).gcount();
			xmlFile.close();

			buffer[count] = '\0';
			char *p = strstr(buffer, pattern);
			*p = '\0';  // buffer end with: <wado_query wadoURL="http://
			p += sizeof(pattern) - 1;  // p start with: /pacs/" requireOnlySOPInstanceUID="false" ...

			size_t contentLen = strlen(buffer) + hostLength + strlen(p);
			fprintf(cgiOut, "Content-type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", contentLen);

			fprintf(cgiOut, buffer);
			fprintf(cgiOut, host);
			fprintf(cgiOut, p);
			delete buffer;
			return 0;
		}
		else
			return -1;
}

int burningStudy(const char *media)
{
	ostringstream errstream;
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		int licenseCount = 0;
		int r = rand();
		if(shieldPC(r) == Lock32_Function(r))
		{
			char lockData[16];
			memset(lockData, 0, sizeof(lockData));
			int operateResult = ReadLock(0, (unsigned char*)lockData, lock_passwd);
			if(operateResult == 0)
			{
				licenseCount = atoi(lockData);
				if(licenseCount <= 0 || licenseCount > 9999)
				{
					int result = generateStudyJDF("0020000d", studyUID, errstream, media);
					if(result == 0)
					{
						memset(lockData, 0, sizeof(lockData));
						_itoa_s(--licenseCount, lockData, sizeof(lockData), 10);
						WriteLock(0, (unsigned char*)lockData, lock_passwd);
						cgiHeaderLocation("getindex.exe?status=html");
						cgiHeaderContentType("text/html");
						//char okMessage[] = "开始刻录CD/DVD...";
						//fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", sizeof(okMessage) - 1);
						//fprintf(cgiOut, okMessage);
						return 0;
					}
					else
						errstream << "生成光盘刻录任务失败" << endl;
				}
				else
					errstream << "可刻录光盘数不足" << endl;
			}
			else
				errstream << "此程序没有合法的授权" << endl;
		}
		else
			errstream << "此程序没有合法的授权" << endl;
	}
	string errmsg = errstream.str();
	fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", errmsg.length());
	fprintf(cgiOut, errmsg.c_str());
	return -1;
}

int reportStatus(const char *flag)
{
    CSimpleIni ini(false, false, false);
    //std::ifstream instream;
    //instream.open("..\\orders\\TDBStatus.txt", std::ifstream::in | std::ifstream::binary, _SH_DENYNO);
	SI_Error rc = ini.LoadFile("..\\orders\\TDBStatus.txt");
	//instream.close();
    if (rc < 0) {
		char okMessage[] = "没有任务";
		fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", sizeof(okMessage) - 1);
		fprintf(cgiOut, okMessage);
		return 0;
	}
	CoInitialize(NULL);
	int result = statusXml(ini, flag);
	CoUninitialize();
	return result;
}

int work()
{
	//locale::global(locale(CHINESE_LOCAL));
	int chdirOK = changeWorkingDirectory(0, NULL, pPacsBase);
	if(chdirOK < 0)
	{
		buffer << "init working dir failed:" << -1 << ',' << chdirOK << endl;
		if(pPacsBase) buffer << "PACS_BASE:" << pPacsBase << endl;
		outputContent(true);
		return -1;
	}
	int hostLength = 0;
	if(strcmp(cgiServerPort, "80"))  // host = http://servername[:port]/
		hostLength = sprintf_s(host, 64, "%s:%s", cgiServerName, cgiServerPort);
	else
		hostLength = sprintf_s(host, 64, "%s", cgiServerName);
	char flag[16];
	if(cgiFormNotFound != cgiFormString("media", flag, sizeof(flag)) && strlen(flag) > 0)
		return burningStudy(flag);
	else if(cgiFormNotFound != cgiFormString("jnlp", flag, sizeof(flag)) && strlen(flag) > 0)
		return jnlp(hostLength);
	else if(cgiFormNotFound != cgiFormString("status", flag, sizeof(flag)) && strlen(flag) > 0)
		return reportStatus(flag);
	else
		return queryXml(hostLength);
}

extern "C" int cppWrapper()
{
	return work();
}
