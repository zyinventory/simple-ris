#include "stdafx.h"
#include <direct.h>
#include <lock.h>
#include <liblock.h>

static char pPacsBase[MAX_PATH];
std::ostringstream buffer;
int statusXml(CSimpleIni &ini, const char *statusFlag);
int statusCharge(const char *flag);

#define CHINESE_LOCAL "chinese"  // full name: Chinese_People's Republic of China.936, posix: zh_CN.GBK
static char patientID[65], studyUID[65], host[65], indexPath[MAX_PATH];
static const char jnlp0[] = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<jnlp spec=\"1.6+\" codebase=\"http://";
static const char jnlp1[] =           "/weasis-1.2.x\">\
	<information>\
		<title>Weasis</title>\
		<vendor>Weasis Team</vendor>\
		<description>DICOM images viewer</description>\
		<description kind=\"short\">An application to visualize and analyze DICOM images.</description>\
		<description kind=\"one-line\">DICOM images viewer</description>\
		<description kind=\"tooltip\">Weasis</description>\
		<icon href=\"images/logo-button.png\" kind=\"default\" />\
		<icon href=\"images/about.png\" kind=\"splash\" />\
		<shortcut online=\"false\">\
			<desktop />\
			<menu submenu=\"Weasis\" />\
		</shortcut>\
	</information>\
	<security><all-permissions /></security>\
	<resources>\
		<j2se version=\"1.6.0_10+\" href=\"http://java.sun.com/products/autodl/j2se\" initial-heap-size=\"128m\" max-heap-size=\"1024m\" />\
		<j2se version=\"1.6.0_10+\" initial-heap-size=\"128m\" max-heap-size=\"1024m\" />\
		<property name=\"jnlp.packEnabled\" value=\"true\" />\
		<jar href=\"weasis-launcher.jar\" main=\"true\" />\
		<jar href=\"felix.jar\" />\
		<extension href=\"substance.jnlp\" />\
	</resources>\
	<application-desc main-class=\"org.weasis.launcher.WebstartLauncher\">\
		<argument>-VMPfelix.config.properties=\"http://";
static const char jnlp2[] =								"/weasis-1.2.x/conf/config.properties\"</argument>\
		<argument>-VMPfelix.extended.config.properties=\"http://";
static const char jnlp3[] =										"/weasis-1.2.x/conf/ext-config.properties\"</argument>\
		<argument>-VMPweasis.codebase.url=\"http://";
static const char jnlp4[] =							"/weasis-1.2.x\"</argument>\
		<argument>-VMPgosh.args=\"-sc telnetd -p 17179 start\"</argument>\
		<argument>-VMPapple.laf.useScreenMenuBar=\"true\"</argument>\
		<argument>-VMPweasis.i18n=\"http://";
static const char jnlp5[] =					"/weasis-i18n\"</argument>"\
	   "<argument>";
static const char jnlp6[] ="$dicom:close -a</argument>\
		<argument>$dicom:get -w http://";
static const char jnlp7[] =				"/pacs/cgi-bin/getindex.exe?";
static const char jnlp8[] =											"</argument>\
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

	int contentLength = sizeof(jnlp0) - 1 + sizeof(jnlp1) - 1 + sizeof(jnlp2) - 1 + sizeof(jnlp3) - 1 + sizeof(jnlp4) - 1 + sizeof(jnlp5) - 1 + hostLength * 5
		+ (indexPathLength > 0 ? sizeof(jnlp6) - 1 + sizeof(jnlp7) - 1 + hostLength + indexPathLength : 0)
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
	fprintf(cgiOut, host);
	fprintf(cgiOut, jnlp5);
	if(indexPathLength > 0)
	{
		fprintf(cgiOut, jnlp6);
		fprintf(cgiOut, host);
		fprintf(cgiOut, jnlp7);
		fprintf(cgiOut, indexPath);
	}
	fprintf(cgiOut, jnlp8);

	return 0;
}

int queryXml(int hostLength)
{
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		int hashStudy = hashCode(studyUID);
		sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%02X\\%02X\\%02X\\%02X\\%s.xml",
			hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
	}
	else if(cgiFormNotFound != cgiFormString("patientID", patientID, 65) && strlen(patientID) > 0)
	{
		int hashPatient = hashCode(patientID);
		sprintf_s(indexPath, MAX_PATH, "indexdir\\00100020\\%02X\\%02X\\%02X\\%02X\\%s.xml",
			hashPatient >> 24 & 0xff, hashPatient >> 16 & 0xff, hashPatient >> 8 & 0xff, hashPatient & 0xff, patientID);
	}
	else
	{
		if(cgiQueryString) buffer << "无效请求:" << cgiQueryString << endl;
		if(pPacsBase) buffer << "PACS_BASE:" << pPacsBase << endl;;
		outputContent(true);
		return 0;
	}

	char *filebuffer = NULL, pattern[] = "localhost";
	size_t bufferSize = 0;
	HANDLE hFile = CreateFile(indexPath, GENERIC_READ | FILE_WRITE_ATTRIBUTES, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fsi;
		if(GetFileInformationByHandle(hFile, &fsi))
		{
			DWORD toRead = 0, bufferSize = fsi.nFileSizeLow;
			filebuffer = new char[bufferSize + 1];
			ReadFile(hFile, filebuffer, bufferSize, &toRead, NULL);
			if(bufferSize == toRead)
				filebuffer[bufferSize] = '\0';
			else
				filebuffer[0] = '\0';
			
			FILE_BASIC_INFO fbi;
			if(GetFileInformationByHandleEx(hFile, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO)))
			{
				GetSystemTimeAsFileTime((FILETIME*)&fbi.LastAccessTime);
				SetFileInformationByHandle(hFile, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO));
			}
		}
		CloseHandle(hFile);
	}

	if(filebuffer && strlen(filebuffer))
	{
		char *p = strstr(filebuffer, pattern);
		*p = '\0';  // buffer end with: <wado_query wadoURL="http://
		p += sizeof(pattern) - 1;  // p start with: /pacs/" requireOnlySOPInstanceUID="false" ...

		size_t contentLen = strlen(filebuffer) + hostLength + strlen(p);
		fprintf(cgiOut, "Content-type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", contentLen);

		fprintf(cgiOut, filebuffer);
		fprintf(cgiOut, host);
		fprintf(cgiOut, p);
		delete filebuffer;
		return 0;
	}
	else
	{
		buffer << "文件没找到" << endl;
		outputContent(true);
		return -1;
	}
}

static void exitHook()
{
	TerminateLock(0);
}

int burningStudy(const char *media)
{
	ostringstream errstream;
	char rw_passwd[9] = "";
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		char countBuffer[12] = "", filename[64] = "..\\etc\\*.key";
		int lockNumber = -1;
		SEED_SIV siv;
		if(InitiateLock(0))
		{
			atexit(exitHook);
			lockNumber = getLockNumber(filename, FALSE, filename + 7);
		}
		else
			errstream << "init lock failed:" << hex << LYFGetLastErr() << endl;

		if(lockNumber != -1 && 0 == loadPublicKeyContent(filename, &siv, lockNumber, NULL, rw_passwd))
		{
			if(!invalidLock("..\\etc\\license.key", filename, &siv))
			{
				int licenseCount = currentCount(rw_passwd);
				if(licenseCount > 0)
				{
					int result = generateStudyJDF("0020000d", studyUID, errstream, media);
					if(result == 0)
					{
						decreaseCount(rw_passwd);
						cgiHeaderLocation("getindex.exe?status=html");
						cgiHeaderContentType("text/html");
						//char okMessage[] = "开始刻录CD/DVD...";
						//fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", sizeof(okMessage) - 1);
						//fprintf(cgiOut, okMessage);
						return 0;
					}
					else
						errstream << "生成光盘刻录任务失败:" << result << endl;
				}
				else
					errstream << "可刻录光盘数不足:" << licenseCount << endl;
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

int reportCharge(const char *flag)
{
	if(!InitiateLock(0))
	{
		char errorMessage[] = "没有加密锁";
		fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", sizeof(errorMessage) - 1);
		fprintf(cgiOut, errorMessage);
		return -1;
	}
	CoInitialize(NULL);
	int result = statusCharge(flag);
	CoUninitialize();
	TerminateLock(0);
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
	char flag[32];
	if(cgiFormNotFound != cgiFormString("media", flag, sizeof(flag)) && strlen(flag) > 0)
		return burningStudy(flag);
	else if(cgiFormNotFound != cgiFormString("jnlp", flag, sizeof(flag)) && strlen(flag) > 0)
		return jnlp(hostLength);
	else if(cgiFormNotFound != cgiFormString("status", flag, sizeof(flag)) && strlen(flag) > 0)
		return reportStatus(flag);
	else if(cgiFormNotFound != cgiFormString("charge", flag, sizeof(flag)) && strlen(flag) > 0)
		return reportCharge(flag);
	else
		return queryXml(hostLength);
}

extern "C" int cppWrapper()
{
	return work();
}
