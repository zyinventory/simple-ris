#include "stdafx.h"
#include <iomanip>
#include <direct.h>
#include <lock.h>
#include <liblock.h>

static const char *pPacsBase;
std::ostringstream index_errlog;
int statusCharge(const char *flag);
int removeStudy(const char *flag);

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

void outputContent(bool isText)
{
	string content = index_errlog.str();
	if(isText)
		fprintf(cgiOut, "Content-Type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", content.length());
	else
		fprintf(cgiOut, "Content-Type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", content.length());
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
	fprintf(cgiOut, "Content-Type: application/x-java-jnlp-file; charset=UTF-8\r\nContent-Length: %d\r\n\r\n", contentLength);
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
	char hashBuf[9];
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		__int64 hashStudy = HashStr(studyUID, hashBuf, sizeof(hashBuf));
		sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
	}
	else if(cgiFormNotFound != cgiFormString("patientID", patientID, 65) && strlen(patientID) > 0)
	{
		__int64 hashPatient = HashStr(patientID, hashBuf, sizeof(hashBuf));
		sprintf_s(indexPath, MAX_PATH, "indexdir\\00100020\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], patientID);
	}
	else
	{
		if(cgiQueryString) index_errlog << "无效请求:" << cgiQueryString << endl;
		if(pPacsBase) index_errlog << "PACS_BASE:" << pPacsBase << endl;;
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
		fprintf(cgiOut, "Content-Type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", contentLen);

		fprintf(cgiOut, filebuffer);
		fprintf(cgiOut, host);
		fprintf(cgiOut, p);
		delete filebuffer;
		return 0;
	}
	else
	{
		index_errlog << "文件没找到" << endl;
		outputContent(true);
		return -1;
	}
}

static void exitHook()
{
	TerminateLock(0);
}

#ifdef _DEBUG
static void exitHookDumpMem()
{
	_CrtDumpMemoryLeaks();
}
#endif

template<class FN> static int AuthenWrapper(ostream &errlog, const char *rw_passwd, FN fn)
{
	int licenseCount = currentCount(rw_passwd);
	if(licenseCount > 0)
	{
		int result = fn(errlog);
		if(result == 0)
		{
			decreaseCount(rw_passwd);
			return 0;
		}
		else
		{
			char errmsg[1024];
			strerror_s(errmsg, result);
			errlog << "生成光盘刻录任务失败:" << errmsg << endl;
		}
	}
	else
		errlog << "可刻录光盘数不足:" << licenseCount << endl;

	return -1;
}

static int burningStudy(const char *media)
{
	char rw_passwd[9] = "";
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		char path[MAX_PATH];
		SeriesInstancePath(studyUID, studyUID, path, MAX_PATH);

		char countBuffer[12] = "", filename[64] = "..\\etc\\*.key";
		int lockNumber = -1;
		SEED_SIV siv;
		if(InitiateLock(0))
		{
			atexit(exitHook);
			lockNumber = getLockNumber(filename, FALSE, filename + 7, 64 - 7);
		}
		else
			index_errlog << "init lock failed:" << hex << LYFGetLastErr() << endl;

		if(lockNumber != -1 && 0 == loadPublicKeyContentRW(filename, &siv, lockNumber, rw_passwd))
		{
			if(!invalidLock("..\\etc\\license.key", filename, &siv))
			{
				int licenseCount = currentCount(rw_passwd);
				if(licenseCount > 0)
				{
					int result = generateStudyJDF("0020000d", studyUID, index_errlog, media);
					if(result == 0)
					{
						decreaseCount(rw_passwd);
						cgiHeaderLocation("getindex.exe?status=html");
						cgiHeaderContentType("text/html");
						//char okMessage[] = "开始刻录CD/DVD...";
						//fprintf(cgiOut, "Content-Type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", sizeof(okMessage) - 1);
						//fprintf(cgiOut, okMessage);
						return 0;
					}
					else
						index_errlog << "生成光盘刻录任务失败:" << result << endl;
				}
				else
					index_errlog << "可刻录光盘数不足:" << licenseCount << endl;
			}
			else
				index_errlog << "此程序没有合法的授权" << endl;
		}
		else
			index_errlog << "此程序没有合法的授权" << endl;
	}
	string errmsg = index_errlog.str();
	fprintf(cgiOut, "Content-Type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", errmsg.length());
	fprintf(cgiOut, errmsg.c_str());
	return -1;
}

static int reportStatus(const char *flag)
{
	int result = StatusXml(flag, TDB_STATUS, licenseCounter(), index_errlog);
	outputContent(result);
	return result;
}

static int reportCharge(const char *flag)
{
	if(!InitiateLock(0))
	{
		char errorMessage[] = "没有加密锁";
		fprintf(cgiOut, "Content-Type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", sizeof(errorMessage) - 1);
		fprintf(cgiOut, errorMessage);
		return -1;
	}
	int result = statusCharge(flag);
	TerminateLock(0);
	return result;
}

static void splitVolume(list<Volume> &vols, list<Study> &studies, const size_t volumeSize, const char *desc)
{
	size_t seq = vols.size() + 1;
	for(list<Study>::const_iterator it = studies.begin(); it != studies.end(); ++it)
	{
		if(it->size > volumeSize)
		{
			index_errlog << it->uid << "大于" << volumeSize << "MB, 无法分卷" << endl;
			continue;
		}
		bool stored = false;
		for(list<Volume>::iterator itvol = vols.begin(); itvol != vols.end(); ++itvol)
		{
			if(it->size < itvol->remain)
			{
				stored = itvol->push_back(*it, index_errlog);
				break;
			}
		}
		if(!stored) vols.push_back(Volume(seq++, volumeSize, desc, *it, index_errlog));
		vols.sort([](const Volume &v1, const Volume &v2) { return v1.remain < v2.remain; });
	}
	size_t volCount = vols.size();
	for_each(vols.begin(), vols.end(), [&volCount](Volume &v) { v.volumeCount = volCount; });
}

#define JOB_ID_MAX 40
static bool generateJDF(Volume &vol, char *volbufNoSeq, const char *mediaType, const string &jdfpath, bool isPatient, char *jobPrefix, const char *timeString)
{
    // jdfpath = ..\\tdd\\<job_id>_<seq>.jdf
    // volbufNoSeq = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>. + isPatient ? "" : "seq."
	ofstream ofs(jdfpath);
	if(ofs.good())
	{
		char valid_publisher[256];
		bool valid_found = SelectValidPublisher(TDB_STATUS, valid_publisher, sizeof(valid_publisher), false);
		if(valid_found || strstr(valid_publisher, "error:") == NULL)
			ofs << "PUBLISHER=" << valid_publisher << endl;
		size_t jobpos = strlen(jobPrefix);
		ofs << "JOB_ID=" << timeString << "_" << vol.sequence << endl;
		ofs << "FORMAT=UDF102" << endl;
		ofs << "DISC_TYPE=" << mediaType << endl;
		ofs << "COPIES=1" << endl;
		ofs << "DATA=" << pPacsBase << "\\viewer" << endl;

        // DATA = $PacsBase\\pacs\\indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>.seq.dir
        ofs << "DATA=" << pPacsBase << "\\pacs\\" << volbufNoSeq;
        if(isPatient) ofs << vol.sequence << ".";
        ofs << "dir\tDICOMDIR" << endl;

        for(list<Study>::const_iterator its = vol.studiesOnVolume.begin(); its != vol.studiesOnVolume.end(); ++its)
			ofs << "DATA=" << pPacsBase << "\\pacs\\"  << its->path << "\\" << its->hash << "\t" << its->hash << endl;
		ofs << "VOLUME_LABEL=SMARTPUB" << endl;
		ofs << "LABEL=" << pPacsBase << (isPatient ? "\\tdd\\patientInfo.tdd" : "\\tdd\\batchInfo.tdd") << endl;
        // volbufNoSeq = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>. + isPatient ? "" : "seq."
		ofs << "REPLACE_FIELD=" << pPacsBase << "\\pacs\\" << volbufNoSeq << "txt" << endl;
		ofs.close();
		return true;
	}
	else
	{
		index_errlog << "ERROR: can't create " << jdfpath << endl;
		return false;
	}
}

static void prepareDicomDirAndBurn(list<Volume> &vols, char *volbuf, const size_t prefixLen, const char *mediaType, bool isPatient, char *jobPrefix)
{
#if (!defined _DEBUG) && (!defined SKIP_ENCRYPTION_KEY)
	char rw_passwd[9] = "";
	if(InitiateLock(0))
	{
		atexit(exitHook);
		char filename[64] = "..\\etc\\*.key";
		int lockNumber = -1;
		SEED_SIV siv;
		lockNumber = getLockNumber(filename, FALSE, filename + 7, 64 - 7);
		if(lockNumber == -1 || 0 != loadPublicKeyContentRW(filename, &siv, lockNumber, rw_passwd))
		{
			index_errlog << "init lock failed:" << hex << LYFGetLastErr() << endl;
			return;
		}
		if(invalidLock("..\\etc\\license.key", filename, &siv))
		{
			index_errlog << "init lock failed:" << hex << LYFGetLastErr() << endl;
			return;
		}
	}
	else
	{
		index_errlog << "init lock failed:" << hex << LYFGetLastErr() << endl;
		return;
	}
#endif //(!defined _DEBUG) && (!defined SKIP_ENCRYPTION_KEY)
    char timeBuffer[36];
    GetNextUniqueNo("job_", timeBuffer, sizeof(timeBuffer));

	for(list<Volume>::iterator itv = vols.begin(); itv != vols.end(); ++itv)
	{
		ostringstream file_name_strm;
		for(list<Study>::iterator its = itv->studiesOnVolume.begin(); its != itv->studiesOnVolume.end(); ++its)
		{
			size_t pathlen = strlen(its->path);
			strcat_s(its->path, "\\DICOMDIR");
			file_name_strm << its->path << endl;
			its->path[pathlen] = '\0';
		}
        // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\STUDY_UID | [hash]\\PATIENT_ID>
        // prefixLen = strlen(volbuf);
        sprintf_s(volbuf + prefixLen, MAX_PATH - prefixLen, ".%03d.dir", itv->sequence);
        // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\STUDY_UID | [hash]\\PATIENT_ID>.seq.dir
        string file_names(file_name_strm.str());
        file_name_strm.str("");
        if(MergeDicomDir(file_names.c_str(), volbuf, "SMART_PUB_SET", index_errlog, false) != 0) 
		{
			index_errlog << "Volume " << itv->sequence << " prepare failed." << endl;
			continue;
		}

		volbuf[prefixLen + 5] = '\0';
        // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>.seq.

		if(isPatient)
			volbuf[prefixLen + 1] = '\0';
            // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>.
		else
		{
			strcpy_s(volbuf + prefixLen + 5, MAX_PATH - prefixLen - 5, "txt");
            // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>.seq.txt
			ofstream ofs(volbuf, ios_base::out | ios_base::trunc);
			if(ofs.good())
			{
				ofs << "Description=" << itv->description << endl;
				ofs << "Sequence=" << itv->sequence << endl;
				ofs << "VolumeCount=" << itv->volumeCount << endl;
				ofs.close();
			}
			else
			{
				index_errlog << "create field file " << volbuf << " failure." << endl;
				continue;
			}
			volbuf[prefixLen + 5] = '\0';
            // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>.seq.
		}
		stringstream jdfpathstrm("..\\tdd\\");
        jdfpathstrm << timeBuffer << "_" << itv->sequence << ".jdf";
        string jdfpath = jdfpathstrm.str();
        // jdfpath = ..\\tdd\\<job_id>_<seq>.jdf
        // volbuf[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>. + isPatient ? "" : "seq."
		if(generateJDF(*itv, volbuf, mediaType, jdfpath, isPatient, jobPrefix, timeBuffer))
		{
			itv->valid = true;
			// execute burning
			const Volume &vol = *itv;
			const char *timeString = timeBuffer;
			if(!vol.valid) continue;
#if (!defined _DEBUG) && (!defined SKIP_ENCRYPTION_KEY)
			AuthenWrapper(index_errlog, rw_passwd, [&vol, &jdfpath, timeString](ostream &errlog)-> int{
#endif //(!defined _DEBUG) && (!defined SKIP_ENCRYPTION_KEY)
				char buffer[MAX_PATH];
				sprintf_s(buffer, "..\\orders\\%s_%d.jdf", timeString, vol.sequence);
#if (defined _DEBUG) || (defined SKIP_ENCRYPTION_KEY)
                rename(jdfpath.c_str(), buffer);
#else
				return rename(jdfpath.c_str(), buffer);
			});
#endif //(defined _DEBUG) || (defined SKIP_ENCRYPTION_KEY)
		}
		else
			index_errlog << "create JDF file " << jdfpath << " failure." << endl;
	}
}

#define IndexPrefix "indexdir\\"
int batchBurn()
{
	HANDLE mh = NULL;
	size_t volumeSize = 0;
	char xmlpath[MAX_PATH] = IndexPrefix, *postfix;
	postfix = xmlpath + sizeof(IndexPrefix) - 1;
	const char *mediaType = detectMediaType(NULL);
	list<Volume> vols;
	if(cgiFormNotFound != cgiFormString("volumeSize", postfix, MAX_PATH - (sizeof(IndexPrefix) - 1)) && strlen(postfix) > 0)
	{
		volumeSize = atoi(postfix);
		if(volumeSize == 0)
		{
			index_errlog << "参数错误，没有分卷大小参数(VolumeSize)" << endl;
			goto end_of_process;
		}
	}
	if(cgiFormNotFound != cgiFormString("matchTag", postfix, MAX_PATH - (sizeof(IndexPrefix) - 1)) && strlen(postfix) > 0)
	{
		char desc[64], jobPrefix[JOB_ID_MAX + 1];
		bool isPatient = false;
		char *tag;
		if(strcmp("receive", postfix) == 0 || strcmp("00080020", postfix) == 0) //传输日期 || 检查日期
		{
			if(*postfix == 'r')
			{
				strcpy_s(desc,  "传输日期:");
				strcpy_s(jobPrefix, "ReceiveDate-");
			}
			else
			{
				strcpy_s(desc,  "检查日期:");
				strcpy_s(jobPrefix, "StudyDate-");
			}
			tag = desc + 9;
		}
		else if(strcmp("00100020", postfix) == 0) //Patient ID
		{
			strcpy_s(desc,  "患者ID:");
			strcpy_s(jobPrefix, "PatientID-");
			tag = desc + 7;
			isPatient = true;
		}
		else
		{
			index_errlog << "参数错误，没有选择条件" << endl;
			goto end_of_process;
		}
		size_t pathlen = strlen(xmlpath);
		postfix = xmlpath + pathlen;
		*postfix++ = '\\';
		*postfix = '\0'; //xmlpath[] = indexdir\\<receive | 00080020 | 00100020>\\ |
		++pathlen;       // pathlen = strlen(xmlpath);                          postfix
        
		if(cgiFormNotFound != cgiFormString("matchValue", postfix, MAX_PATH - pathlen) && strlen(postfix) > 0)
		{   // *postfix = FormString["matchValue"]
			char mutexName[MAX_PATH];
			sprintf_s(mutexName, "Global\\%s%s", jobPrefix, postfix);
			mh = CreateMutex(NULL, FALSE, mutexName);
			if(mh == NULL)
			{
				DWORD errcode = GetLastError();
				index_errlog << "无法创建互斥对象" << mutexName << ", error code " << hex << errcode << endl;
				goto end_of_process;
			}
			DWORD waitResult = WaitForSingleObject(mh, 0);
			if(waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED)
			{
				index_errlog << "已有相同的任务在运行:" << jobPrefix << postfix << endl;
				goto end_of_process;
			}
			strcat_s(desc, postfix);
			if(isPatient)
			{
				char hashBuf[9];
				__int64 hashUid36 = HashStr(postfix, hashBuf, sizeof(hashBuf));
				sprintf_s(xmlpath + pathlen, MAX_PATH - pathlen, "%c%c\\%c%c\\%c%c\\%c%c\\%s",
					hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], tag);
			}
			else
			{
				for(char *p = postfix; *p != '\0'; ++p)
					if(*p == '/') *p = '\\';
			}
			size_t prefixLen = strlen(xmlpath);
			strcpy_s(xmlpath + prefixLen, MAX_PATH - prefixLen, ".xml");
            // xmlpath[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID> .xml
			list<Study> studies;
			collectionToFileNameList(xmlpath, studies, isPatient);
			vols.push_back(Volume(1, volumeSize, desc));
			splitVolume(vols, studies, volumeSize, desc);
			vols.sort([](const Volume &v1, const Volume &v2) { return v1.sequence < v2.sequence; });
			xmlpath[prefixLen] = '\0';
            // xmlpath[] = indexdir\\ <receive | 00080020 | 00100020> \\ <YYYY\\MM\\DD | [hash]\\PATIENT_ID>
            // prefixLen = strlen(xmlpath);
			prepareDicomDirAndBurn(vols, xmlpath, prefixLen, mediaType, isPatient, jobPrefix);
		}
		else
		{
			if(isPatient)
				index_errlog << "参数错误，没有患者ID" << endl;
			else
				index_errlog << "参数错误，没有日期" << endl;
		}
	}
end_of_process:
	if(mh)
	{
		ReleaseMutex(mh);
		CloseHandle(mh);
	}
	string errbuf = index_errlog.str();
	if(errbuf.length() == 0)
	{
		cgiHeaderLocation("getindex.exe?status=html");
		cgiHeaderContentType("text/html");
		return 0;
	}
	else
	{
		for(list<Volume>::const_iterator it = vols.begin(); it != vols.end(); ++it) it->print(index_errlog);
		outputContent(true);
		return -1;
	}
}
#define CHUNK_BUFFER_SIZE 64*1024
static int wadoRequest(const char *flag)
{
	char *buffer = NULL;
	int status = 404;
	ifstream xmlfile;
	if(cgiFormNotFound == cgiFormString("studyUID", studyUID, 65) || strlen(studyUID) == 0)
		goto study_process_error;
	char hashBuf[9];
	__int64 hashUid36 = HashStr(studyUID, hashBuf, sizeof(hashBuf));
	if(strcmp(flag, "WADO") == 0)
	{
		char seriesUID[65], objectUID[65], instancePath[MAX_PATH];
		const char *contentType = "application/dicom";
		if(cgiFormNotFound == cgiFormString("contentType", seriesUID, 65) || strcmp(seriesUID, contentType))
			goto study_process_error;
		if(cgiFormNotFound == cgiFormString("seriesUID", seriesUID, 65) || strlen(seriesUID) == 0)
			goto study_process_error;
		if(cgiFormNotFound == cgiFormString("objectUID", objectUID, 65) || strlen(objectUID) == 0)
			goto study_process_error;
		int pathlen = sprintf_s(instancePath, MAX_PATH, "archdir\\v0000000\\%c%c\\%c%c\\%c%c\\%c%c\\%s\\%s\\",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID, hashBuf);
		SeriesInstancePath(seriesUID, objectUID, instancePath + pathlen, MAX_PATH - pathlen);

		xmlfile.open(instancePath, ios_base::binary);
		if(xmlfile.fail()) goto study_process_error;
		buffer = new char[CHUNK_BUFFER_SIZE];
		fprintf_s(cgiOut, "Content-Type: %s\r\nTransfer-Encoding:chunked\r\n\r\n", contentType);
		do {
			xmlfile.read(buffer, CHUNK_BUFFER_SIZE);
			unsigned int read = static_cast<unsigned int>(xmlfile.gcount());
			if(read == 0) break;
			fprintf_s(cgiOut, "%X\r\n", read);
			size_t written = fwrite(buffer, 1, read, cgiOut);
			written = fwrite("\r\n", 1, 2, cgiOut);
		} while(xmlfile.good());
		fwrite("0\r\n\r\n", 1, 5, cgiOut);
	}
	else //generate study list
	{
		char dirpath[MAX_PATH], xmlpath[MAX_PATH];
		sprintf_s(dirpath, MAX_PATH, "archdir\\v0000000\\%c%c\\%c%c\\%c%c\\%c%c\\%s.dir",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
		sprintf_s(xmlpath, MAX_PATH, "archdir\\v0000000\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
		if(!DicomDir2Xml(dirpath, xmlpath)) goto study_process_error;

		xmlfile.open(xmlpath, ios_base::binary);
		if(xmlfile.fail()) goto study_process_error;
		istream::pos_type pos = xmlfile.seekg(0, ios_base::end).tellg();
		unsigned int length = (streamoff)pos;
		xmlfile.seekg(0, ios_base::beg);
		buffer = new char[length];
		if(xmlfile.read(buffer, pos).fail()) goto study_process_error;
		fprintf(cgiOut, "Content-Type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", length);
		size_t written = fwrite(buffer, 1, length, cgiOut);
	}
	status = 0;
study_process_error:
	if(xmlfile.is_open()) xmlfile.close();
	if(buffer) delete[] buffer;
	if(status > 0) cgiHeaderStatus(status, "Study Not Found");
	return status;
}

int work()
{
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	atexit(exitHookDumpMem);
#endif
	//locale::global(locale(CHINESE_LOCAL));
    pPacsBase = GetPacsBase();
	int chdirOK = ChangeToPacsWebSub(NULL, 0);
	if(chdirOK < 0)
	{
		index_errlog << "init working dir failed:" << -1 << ',' << chdirOK << endl;
		if(pPacsBase) index_errlog << "PACS_BASE:" << pPacsBase << endl;
		outputContent(true);
		return -1;
	}
	int hostLength = 0;
	if(strcmp(cgiServerPort, "80"))  // host = http://servername[:port]/
		hostLength = sprintf_s(host, 64, "%s:%s", cgiServerName, cgiServerPort);
	else
		hostLength = sprintf_s(host, 64, "%s", cgiServerName);
	char flag[32];
	if(cgiFormNotFound != cgiFormString("mode", flag, sizeof(flag)) && strlen(flag) > 0
		&& 0 == strcmp(flag, "batchBurn"))
		return batchBurn();
	else if(cgiFormNotFound != cgiFormString("media", flag, sizeof(flag)) && strlen(flag) > 0)
		return burningStudy(flag);
	else if(cgiFormNotFound != cgiFormString("jnlp", flag, sizeof(flag)) && strlen(flag) > 0)
		return jnlp(hostLength);
	else if(cgiFormNotFound != cgiFormString("status", flag, sizeof(flag)) && strlen(flag) > 0)
		return reportStatus(flag);
	else if(cgiFormNotFound != cgiFormString("requestType", flag, sizeof(flag)) && strlen(flag) > 0)
		return wadoRequest(flag);
	else if(cgiFormNotFound != cgiFormString("remove", flag, sizeof(flag)) && strlen(flag) > 0)
		return removeStudy(flag);
	else if(cgiFormNotFound != cgiFormString("charge", flag, sizeof(flag)) && strlen(flag) > 0)
		return reportCharge(flag);
	else
		return queryXml(hostLength);
}

extern "C" int cppWrapper()
{
	CoInitialize(NULL);
	int wr = work();
	CoUninitialize();
	return wr;
}
