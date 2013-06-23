#include "stdafx.h"

#define CHINESE_LOCAL "chinese"  // full name: Chinese_People's Republic of China.936, posix: zh_CN.GBK
static char patientID[65], studyUID[65], host[65], indexPath[MAX_PATH];

using namespace std;

int work()
{
	ifstream xmlFile;
	//locale::global(locale(CHINESE_LOCAL));
	changeWorkingDirectory(0, NULL); // switch to ${PACS_BASE}/pacs
	cgiFormString("patientID", patientID, 65);
	if(strlen(patientID) > 0)
	{
		int hashPatient = hashCode(patientID);
		sprintf_s(indexPath, MAX_PATH, "indexdir\\00100020\\%02X\\%02X\\%02X\\%02X\\%s.xml",
			hashPatient >> 24 & 0xff, hashPatient >> 16 & 0xff, hashPatient >> 8 & 0xff, hashPatient & 0xff, patientID);
		xmlFile.open(indexPath, ios_base::in);
	}
	else
	{
		cgiFormString("studyUID", studyUID, 65);
		if(strlen(studyUID) > 0)
		{
			int hashStudy = hashCode(studyUID);
			sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%02X\\%02X\\%02X\\%02X\\%s.xml",
				hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
			xmlFile.open(indexPath, ios_base::in);
		}
	}

	if(xmlFile.is_open() && xmlFile.good())
	{
		char *buffer, pattern[] = "http://localhost/";
		streamoff filelen = xmlFile.seekg(0, ios_base::end).tellg();
		xmlFile.seekg(0);
		int bufferSize = (int)filelen + 1;
		buffer = new char[bufferSize];
		streamsize count = xmlFile.read(buffer, filelen).gcount();
		xmlFile.close();

		buffer[count] = '\0';
		char *p = strstr(buffer, pattern);
		*p = '\0';  // buffer end with: <wado_query wadoURL="
		if(strcmp(cgiServerPort, "80"))  // host = http://servername[:port]/
			sprintf_s(host, 64, "http://%s:%s/", cgiServerName, cgiServerPort);
		else
			sprintf_s(host, 64, "http://%s/", cgiServerName);
		p += sizeof(pattern) - 1;  // p start with: pacs/" requireOnlySOPInstanceUID="false" ...

		size_t contentLen = strlen(buffer) + strlen(host) + strlen(p);
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

extern "C" int cppWrapper()
{
	return work();
}
