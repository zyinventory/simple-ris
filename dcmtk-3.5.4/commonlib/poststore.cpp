// poststore.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#import <msxml3.dll>

#include "commonlib.h"
#include "SimpleIni.h"
#include "poststore.h"

using namespace std;

const char UNIT_SEPARATOR = 0x1F;
const size_t BUFF_SIZE = 1024;
const int RMDIR_WAIT_SECONDS = 10;
static char buffer[BUFF_SIZE];
static bool burnOnce = false;
string archivePath("archdir");
string indexBase("indexdir");
string baseurl, downloadUrl, studyDatePath;

bool getBurnOnce()
{
	return burnOnce;
}

void setBurnOnce()
{
	burnOnce = true;
}

HRESULT getStudyNode(const char *line, MSXML2::IXMLDOMDocumentPtr& pXMLDom, MSXML2::IXMLDOMElementPtr& study)
{
	istringstream patientStrm(line);

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string patientId(buffer);
	if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << line << endl; return E_FAIL; }

	patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
	string patientsName(buffer);
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
	studyDatePath.append(1, '/').append(studyDate.substr(0, 4)).append(1, '/').append(studyDate.substr(4, 2));
	studyDatePath.append(1, '/').append(studyDate.substr(6, 2));

	char buf[MAX_PATH];
	unsigned int hashStudy = hashCode(studyUID.c_str());
	if(studyUID.length() == 0) studyUID = "NULL";
	sprintf_s(buf, sizeof(buf), "%s/%02X/%02X/%02X/%02X/%s/%08X", archivePath.c_str(),
		hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID.c_str(), hashStudy);
	downloadUrl = buf;  //downloadUrl = "<archivePath>/Ha/sh/Co/de/<study uid>/<study uid hashCode>"

	HRESULT hr;
	hr = pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		cerr << "Failed to CreateInstance on an XML DOM.\n";
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
	return S_OK;
}

HRESULT addInstance(char *buffer, MSXML2::IXMLDOMElementPtr& study)
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
	sprintf_s(buf, sizeof(buf), "%s/%08X/%08X/%08X", downloadUrl.c_str(), hashCode(seriesUID.c_str()), hashCode(instanceUID.c_str()), hashCode(instanceUID.c_str(), 131));
	instance->setAttribute("DirectDownloadFile", buf);
	return S_OK;
}

HRESULT createOrOpenFile(string &filePath, HANDLE &fh, MSXML2::IXMLDOMDocumentPtr &oldIndex)
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

HRESULT createDateIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom, const char *xslFile, string &indexFilePath, bool uniqueStudy)
{
	HRESULT hr;
	MSXML2::IXMLDOMDocumentPtr pXsl;
	hr = pXsl.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		cerr << "Failed to CreateInstance on an XSL DOM.\n";
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
	HANDLE fh = INVALID_HANDLE_VALUE;
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
		cerr << "Failed to transform XML+XSLT: " << ex.ErrorMessage() << '\n';
		return ex.Error();
	}
	catch(char * message)
	{
		if(fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
		cerr << message << indexFilePath << '\n';
		return E_FAIL;
	}
	return S_OK;
}

int generateStudyJDF(const char *tag, const char *tagValue, ostream &errstrm, const char *media)
{
	if(!strcmp(tag, "0020000d"))
	{
		string pacsBase;
		size_t requiredSize;
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
			unsigned int hash = hashCode(tagValue);
			sprintf_s(buffer, BUFF_SIZE, "%s\\tdd\\%s.jdf", pacsBase.c_str(), tagValue);
			string jdfPath(buffer);
			sprintf_s(buffer, BUFF_SIZE, "%s\\pacs\\%s\\%s\\%02X\\%02X\\%02X\\%02X\\%s.txt", pacsBase.c_str(), indexBase.c_str(), tag,
				hash >> 24 & 0xff, hash >> 16 & 0xff, hash >> 8 & 0xff, hash & 0xff, tagValue);
			string fieldsPath(buffer);
			sprintf_s(buffer, BUFF_SIZE, "%s\\pacs\\%s\\%02X\\%02X\\%02X\\%02X\\%s", pacsBase.c_str(), archivePath.c_str(),
				hash >> 24 & 0xff, hash >> 16 & 0xff, hash >> 8 & 0xff, hash & 0xff, tagValue);
			try
			{
				ofstream ofs(jdfPath.c_str(), ios_base::out | ios_base::trunc);
				if(ofs.good())
				{
					CSimpleIni ini(false, false, false);
					SI_Error rc = SI_OK;
					for(int i = 0; i < 5; ++i)
					{
						rc = ini.LoadFile("..\\orders\\TDBStatus.txt");
						if(rc >= 0) break;
						Sleep(100);
					}

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
					if(strcmp(MEDIA_AUTO, media))
					{
						ofs << "DISC_TYPE=" << media << endl;
					}
					else
					{
						if (rc >= 0)
						{
							long mediaType1 = 100, mediaType2 = 100;
							mediaType1 = ini.GetLongValue("PUBLISHER1", "STACKER1_SETTING", 100);
							if(mediaType1 != 1 && mediaType1 != 4 && mediaType1 != 7 && mediaType1 != 8 && mediaType1 != 9) mediaType1 = 100;
							mediaType2 = ini.GetLongValue("PUBLISHER1", "STACKER2_SETTING", 100);
							if(mediaType2 != 1 && mediaType2 != 4 && mediaType2 != 7 && mediaType2 != 8 && mediaType2 != 9) mediaType2 = 100;
							switch(min(mediaType1, mediaType2))
							{
							case 1:
								ofs << "DISC_TYPE=" << MEDIA_CD << endl;
								paramOK = true;
								break;
							case 4:
								ofs << "DISC_TYPE=" << MEDIA_DVD << endl;
								paramOK = true;
								break;
							case 7:
								ofs << "DISC_TYPE=" << MEDIA_DVD_DL << endl;
								paramOK = true;
								break;
							case 8:
								ofs << "DISC_TYPE=" << MEDIA_BD << endl;
								paramOK = true;
								break;
							case 9:
								ofs << "DISC_TYPE=" << MEDIA_BD_DL << endl;
								paramOK = true;
								break;
							default:
								paramOK = false;
							}
						}
						if(! paramOK) ofs << "DISC_TYPE=" << MEDIA_CD << endl;
					}

					ofs << "COPIES=1" << endl;
					ofs << "DATA=" << pacsBase << "\\eFilmLite\\Autorun.inf" << endl;
					ofs << "DATA=" << pacsBase << "\\eFilmLite\teFilmLite" << endl;
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
				errstrm << "create file failed: " << outPath << endl;
				return -2;
			}
			catch(...)
			{
				errstrm << "write jdf error" << endl;
				return -3;
			}
			char timeBuffer[16];
			generateTime(DATE_FORMAT_COMPACT, timeBuffer, sizeof(timeBuffer));
			sprintf_s(buffer, BUFF_SIZE, "%s\\orders\\%s_%s.jdf", pacsBase.c_str(), timeBuffer, tagValue);
			if(!rename(jdfPath.c_str(), buffer))
				return 0;
			else
				errstrm << "move " << jdfPath << " to " << buffer << " failed" << endl;
		}
	}
	return -1;
}

HRESULT createKeyValueIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom, const char *tag, const char *queryValue)
{
	HRESULT hr;
	_bstr_t tagValue = pXMLDom->selectSingleNode(queryValue)->Gettext();
	unsigned int hash = hashCodeW(tagValue);
	if(tagValue.length() == 0) return FWP_E_NULL_POINTER;

	sprintf_s(buffer, BUFF_SIZE, "%s\\%s\\%02X\\%02X\\%02X\\%02X\\%s", indexBase.c_str(), tag,
		hash >> 24 & 0xff, hash >> 16 & 0xff, hash >> 8 & 0xff, hash & 0xff, (const char*)tagValue);
	string indexPath = buffer;
	indexPath.append(".xml");
	string fieldsPath = buffer;
	fieldsPath.append(".txt");
	HANDLE fh = INVALID_HANDLE_VALUE;
	try
	{
		unsigned long written = 0;
		const char *header = "<?xml version=\"1.0\" encoding=\"gbk\"?>\n";
		MSXML2::IXMLDOMDocumentPtr oldIndex;
		hr = createOrOpenFile(indexPath, fh, oldIndex);
		if(SUCCEEDED(hr))
		{
			if(oldIndex)
			{
				MSXML2::IXMLDOMNodePtr newStudy = pXMLDom->selectSingleNode("/wado_query/Patient/Study");
				_bstr_t studyUid = newStudy->selectSingleNode("./@StudyInstanceUID")->Gettext();
				sprintf_s(buffer, BUFF_SIZE, "/wado_query/Patient/Study[@StudyInstanceUID='%s']", (const char*)studyUid);
				MSXML2::IXMLDOMNodePtr existStudy = oldIndex->selectSingleNode(buffer);
				if(existStudy) oldIndex->lastChild->lastChild->removeChild(existStudy); // /wado_query/Patient ->removeChild(existStudy)
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
				cerr << "Failed to load XSL DOM: " << xslFile << endl;
			}
			else
			{
				_bstr_t textFields = oldIndex ? oldIndex->transformNode(pXsl) : pXMLDom->transformNode(pXsl);
				ofstream ofs(fieldsPath.c_str(), ios_base::out | ios_base::trunc);
				if(strcmp(TAG_StudyInstanceUID, tag) == 0)
					ofs << "StudySize=" << diskUsage("..", (const char*)tagValue) << endl;
				ofs << textFields;
				ofs.close();
			}

			// jdf file
			if(burnOnce)
				burnOnce = (0 != generateStudyJDF(tag, (const char*)tagValue, cerr));
		}
		else
		{
			throw "Failed to create file ";
		}
	}
	catch(_com_error &ex) 
	{
		cerr << "Failed to transform XML+XSLT: " << ex.ErrorMessage() << '\n';
		return ex.Error();
	}
	catch(char * message)
	{
		if(fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
		cerr << message << indexPath << '\n';
		return E_FAIL;
	}
	return hr;
}

HRESULT processInputStream(istream& istrm)
{
	istrm.getline(buffer, BUFF_SIZE);
	if( ! istrm.good() )
	{
		cerr << "Failed to read input file\n";
		return E_FAIL;
	}

	HRESULT hr;
	MSXML2::IXMLDOMDocumentPtr pXMLDom;
	MSXML2::IXMLDOMElementPtr study;

	hr = getStudyNode(buffer, pXMLDom, study);
	if (FAILED(hr)) 
	{
		cerr << "Failed to create DOM\n";
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
		cerr << "Failed to save StudyInstanceUID index\n";
		return hr;
	}
	
	// patient id index
	hr = createKeyValueIndex(pXMLDom, TAG_PatientID, "/wado_query/Patient/@PatientID");
	if (FAILED(hr) && hr != FWP_E_NULL_POINTER)
	{
		cerr << "Failed to save PatientID index\n";
		return hr;
	}

	// main index OK, the following is other index.

	//dayIndexFilePath = "<indexBase>/<tagStudyDate>/YYYY/MM/DD.xml"
	sprintf_s(buffer, BUFF_SIZE, "%s/%s/%s.xml", indexBase.c_str(), TAG_StudyDate, studyDatePath.c_str());
	string dayIndexFilePath = buffer;
	hr = createDateIndex(pXMLDom, "xslt\\receive.xsl", dayIndexFilePath, true);
	if (FAILED(hr))
		cerr << "Failed to create study date index: " << dayIndexFilePath << endl;

	// receive date index
	time_t t = time( NULL );
	struct tm today;
	localtime_s(&today, &t);
	strftime(buffer, BUFF_SIZE, "\\receive\\%Y\\%m\\%d.xml", &today );
	string receiveIndexFilePath = indexBase;
	receiveIndexFilePath.append(buffer);
	hr = createDateIndex(pXMLDom, "xslt\\receive.xsl", receiveIndexFilePath, false);
	if (FAILED(hr))
		cerr << "Failed to create receive date index: " << receiveIndexFilePath << endl;

	return S_OK;
}

bool operationRetry(int(*fn)(const char *), const char *param, int state, int seconds, const char *messageHeader)
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

HRESULT generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath, bool deleteSourceCSV)
{
	HRESULT hr;
	if(paramBaseUrl) baseurl = paramBaseUrl;
	if(archPath) archivePath = archPath;
	if(indPath) indexBase = indPath;
	ifstream infile(inputFile);
	if(infile)
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
		perror(inputFile);
		cerr << "error at open file: " << inputFile <<endl;
		hr = E_FAIL;
	}
	return hr;
}

bool generateStudyXML(const char *line, ostream &xmlStream, bool isEncapsulated)
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

long long diskUsage(const char *pacsBase, const char *studyUID)
{
	long long filesizes = 0LL;
	unsigned int hashStudy = hashCode(studyUID);
	if(strlen(studyUID) == 0) studyUID = "NULL";
	sprintf_s(buffer, sizeof(buffer), "%s\\pacs\\%s\\%02X\\%02X\\%02X\\%02X\\%s", pacsBase, archivePath.c_str(),
		hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
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
