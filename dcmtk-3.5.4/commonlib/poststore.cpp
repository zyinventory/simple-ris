// poststore.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#import <msxml3.dll>

#include <io.h>
#include "commonlib.h"
#include "SimpleIni.h"
#include "poststore.h"

using namespace std;

const char UNIT_SEPARATOR = 0x1F;
const size_t BUFF_SIZE = 1024;
const int RMDIR_WAIT_SECONDS = 10;
static char buffer[BUFF_SIZE];
static string archivePath("archdir");
static string indexBase("indexdir");
static string baseurl, downloadUrl, studyDatePath;

COMMONLIB_API bool CommonlibBurnOnce, CommonlibInstanceUniquePath;

static stringstream errstrm;
static string generateIndexLog;
COMMONLIB_API const char *GetGenerateIndexLog()
{
    generateIndexLog = errstrm.str();
    if(generateIndexLog.length())
        return generateIndexLog.c_str();
    else
        return NULL;
}
COMMONLIB_API void ClearGenerateIndexLog()
{
    generateIndexLog.clear();
    errstrm.str(generateIndexLog);
}

static string parsePatientName(string &patient)
{
	char outbuf[64];
	patient._Copy_s(outbuf, sizeof(outbuf), patient.length());
	outbuf[patient.length()] = '\0';

	string test(outbuf);
	regex repToSpace("[=^]+");
	*regex_replace(outbuf, test.begin(), test.end(), repToSpace, string(" ")) = '\0';
	test = outbuf;

	regex ltrim("^ +");
	*regex_replace(outbuf, test.begin(), test.end(), ltrim, string("")) = '\0';
	test = outbuf;

	regex rtrim(" +$");
	*regex_replace(outbuf, test.begin(), test.end(), rtrim, string("")) = '\0';
	test = outbuf;
	return test;
}

COMMONLIB_API bool deleteSubTree(const char *dirpath, ostream *ostrm)
{
	bool allOK = true;
	WIN32_FIND_DATA wfd;
	char fileFilter[MAX_PATH];
	strcpy_s(fileFilter, dirpath);
	PathAppend(fileFilter, "*.*");
	HANDLE hDiskSearch = FindFirstFile(fileFilter, &wfd);
	
	if(!ostrm) ostrm = &cerr;

	if (hDiskSearch == INVALID_HANDLE_VALUE)  // 如果没有找到或查找失败
	{
		DWORD winerr = GetLastError();
		if(ERROR_FILE_NOT_FOUND == winerr)
			*ostrm << fileFilter << " not found, skip" << endl;
		return false;
	}
	do
	{
		if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0) 
			continue; // skip . ..
		fileFilter[strlen(dirpath)] = '\0';
		PathAppend(fileFilter, wfd.cFileName);
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(deleteSubTree(fileFilter))
			{
				if(_rmdir(fileFilter))
				{
					*ostrm << "rmdir " << fileFilter << " failed" << endl;
					allOK = false;
				}
			}
			else
			{
				allOK = false;
			}
		}
		else
		{
			if(remove(fileFilter))
			{
				*ostrm << "delete " << fileFilter << " failed" << endl;
				allOK = false;
			}
		}	
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // 关闭查找句柄
	return allOK;
}

COMMONLIB_API bool deleteTree(const char *dirpath, ostream *ostrm)
{
	if (_access_s(dirpath, 0))
		return true;  // dirpath dose not exist

	if(deleteSubTree(dirpath, ostrm))
	{
		if( ! _rmdir(dirpath))
			return true;
	}
	return false;
}

static HRESULT getStudyNode(const char *line, MSXML2::IXMLDOMDocumentPtr& pXMLDom, MSXML2::IXMLDOMElementPtr& study)
{
	istringstream patientStrm(line);

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string patientId(buffer);
	if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << line << endl; return E_FAIL; }

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string patientsName = parsePatientName(string(buffer));
	if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << line << endl; return E_FAIL; }

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string birthdate(buffer);

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string sex(buffer);

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string studyUID(buffer);
	if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << line << endl; return E_FAIL; }

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string studyDate(buffer);
	if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << line << endl; return E_FAIL; }

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string accNum(buffer);

	patientStrm.getline(buffer, BUFF_SIZE);
	string callingaetitle(buffer);

	//studyDatePath = "/YYYY/MM/DD"
	studyDatePath.clear();
	studyDatePath.append(1, '/').append(studyDate.substr(0, 4)).append(1, '/').append(studyDate.substr(4, 2));
	studyDatePath.append(1, '/').append(studyDate.substr(6, 2));

	char buf[MAX_PATH], hashBuf[9];
	__int64 hashStudy = uidHash(studyUID.c_str(), hashBuf, sizeof(hashBuf));
	if(studyUID.length() == 0) studyUID = "NULL";
	sprintf_s(buf, sizeof(buf), "%s/%c%c/%c%c/%c%c/%c%c/%s/%s", archivePath.c_str(),
		hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID.c_str(), hashBuf);
	downloadUrl = buf;  //downloadUrl = "<archivePath>/Ha/sh/Co/de/<study uid>/<study uid hash_code>"

	HRESULT hr;
	hr = pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		cerr << "poststore.cpp, getStudyNode(): Failed to CreateInstance on an XML DOM.\n";
		return hr;
	}
	pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
	pXMLDom->async = VARIANT_FALSE;

	MSXML2::IXMLDOMElementPtr wado;
	MSXML2::IXMLDOMProcessingInstructionPtr pi;
	pi = pXMLDom->createProcessingInstruction("xml", "version='1.0' encoding='gbk'");
	if (pi != NULL)
	{
		pXMLDom->appendChild(pi);
		pi.Release();
	}
	/*
	pi = pXMLDom->createProcessingInstruction("xml-stylesheet", "type='text/xml' href='xslt/receive.xsl'");
	if (pi != NULL)
	{
	pXMLDom->appendChild(pi);
	pi.Release();
	}
	*/
	wado = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "wado_query", "http://www.weasis.org/xsd");
	wado->setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	wado->setAttribute("requireOnlySOPInstanceUID", "false");
	pXMLDom->appendChild(wado);
	wado->setAttribute("wadoURL", baseurl.c_str());

	MSXML2::IXMLDOMElementPtr httpTag;
	httpTag = pXMLDom->selectSingleNode("/wado_query/httpTag");
	if( ! httpTag )
	{
		httpTag = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "httpTag", "http://www.weasis.org/xsd");
		wado->appendChild(httpTag);
	}
	httpTag->setAttribute("key", "callingAE");
	httpTag->setAttribute("value", callingaetitle.c_str());

	MSXML2::IXMLDOMElementPtr patient;
	patient = pXMLDom->selectSingleNode("/wado_query/Patient");
	if( ! patient )
	{
		patient = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Patient", "http://www.weasis.org/xsd");
		wado->appendChild(patient);
	}
	patient->setAttribute("PatientID", patientId.c_str());
	patient->setAttribute("PatientName", patientsName.c_str());
	if( ! birthdate.empty() ) patient->setAttribute("PatientBirthDate", birthdate.c_str());
	if( ! sex.empty() ) patient->setAttribute("PatientSex", sex.c_str());

	study = patient->selectSingleNode("./Study");
	if( ! study )
	{
		study = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Study", "http://www.weasis.org/xsd");
		study->setAttribute("StudyInstanceUID", studyUID.c_str());
		patient->appendChild(study);
	}
	study->setAttribute("StudyDate", studyDate.c_str());
	study->setAttribute("AccessionNumber", accNum.c_str());
	size_t studySize = diskUsage("..", studyUID.c_str()) / (1024 * 1024) + 1;
	study->setAttribute("StudyDescription", studySize);
	return S_OK;
}

static HRESULT addInstance(char *buffer, MSXML2::IXMLDOMElementPtr& study)
{
	string inputLine(buffer);
	istringstream instStrm(inputLine);

	instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string seriesUID(buffer);
	if(! instStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

	instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string seriesNumber(buffer);
	if(! instStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

	instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string modality(buffer);
	if(! instStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

	instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string instanceUID(buffer);
	if(! instStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

	instStrm.getline(buffer, BUFF_SIZE);
	string instanceNumber(buffer);

	string seriesQuery("./Series[@SeriesInstanceUID='");
	seriesQuery.append(seriesUID).append("']");

	string instanceQuery("./Instance[@SOPInstanceUID='");
	instanceQuery.append(instanceUID).append("']");

	MSXML2::IXMLDOMElementPtr series;
	MSXML2::IXMLDOMElementPtr instance;

	series = study->selectSingleNode(seriesQuery.c_str());
	if( ! series )
	{
		series = study->ownerDocument->createNode(MSXML2::NODE_ELEMENT, "Series", "http://www.weasis.org/xsd");
		series->setAttribute("SeriesInstanceUID", seriesUID.c_str());
		study->appendChild(series);
	}
	series->setAttribute("SeriesNumber", seriesNumber.c_str());
	series->setAttribute("Modality", modality.c_str());

	instance = series->selectSingleNode(instanceQuery.c_str());
	if( ! instance )
	{
		instance = study->ownerDocument->createNode(MSXML2::NODE_ELEMENT, "Instance", "http://www.weasis.org/xsd");
		instance->setAttribute("SOPInstanceUID", instanceUID.c_str());
		series->appendChild(instance);
	}
	instance->setAttribute("InstanceNumber", instanceNumber.c_str());
	char buf[MAX_PATH];
	if(CommonlibInstanceUniquePath)
	{
		size_t current = sprintf_s(buf, "%s/", downloadUrl.c_str());
		SeriesInstancePath(seriesUID.c_str(), instanceUID, buf + current, MAX_PATH - current, '/');
	}
	else
	{
		char hashBufSeries[9], hashBufInstance[9];
		uidHash(seriesUID.c_str(), hashBufSeries, sizeof(hashBufSeries));
		uidHash(instanceUID.c_str(), hashBufInstance, sizeof(hashBufInstance));
		sprintf_s(buf, sizeof(buf), "%s/%s/%s", downloadUrl.c_str(), hashBufSeries, hashBufInstance);
	}
	instance->setAttribute("DirectDownloadFile", buf);
	return S_OK;
}

static HRESULT createOrOpenFile(string &filePath, HANDLE &fh, MSXML2::IXMLDOMDocumentPtr &oldIndex)
{
	string indexFilePath = filePath;
	string::size_type p;
	while((p = indexFilePath.find('/')) != string::npos) indexFilePath.replace(p, 1, 1, '\\');
	p = indexFilePath.rfind('\\');
	string dirOnly = indexFilePath.substr(0, p);
	if( ! MkdirRecursive(dirOnly.c_str()))
	{
		cerr << "Failed to create receive date index dir: " << dirOnly << endl;
		return E_FAIL;
	}

	try
	{
		fh = ::CreateFile(indexFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if(fh == INVALID_HANDLE_VALUE)
		{
			if(ERROR_FILE_EXISTS == ::GetLastError())
			{
				for(int i = 0; i < 182; ++i)  // 55 ms * 182 = 10 seconds
				{
					fh = ::CreateFile(indexFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if(fh == INVALID_HANDLE_VALUE)
					{
						if(ERROR_SHARING_VIOLATION == ::GetLastError())
						{
							cerr << "Waiting lock " << indexFilePath << endl;
							::Sleep(55);
						}
						else
							throw "Failed to lock file ";
					}
					else
					{
						//cerr << "open file " << indexFilePath << endl;
						FILE_BASIC_INFO fbi;
						if(GetFileInformationByHandleEx(fh, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO)))
						{
							if(fbi.FileAttributes & FILE_ATTRIBUTE_ARCHIVE)
							{
								fbi.FileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
								SetFileInformationByHandle (fh, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO));
							}
						}
						break;
					}
				}

				if(fh == INVALID_HANDLE_VALUE) throw "Lock file timeout: ";

				oldIndex.CreateInstance(__uuidof(MSXML2::DOMDocument30));
				oldIndex->load(indexFilePath.c_str());
			}
			else
			{
				throw "Failed to create file ";
			}
		}
		//else
			//cerr << "create file " << indexFilePath << endl;
	}
	catch(_com_error &ex) 
	{
		cerr << "Failed to transform XML+XSLT: " << ex.ErrorMessage() << '\n';
		return ex.Error();
	}
	catch(char * message)
	{
		if(fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
		cerr << message << indexFilePath << '\n';
		displayErrorToCerr("createOrOpenFile");
		return E_FAIL;
	}
	return S_OK;
}

static HANDLE createFileMutex(string &indexFilePath)
{
	string mutexName("Global\\");
	string fileForMutexName(indexFilePath);
	string::size_type p;
	while((p = fileForMutexName.find('\\')) != string::npos) fileForMutexName.replace(p, 1, 1, ':');
	mutexName.append(fileForMutexName);

	HANDLE fileMutex = CreateMutex(NULL, FALSE, mutexName.c_str());
	if(fileMutex == NULL && ERROR_ACCESS_DENIED == GetLastError())
		fileMutex = OpenMutex(SYNCHRONIZE, FALSE, mutexName.c_str());
	DWORD result = WaitForSingleObject(fileMutex, INFINITE);
	if(result == WAIT_TIMEOUT || result == WAIT_FAILED)
	{
		CloseHandle(fileMutex);
		fileMutex = NULL;
	}
	return fileMutex;
}

static HRESULT createDateIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom, const char *xslFile, string &indexFilePath, bool uniqueStudy)
{
	HRESULT hr;
	MSXML2::IXMLDOMDocumentPtr pXsl;
	hr = pXsl.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		cerr << "Failed to CreateInstance on an XSL DOM." << endl;
		return hr;
	}
	pXsl->preserveWhiteSpace = VARIANT_FALSE;
	pXsl->async = VARIANT_FALSE;

	if(pXsl->load(xslFile) == VARIANT_FALSE)
	{
		cerr << "Failed to load XSL DOM: " << xslFile << endl;
		return E_FAIL;
	}

	string errorMessage;
	HANDLE fh = INVALID_HANDLE_VALUE, fileMutex = NULL;
	try
	{
		unsigned long written = 0;
		const char *header = "<?xml version=\"1.0\" encoding=\"gbk\"?>\r\n";
		_bstr_t xmlText(pXMLDom->transformNode(pXsl));
		MSXML2::IXMLDOMDocumentPtr dayDomPtr;
		dayDomPtr.CreateInstance(__uuidof(MSXML2::DOMDocument30));
		dayDomPtr->loadXML(xmlText);
		dayDomPtr->firstChild->attributes->getNamedItem("encoding")->text = "GBK";
		MSXML2::IXMLDOMNodePtr day = dayDomPtr->lastChild;

		fileMutex = createFileMutex(indexFilePath);

		MSXML2::IXMLDOMDocumentPtr oldIndex;
		hr = createOrOpenFile(indexFilePath, fh, oldIndex);
		if(SUCCEEDED(hr))
		{
			if(oldIndex)
			{
				if(uniqueStudy)
				{
					string query("/Collection/Study[text()='");
					query.append(day->firstChild->text).append("']");
					MSXML2::IXMLDOMNodePtr existStudy = oldIndex->selectSingleNode(query.c_str());
					if(existStudy) oldIndex->lastChild->removeChild(existStudy);
				}
				oldIndex->lastChild->appendChild(day->firstChild);
				::SetEndOfFile(fh);
				if( ! ( WriteFile(fh, header, strlen(header), &written, NULL) && 
					WriteFile(fh, (const char *)oldIndex->lastChild->xml, strlen((const char *)oldIndex->lastChild->xml), &written, NULL) ) )
				{
					::CloseHandle(fh);
					throw "Failed to write file ";
				}
			}
			else
			{
				if( ! ( WriteFile(fh, header, strlen(header), &written, NULL) && 
					WriteFile(fh, (const char *)day->xml, strlen((const char *)day->xml), &written, NULL) ) )
				{
					::CloseHandle(fh);
					throw "Failed to write file ";
				}
			}
			::CloseHandle(fh);
		}
		else
		{
			throw "Failed to create file ";
		}
	}
	catch(_com_error &ex) 
	{
		if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
		cerr << "Failed to transform XML+XSLT: " << ex.ErrorMessage() << endl;
		return ex.Error();
	}
	catch(char * message)
	{
		if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
		if(fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
		cerr << message << indexFilePath << endl;
		return E_FAIL;
	}
	if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
	return S_OK;
}

COMMONLIB_API const char* detectMediaType(size_t *pSize)
{
	CSimpleIni ini(false, false, false);
	SI_Error rc = SI_OK;
	for(int i = 0; i < 500; ++i)
	{
		rc = ini.LoadFile(TDB_STATUS);
		if(rc >= 0) break;
		Sleep(10);
	}
	/*
	bool paramOK = false;
	if(rc >= 0)
	{
		string publisherName(ini.GetValue("PUBLISHER1", "NAME", ""));
		if(publisherName.find("PP-100 ") == 0 || publisherName.find("PP-100N") == 0)
		{
			ofs << "FORMAT=ISO9660L2" << endl;
			paramOK = true;
		}
	}
	if(! paramOK) ofs << "FORMAT=UDF102" << endl;

	paramOK = false;
	*/
	long mediaType = 100;
	if (rc >= 0)
	{
		long mediaType1 = 100, mediaType2 = 100;
		mediaType1 = ini.GetLongValue("PUBLISHER1", "STACKER1_SETTING", 100);
		if(mediaType1 != 1 && mediaType1 != 4 && mediaType1 != 7 && mediaType1 != 8 && mediaType1 != 9) mediaType1 = 100;
		mediaType2 = ini.GetLongValue("PUBLISHER1", "STACKER2_SETTING", 100);
		if(mediaType2 != 1 && mediaType2 != 4 && mediaType2 != 7 && mediaType2 != 8 && mediaType2 != 9) mediaType2 = 100;
		mediaType = min(mediaType1, mediaType2);
	}
	else
		mediaType = 4; //MEDIA_DVD
	switch(mediaType)
	{
	case 1:
		if(pSize) *pSize = 580;
		return MEDIA_CD;
	case 7:
		if(pSize) *pSize = 8000;
		return MEDIA_DVD_DL;
	case 8:
		if(pSize) *pSize = 26000;
		return MEDIA_BD;
	case 9:
		if(pSize) *pSize = 52000;
		return MEDIA_BD_DL;
	default:  //4
		if(pSize) *pSize = 4000;
		return MEDIA_DVD;
	}
}

static int create_map_fields(map<string, string> &map_field, istream &ff, ostream &index_log)
{
    char line[1024];
    ff.getline(line, sizeof(line));
    int cnt = 0;
    while(ff.good())
    {
        if(strlen(line) <= 0) goto next_line;
        char *p = strchr(line, '=');
        if(p)
        {
            *p++ = '\0';
            map_field[line] = p;
        }
        else
        {
            map_field[line] = string();
        }
        index_log << "create_map_fields() " << line << " " << p << endl;
        ++cnt;
next_line:
        ff.getline(line, sizeof(line));
    }
    return cnt;
}

COMMONLIB_API int generateStudyJDF(const char *tag, const char *tagValue, ostream &index_log, const char *media)
{
	if(!strcmp(tag, "0020000d"))
	{
		string pacsBase;
		size_t requiredSize;
        char jobIdBuf[41] = "";
		getenv_s( &requiredSize, NULL, 0, "PACS_BASE");
		if(requiredSize > 0)
		{
			char* pPacsBase = NULL;
			pPacsBase = new char[requiredSize];
			getenv_s( &requiredSize, pPacsBase, requiredSize, "PACS_BASE");
			pacsBase = pPacsBase;
			delete pPacsBase;
		}
		if(!pacsBase.empty())
		{
			char hashBuf[9];
			__int64 hashStudy = uidHash(tagValue, hashBuf, sizeof(hashBuf));
            GetNextUniqueNo("job_", jobIdBuf, sizeof(jobIdBuf));
			sprintf_s(buffer, BUFF_SIZE, "%s\\tdd\\%s.jdf", pacsBase.c_str(), jobIdBuf);
			string jdfPath(buffer);

            sprintf_s(buffer, BUFF_SIZE, "%s\\pacs\\%s\\%s\\%c%c\\%c%c\\%c%c\\%c%c\\%s.txt", pacsBase.c_str(), indexBase.c_str(), tag,
				hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], tagValue);
			string fieldsPath(buffer);

            sprintf_s(buffer, BUFF_SIZE, "%s\\pacs\\%s\\%c%c\\%c%c\\%c%c\\%c%c\\%s", pacsBase.c_str(), archivePath.c_str(),
				hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], tagValue);
			if (_access_s( buffer, 4 ))
			{
				index_log << "generateStudyJDF() data don't exist: " << buffer << endl;
				return -4;
			}

            index_log << "generateStudyJDF() open fields: " << fieldsPath << endl;
            ifstream ifs(fieldsPath);
            if(ifs.good())
            {
                map<string, string> map_field_io;
                create_map_fields(map_field_io, ifs, index_log);
                ifs.close();

                char hash[12], chs_path[MAX_PATH];
                string pid = map_field_io["PatientID"];
                if(pid.length())
                {
                    uidHash(pid.c_str(), hash, sizeof(hash));
                    sprintf_s(chs_path, "%s\\00100020\\%c%c\\%c%c\\%c%c\\%c%c\\%s_ris.txt", indexBase.c_str(),
                        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], pid.c_str());
                    ifstream ffi(chs_path, ios_base::in);
                    if(ffi.good())
                    {
                        map<string, string> map_field_i;
                        create_map_fields(map_field_i, ffi, index_log);
                        ffi.close();
                        for(map<string, string>::iterator it = map_field_i.begin(); it != map_field_i.end(); ++it)
                        {
                            index_log << "generateStudyJDF() merge fields " << it->first << " " << it->second << endl;
                            map_field_io[it->first] = it->second;
                        }
                    }
                    else
                        index_log << "generateStudyJDF() can't open " << chs_path << endl;

                    ofstream ofs(fieldsPath, ios_base::out | ios_base::trunc);
                    if(ofs.good())
                    {
                        for(map<string, string>::iterator it = map_field_io.begin(); it != map_field_io.end(); ++it)
                        {
                            index_log << "generateStudyJDF() write fields " << it->first << " " << it->second << endl;
                            ofs << it->first << "=" << it->second << endl;
                        }
                        ofs.close();
                    }
                }
                else
                    index_log << "generateStudyJDF() no PatientID: " << pid << endl;
            }
            else
                index_log << "generateStudyJDF() open " << fieldsPath << " error" << endl;

			try
			{
				ofstream ofs(jdfPath.c_str(), ios_base::out | ios_base::trunc);
				if(ofs.good())
				{
					string valid_publisher;
					bool valid_found = SelectValidPublisher(TDB_STATUS, valid_publisher);
					if(valid_found || valid_publisher.find("error:", 0) == string::npos)
						ofs << "PUBLISHER=" << valid_publisher << endl;
                    ofs << "JOB_ID=" << jobIdBuf << endl;
					ofs << "FORMAT=UDF102" << endl;
					if(strcmp(MEDIA_AUTO, media))
						ofs << "DISC_TYPE=" << media << endl;
					else
						ofs << "DISC_TYPE=" << detectMediaType(NULL) << endl;
					ofs << "COPIES=1" << endl;
					ofs << "DATA=" << pacsBase << "\\viewer" << endl;
					ofs << "DATA=" << buffer << endl;
					ofs << "VOLUME_LABEL=SMARTPUB" << endl;
					ofs << "LABEL=" << pacsBase << "\\tdd\\patientInfo.tdd" << endl;
					ofs << "REPLACE_FIELD=" << fieldsPath << endl;
					ofs.close();
				}
				else
				{
					throw jdfPath;
				}
			}
			catch(string &outPath)
			{
				index_log << "create file failed: " << outPath << endl;
				return -2;
			}
			catch(...)
			{
				index_log << "write jdf error" << endl;
				return -3;
			}
			sprintf_s(buffer, BUFF_SIZE, "%s\\orders\\%s.jdf", pacsBase.c_str(), jobIdBuf);
			if(!rename(jdfPath.c_str(), buffer))
				return 0;
			else
				index_log << "move " << jdfPath << " to " << buffer << " failed" << endl;
		}
	}
	return -1;
}

static HRESULT mergeStudy(MSXML2::IXMLDOMNodePtr src, MSXML2::IXMLDOMNodePtr dest)
{
	_bstr_t seriesAttr("SeriesInstanceUID"), instanceAttr("SOPInstanceUID");
	MSXML2::IXMLDOMNodeListPtr childrenSeries = src->childNodes;
	while(MSXML2::IXMLDOMNodePtr seriesNodePtr = childrenSeries->nextNode())
	{
		MSXML2::DOMNodeType nodeType;
		seriesNodePtr->get_nodeType(&nodeType);
		if(nodeType != DOMNodeType::NODE_ELEMENT) continue;
		MSXML2::IXMLDOMElementPtr seriesPtr(seriesNodePtr);
		_variant_t seriesUidVariant = seriesPtr->getAttribute(seriesAttr);
		bstr_t seriesUid = (bstr_t)seriesUidVariant;
		if(seriesUid.length() == 0) continue;
		_bstr_t querySeries("./Series[@SeriesInstanceUID='");
		querySeries += seriesUid;
		querySeries += "']";
		MSXML2::IXMLDOMNodePtr destSeriesNodePtr = dest->selectSingleNode(querySeries);
		if(destSeriesNodePtr == NULL)
			dest->appendChild(seriesNodePtr->cloneNode(VARIANT_TRUE));
		else
		{
			MSXML2::IXMLDOMNodeListPtr childrenInstances = seriesNodePtr->childNodes;
			while(MSXML2::IXMLDOMNodePtr instanceNodePtr = childrenInstances->nextNode())
			{
				instanceNodePtr->get_nodeType(&nodeType);
				if(nodeType != DOMNodeType::NODE_ELEMENT) continue;
				MSXML2::IXMLDOMElementPtr instancePtr(instanceNodePtr);
				_variant_t instanceUidVariant = instancePtr->getAttribute(instanceAttr);
				bstr_t instanceUid = (bstr_t)instanceUidVariant;
				if(instanceUid.length() == 0) continue;
				_bstr_t queryInstance("./Instance[@SOPInstanceUID='");
				queryInstance += instanceUid;
				queryInstance += "']";
				MSXML2::IXMLDOMNodePtr destInstanceNodePtr = destSeriesNodePtr->selectSingleNode(queryInstance);
				if(destInstanceNodePtr == NULL)
					destSeriesNodePtr->appendChild(instanceNodePtr->cloneNode(VARIANT_TRUE));
			}
		}
	}
	return S_OK;
}

COMMONLIB_API bool deleteStudyFromIndex(const char *mode, const char *modeValue, const char *studyUid)
{
	HANDLE fileMutex = NULL;
	char buffer[1024];
    if(strcmp(mode, "00100020") == 0)   // patient id
    {
        char hashBuf[9];
	    __int64 hash = uidHash(modeValue, hashBuf, sizeof(hashBuf));
	    sprintf_s(buffer, sizeof(buffer), "indexdir\\00100020\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml",
		    hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], modeValue);
    }
    else                                // transfer or study date
    {
        int buflen = sprintf_s(buffer, sizeof(buffer), "indexdir\\%s\\%s.xml", mode, modeValue);
        replace(buffer, buffer + buflen, '/', '\\');
    }
	string indexPath(buffer);
	if(_access_s(buffer, 0)) return true; // if xml file dose not exist, return true;
	try
	{
		MSXML2::IXMLDOMDocumentPtr oldIndex;
		fileMutex = createFileMutex(indexPath);

		_variant_t xmlsrc(buffer);
		oldIndex.CreateInstance(__uuidof(MSXML2::DOMDocument30));
		oldIndex->load(xmlsrc);
        if(strcmp(mode, "00100020") == 0)   // patient id
		    sprintf_s(buffer, BUFF_SIZE, "/wado_query/Patient/Study[@StudyInstanceUID='%s']", studyUid);
        else                                // transfer or study date
            sprintf_s(buffer, BUFF_SIZE, "/Collection/Study[text()='%s']", studyUid);
        MSXML2::IXMLDOMNodeListPtr studies = oldIndex->selectNodes(buffer);
        if(studies && studies->Getlength() > 0)
		{
            for(long i = 0; i < studies->Getlength(); ++i)
            {
                MSXML2::IXMLDOMNodePtr cur = studies->Getitem(i);
                if(strcmp(mode, "00100020") == 0)   // patient id
			        oldIndex->lastChild->lastChild->removeChild(cur); // /wado_query/Patient ->removeChild(cur)
                else                                // transfer or study date
                    oldIndex->lastChild->removeChild(cur); // /wado_query/Patient ->removeChild(cur)
			    //oldIndex->firstChild->attributes->getNamedItem("encoding")->text = "gbk";
            }
			oldIndex->save(xmlsrc);
		}
		if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
	}
	catch(...)
	{
		if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
		//cerr << indexPath << "检查节点删除失败" << endl;
		return false;
	}
	return true;
}

static HRESULT createKeyValueIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom, const char *tag, const char *queryValue)
{
	HRESULT hr;
	_bstr_t tagValue = pXMLDom->selectSingleNode(queryValue)->Gettext();
	char hashBuf[9];
	__int64 hashUid36 = uidHashW(tagValue, hashBuf, sizeof(hashBuf));
	if(tagValue.length() == 0) return FWP_E_NULL_POINTER;

	sprintf_s(buffer, BUFF_SIZE, "%s\\%s\\%c%c\\%c%c\\%c%c\\%c%c\\%s", indexBase.c_str(), tag,
		hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], (const char*)tagValue);
	string indexPath = buffer;
	indexPath.append(".xml");
	string fieldsPath = buffer;
	fieldsPath.append(".txt");
	HANDLE fh = INVALID_HANDLE_VALUE, fileMutex = NULL;
	try
	{
		unsigned long written = 0;
		const char *header = "<?xml version=\"1.0\" encoding=\"gbk\"?>\n";
		MSXML2::IXMLDOMDocumentPtr oldIndex;
		fileMutex = createFileMutex(indexPath);
		hr = createOrOpenFile(indexPath, fh, oldIndex);
		if(SUCCEEDED(hr))
		{
			if(oldIndex)
			{
				MSXML2::IXMLDOMNodePtr newStudy = pXMLDom->selectSingleNode("/wado_query/Patient/Study");
				_bstr_t studyUid = newStudy->selectSingleNode("./@StudyInstanceUID")->Gettext();
				sprintf_s(buffer, BUFF_SIZE, "/wado_query/Patient/Study[@StudyInstanceUID='%s']", (const char*)studyUid);
				MSXML2::IXMLDOMNodePtr existStudy = oldIndex->selectSingleNode(buffer);
				if(existStudy)
				{
					mergeStudy(existStudy, newStudy);
					oldIndex->lastChild->lastChild->removeChild(existStudy); // /wado_query/Patient ->removeChild(existStudy)
				}
				oldIndex->lastChild->lastChild->appendChild(newStudy->cloneNode(VARIANT_TRUE)); // /wado_query/Patient ->appendChild(newStudy)
				::SetEndOfFile(fh);
				if( ! ( WriteFile(fh, header, strlen(header), &written, NULL) && 
					WriteFile(fh, (const char *)oldIndex->lastChild->xml, strlen((const char *)oldIndex->lastChild->xml), &written, NULL) ) )
				{
					::CloseHandle(fh);
					throw "Failed to write file ";
				}
			}
			else
			{
				if( ! ( WriteFile(fh, header, strlen(header), &written, NULL) && 
					WriteFile(fh, (const char *)pXMLDom->lastChild->xml, strlen((const char *)pXMLDom->lastChild->xml), &written, NULL) ) )
				{
					::CloseHandle(fh);
					throw "Failed to write file ";
				}
			}
			::CloseHandle(fh);

			// cd label fields value
			MSXML2::IXMLDOMDocumentPtr pXsl;
			const char *xslFile = "xslt\\mktext.xsl";
			hr = pXsl.CreateInstance(__uuidof(MSXML2::DOMDocument30));
			pXsl->preserveWhiteSpace = VARIANT_FALSE;
			pXsl->async = VARIANT_FALSE;
			if(pXsl->load(xslFile) == VARIANT_FALSE)
			{
				errstrm << "Failed to load XSL DOM: " << xslFile << endl;
			}
			else
			{
				_bstr_t textFields = oldIndex ? oldIndex->transformNode(pXsl) : pXMLDom->transformNode(pXsl);
				ofstream ofs(fieldsPath.c_str(), ios_base::out | ios_base::trunc);
				string istr((LPCSTR)textFields);
				istr.erase(remove(istr.begin(), istr.end(), '\r'), istr.end());
                ofs << istr;
				ofs.close();
			}

			// jdf file
			if(CommonlibBurnOnce)
				CommonlibBurnOnce = (0 != generateStudyJDF(tag, (const char*)tagValue, errstrm));
		}
		else
		{
			throw "Failed to create file ";
		}
	}
	catch(_com_error &ex) 
	{
		if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
		errstrm << "Failed to transform XML+XSLT: " << ex.ErrorMessage() << '\n';
		return ex.Error();
	}
	catch(char * message)
	{
		if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
		if(fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
		errstrm << message << indexPath << '\n';
		return E_FAIL;
	}
	if(fileMutex) { ReleaseMutex(fileMutex); CloseHandle(fileMutex); }
	return hr;
}

static HRESULT processInputStream(istream& istrm)
{
	istrm.getline(buffer, BUFF_SIZE);
	if( ! istrm.good() )
	{
		errstrm << "Failed to read input file\n";
		return E_FAIL;
	}

	HRESULT hr;
	MSXML2::IXMLDOMDocumentPtr pXMLDom;
	MSXML2::IXMLDOMElementPtr study;

	hr = getStudyNode(buffer, pXMLDom, study);
	if (FAILED(hr)) 
	{
		errstrm << "Failed to create DOM\n";
		return hr;
	}

	bool successOnce = false;
	while(istrm && istrm.peek() != istream::traits_type::eof())
	{
		istrm.getline(buffer, BUFF_SIZE);
		hr = addInstance(buffer, study);
		successOnce |= SUCCEEDED(hr);
	}
	if( ! successOnce ) return E_FAIL;

	// study instance uid
	hr = createKeyValueIndex(pXMLDom, TAG_StudyInstanceUID, "/wado_query/Patient/Study/@StudyInstanceUID");
	if (FAILED(hr))
	{
		errstrm << "Failed to save StudyInstanceUID index\n";
		return hr;
	}
	
	// patient id index
	hr = createKeyValueIndex(pXMLDom, TAG_PatientID, "/wado_query/Patient/@PatientID");
	if (FAILED(hr) && hr != FWP_E_NULL_POINTER)
	{
		errstrm << "Failed to save PatientID index\n";
		return hr;
	}

	// main index OK, the following is other index.

	//dayIndexFilePath = "<indexBase>/<tagStudyDate>/YYYY/MM/DD.xml"
	sprintf_s(buffer, BUFF_SIZE, "%s/%s%s.xml", indexBase.c_str(), TAG_StudyDate, studyDatePath.c_str());
	string dayIndexFilePath = buffer;
	hr = createDateIndex(pXMLDom, "xslt\\receive.xsl", dayIndexFilePath, true);
	if (FAILED(hr))
		errstrm << "Failed to create study date index: " << dayIndexFilePath << endl;

	// receive date index
	time_t t = time( NULL );
	struct tm today;
	localtime_s(&today, &t);
	strftime(buffer, BUFF_SIZE, "\\receive\\%Y\\%m\\%d.xml", &today );
	string receiveIndexFilePath = indexBase;
	receiveIndexFilePath.append(buffer);
	hr = createDateIndex(pXMLDom, "xslt\\receive.xsl", receiveIndexFilePath, false);
	if (FAILED(hr))
		errstrm << "Failed to create receive date index: " << receiveIndexFilePath << endl;

	return S_OK;
}

static bool operationRetry(int(*fn)(const char *), const char *param, int state, int seconds, const char *messageHeader)
{
	bool opFail = true;
	for(int i = 0; i < seconds; ++i )
	{
		if( fn(param) )
		{
			int errnoOperation = 0;
			_get_errno(&errnoOperation);
			if(errnoOperation == state)
				::Sleep(1000);
			else
				break;
		}
		else
		{
			opFail = false;
			break;
		}
	}

	if(opFail)
	{
		perror(param);
		cerr << messageHeader << param <<endl;
	}

	return ! opFail;
}

COMMONLIB_API HRESULT generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath, bool deleteSourceCSV)
{
	HRESULT hr;
	if(paramBaseUrl) baseurl = paramBaseUrl;
	if(archPath) archivePath = archPath;
	if(indPath) indexBase = indPath;
	ifstream infile(inputFile);
	if(infile.good())
	{
		CoInitialize(NULL);
		hr = processInputStream(infile);
		CoUninitialize();
		infile.close();
		if(SUCCEEDED(hr))
		{
			if(deleteSourceCSV)
			{
				// dcmcjpeg inherit handle of instance.txt from storescp, wait dcmcjpeg close it.
				if( operationRetry(remove, inputFile, EACCES, RMDIR_WAIT_SECONDS, "error at remove file: ") )
				{
					char *pos = NULL;
					if((pos = strrchr(inputFile, '\\')) != NULL)
					{
						*pos = '\0';
						operationRetry(_rmdir, inputFile, ENOTEMPTY, RMDIR_WAIT_SECONDS, "error at remove dir: ");
					}
				}
			}
		}
		else
		{
			cerr << "processInputStream(" << inputFile << ") failed: " << hr << endl;
		}
	}
	else
	{
		cerr << "error at open csv file: " << (inputFile == NULL ? "NULL" : inputFile) <<endl;
		hr = E_FAIL;
	}
	return hr;
}

COMMONLIB_API bool generateStudyXML(const char *line, ostream &xmlStream, bool isEncapsulated)
{
	MSXML2::IXMLDOMDocumentPtr pXmlDom;
	MSXML2::IXMLDOMElementPtr study;
	HRESULT hr = getStudyNode(line, pXmlDom, study);
	if(FAILED(hr)) return false;

	MSXML2::IXMLDOMElementPtr httpTag = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "httpTag", "http://www.weasis.org/xsd");
	MSXML2::IXMLDOMElementPtr wado = pXmlDom->selectSingleNode("/wado_query");
	wado->appendChild(httpTag);
	httpTag->setAttribute("key", "command");
	if(isEncapsulated)
		httpTag->setAttribute("value", MOVE_PLACE_HOLDER);
	else
		httpTag->setAttribute("value", REPLACE_PLACE_HOLDER);

	xmlStream << "<?xml version=\"1.0\" encoding=\"gbk\"?>" << endl;
	xmlStream << (const char *)pXmlDom->lastChild->xml;
	return true;
}

static void searchRecursively(string &path, long long &filesizes)
{
	WIN32_FIND_DATA wfd;

	string::size_type pathLength = path.length();
	path.append("\\*.*");
	HANDLE hDiskSearch = FindFirstFile(path.c_str(), &wfd);
	path.resize(pathLength);

	if (hDiskSearch != INVALID_HANDLE_VALUE)  // 如果没有找到或查找失败
    {
		do
		{
			if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0) 
				continue; // skip . ..
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{// 如果找到的是目录，则递归查找
				path.append(1, '\\');
				path.append(wfd.cFileName);
				//cout << path << '\\' << endl;
				searchRecursively(path, filesizes);
				path.resize(pathLength);
			}
			else
			{
				LARGE_INTEGER temp;
				temp.LowPart = wfd.nFileSizeLow;
				temp.HighPart = wfd.nFileSizeHigh;
				filesizes += temp.QuadPart;
				//cout << path << '\\' << wfd.cFileName << '\t' << wfd.nFileSizeLow << endl;
			}
		} while (FindNextFile(hDiskSearch, &wfd));
		FindClose(hDiskSearch); // 关闭查找句柄
	}
}

COMMONLIB_API long long diskUsage(const char *pacsBase, const char *studyUID)
{
	long long filesizes = 0LL;
	char hashBuf[9];
	__int64 hashStudy = uidHash(studyUID, hashBuf, sizeof(hashBuf));
	if(strlen(studyUID) == 0) studyUID = "NULL";
	sprintf_s(buffer, sizeof(buffer), "%s\\pacs\\%s\\%c%c\\%c%c\\%c%c\\%c%c\\%s", pacsBase, archivePath.c_str(),
		hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
	string strbuf(MAX_PATH, ' ');
	strbuf = buffer;

	WIN32_FIND_DATA wfd;
	HANDLE hDiskSearch = FindFirstFile(strbuf.c_str(), &wfd);
	if (hDiskSearch != INVALID_HANDLE_VALUE)  // 如果没有找到或查找失败
    {
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			FindClose(hDiskSearch); // 关闭查找句柄
			searchRecursively(strbuf, filesizes);
		}
		else
		{
			LARGE_INTEGER temp;
			temp.LowPart = wfd.nFileSizeLow;
			temp.HighPart = wfd.nFileSizeHigh;
			filesizes += temp.QuadPart;
			FindClose(hDiskSearch); // 关闭查找句柄
		}
	}
	return filesizes;
}

COMMONLIB_API bool SelectValidPublisher(const char *ini_path, string &valid_publisher)
{
    CSimpleIni ini(false, false, false);
    //std::ifstream instream;
    //instream.open("..\\orders\\TDBStatus.txt", std::ifstream::in | std::ifstream::binary, _SH_DENYNO);
	SI_Error rc = SI_OK;
	for(int i = 0; i < 500; ++i)
	{
		rc = ini.LoadFile(ini_path);
		if(rc >= 0) break;
		Sleep(10);
	}
	//instream.close();
    if (rc < 0) {
		valid_publisher = "error:没有任务";
		return false;
	}
	CSimpleIni::TNamesDepend sections;
	ini.GetAllSections(sections);
	CSimpleIni::TNamesDepend::iterator sec = sections.begin();
	string default_publisher;
	while(sec != sections.end())
	{
		string currentSection((*sec).pItem);
		if(string::npos == currentSection.find("PUBLISHER", 0))
		{
			++sec;
			continue;
		}
		CSimpleIni::TNamesDepend keys;
		ini.GetAllKeys((*sec).pItem, keys);
		CSimpleIni::TNamesDepend::iterator key = keys.begin();
		bool valid = true;
		long stack1 = 0, stack2 = 0, d1_status = -1, d1_life = 100, d2_status = -1, d2_life = 100;
		const char *pname = ini.GetValue(currentSection.c_str(), "NAME", NULL);
		string name;
		if(pname == NULL || *pname == '\0')
		{
			++sec;
			continue;
		}
		else
			name = pname;

		if(default_publisher.empty()) default_publisher = name;

		while(key != keys.end())
		{
			string currentKey((*key).pItem);
			if(currentKey == "INFO1")
			{
				const char *pv = ini.GetValue(currentSection.c_str(), currentKey.c_str(), NULL);
				valid = (pv == NULL || *pv == '\0');
			}
			else if(currentKey == "STACKER1") { stack1 = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), 0); }
			else if(currentKey == "STACKER2") { stack2 = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), 0); }
			else if(0 == currentKey.find("INK_", 0)) { valid = (0 < ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), -1)); }
			else if(currentKey == "PRINTER_STATUS")
			{
				long prn_status = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), -1);
				valid = (prn_status == 1 || prn_status == 2 || prn_status == 3 || prn_status == 4 || prn_status == 6);
			}
			else if(currentKey == "MAINTENANCE_BOX_FREE_SPACE") { valid = (0 < ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), -1)); }
			else if(currentKey == "DRIVE1_STATUS") { d1_status = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), -1); }
			else if(currentKey == "DRIVE2_STATUS") { d2_status = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), -1); }
			else if(currentKey == "DRIVE1_LIFE") { d1_life = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), 100); }
			else if(currentKey == "DRIVE2_LIFE") { d2_life = ini.GetLongValue(currentSection.c_str(), currentKey.c_str(), 100); }
			if(!valid) break;
			++key;
		}
		if(valid)
		{
			valid = ((stack1 + stack2) > 0 && ((d1_status == 1 && d1_life < 100) || (d2_status == 1 && d2_life < 100)));
			if(valid)
			{
				valid_publisher = name;
				return true;
			}
		}
		++sec;
	}
	valid_publisher = default_publisher.empty() ? "error:没有可用的刻录机" : default_publisher;
	return false;
}

COMMONLIB_API int StatusXml(const char *statusFlag, const char *ini_path, int licenseCnt, std::ostream &outputbuf)
{
	bool hasError = false;
	MSXML2::IXMLDOMDocumentPtr pXmlDom;
	HRESULT hr = pXmlDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		outputbuf << "poststore.cpp, StatusXml(): Failed to CreateInstance on an XML DOM." << endl;
		return -1;
	}
	pXmlDom->preserveWhiteSpace = VARIANT_FALSE;
	pXmlDom->async = VARIANT_FALSE;

	MSXML2::IXMLDOMProcessingInstructionPtr pi = pXmlDom->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"gbk\"");
	if (pi != NULL) pXmlDom->appendChild(pi);

	MSXML2::IXMLDOMProcessingInstructionPtr pXslt = NULL;
	if(strcmp(statusFlag, "xml"))
	{
		pXslt = pXmlDom->createProcessingInstruction("xml-stylesheet", "type=\"text/xml\" href=\"../xslt/status.xsl\"");
		if (pXslt != NULL) pXmlDom->appendChild(pXslt);
	}

	MSXML2::IXMLDOMElementPtr root = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "tdb_status", "");
	pXmlDom->appendChild(root);
	MSXML2::IXMLDOMElementPtr errorInfos = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "error_infos", "");

	CSimpleIni ini(false, false, false);
    //std::ifstream instream;
    //instream.open("..\\orders\\TDBStatus.txt", std::ifstream::in | std::ifstream::binary, _SH_DENYNO);
	SI_Error rc = ini.LoadFile(ini_path);
	//instream.close();
    if (rc < 0) {
		outputbuf << "没有任务" << endl;
		return -2;
	}

	CSimpleIni::TNamesDepend sections;
	ini.GetAllSections(sections);
	CSimpleIni::TNamesDepend::iterator sec = sections.begin();
	while(sec != sections.end())
	{
		string currentSection;
		const char *currentKey = NULL, *currentValue = NULL;
		try
		{
			currentSection = (*sec).pItem;
			if(string::npos != currentSection.find("PUBLISHER", 0) || currentSection == "TDB_INFO"
				|| currentSection == "ACTIVE_JOB" || currentSection == "COMPLETE_JOB")
			{
				MSXML2::IXMLDOMElementPtr sectionNode = pXmlDom->createNode(MSXML2::NODE_ELEMENT, 
					string::npos == currentSection.find("PUBLISHER", 0) ? currentSection.c_str() : "PUBLISHER", "");
				CSimpleIni::TNamesDepend keys;
				ini.GetAllKeys((*sec).pItem, keys);
				CSimpleIni::TNamesDepend::iterator key = keys.begin();
				while(key != keys.end())
				{
					currentKey = (*key).pItem;
					currentValue = ini.GetValue(currentSection.c_str(), currentKey);
					MSXML2::IXMLDOMElementPtr item;
					if(currentSection == "ACTIVE_JOB" || currentSection == "COMPLETE_JOB")
					{
						item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "JOB", "");
						item->setAttribute("id", currentValue);
					}
					else
					{
						item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentKey, "");
						item->appendChild(pXmlDom->createTextNode(currentValue));
					}
					sectionNode->appendChild(item);
					currentKey = NULL;
					currentValue = NULL;
					++key;
				}
				root->appendChild(sectionNode);
			}
			else  // treats it as job
			{
				MSXML2::IXMLDOMElementPtr sectionNode = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "JOB_STATUS", "");
				sectionNode->setAttribute("id", currentSection.c_str());
				CSimpleIni::TNamesDepend keys;
				ini.GetAllKeys((*sec).pItem, keys);
				CSimpleIni::TNamesDepend::iterator key = keys.begin();
				while(key != keys.end())
				{
					currentKey = (*key).pItem;
					currentValue = ini.GetValue(currentSection.c_str(), currentKey);
					MSXML2::IXMLDOMElementPtr item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentKey, "");
					item->appendChild(pXmlDom->createTextNode(currentValue));
					sectionNode->appendChild(item);
					currentKey = NULL;
					currentValue = NULL;
					++key;
				}
				root->appendChild(sectionNode);
			}
		}
		catch(_com_error &ex) 
		{
			hasError = true;
			ostringstream errbuf;
			errbuf << "设备状态错误: 0x" << hex << ex.Error() << ',' << ex.ErrorMessage();
			if(! currentSection.empty())
			{
				errbuf << ", [" << currentSection  << ']';
				if(currentKey) errbuf << ", " << currentKey	<< " = " << (currentValue ? currentValue : "");
			}
			MSXML2::IXMLDOMElementPtr errorInfo = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "error_info", "");
			errorInfo->appendChild(pXmlDom->createTextNode(errbuf.str().c_str()));
			errorInfos->appendChild(errorInfo);
		}
		++sec;
	}

	// append license count
	MSXML2::IXMLDOMElementPtr licenseNode = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "LICENSE_COUNTER", "");
	_variant_t licenseCount(licenseCnt);
	licenseNode->appendChild(pXmlDom->createTextNode(_bstr_t(licenseCount)));
	root->appendChild(licenseNode);

	if(hasError) root->appendChild(errorInfos);
	outputbuf << "<?xml version=\"1.0\" encoding=\"gbk\"?>" << (pXslt ? pXslt->xml : "") << root->xml;
	return 0;
}
