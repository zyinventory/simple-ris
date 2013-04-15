// poststore.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <limits.h>
#include <direct.h>
#include <errno.h>
#include <memory>
#include "commonlib.h"
#include "poststore.h"

using namespace std;
#import <msxml3.dll>
using namespace MSXML2;

const char UNIT_SEPARATOR = 0x1F;
const streamsize BUFF_SIZE = 1024;
const int RMDIR_WAIT_SECONDS = 10;
char buffer[BUFF_SIZE];
string archivePath("archdir");
string indexBase("indexdir");
string baseurl, downloadUrl, studyDatePath;

time_t dcmdate2tm(int dcmdate)
{
  struct tm timeBirth;
  timeBirth.tm_year = dcmdate / 10000 - 1900;
  timeBirth.tm_mon = dcmdate % 10000 / 100 - 1;
  timeBirth.tm_mday = dcmdate % 100;
  timeBirth.tm_hour = 0;
  timeBirth.tm_min = 0;
  timeBirth.tm_sec = 0;
  return mktime(&timeBirth);
}

HRESULT getStudyNode(char *buffer, MSXML2::IXMLDOMDocumentPtr& pXMLDom, MSXML2::IXMLDOMElementPtr& study)
{
  string inputLine(buffer);
  istringstream patientStrm(inputLine);

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string patientId(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string patientsName(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string birthdate(buffer);

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string sex(buffer);

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string studyUID(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string studyDate(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE);
  string accNum(buffer);

  //studyDatePath = "/YYYY/MM/DD"
  studyDatePath.append(1, '/').append(studyDate.substr(0, 4)).append(1, '/').append(studyDate.substr(4, 2));
  studyDatePath.append(1, '/').append(studyDate.substr(6, 2));

  //downloadUrl = "<archivePath>/YYYY/MM/DD/<study uid>/"
  downloadUrl = archivePath;
  downloadUrl.append(studyDatePath).append(1, '/').append(studyUID).append(1, '/');

  HRESULT hr;
  hr = pXMLDom.CreateInstance(__uuidof(DOMDocument30));
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
  pi = pXMLDom->createProcessingInstruction("xml-stylesheet", "type='text/xml' href='/xslt/study.xsl'");
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
  ostringstream url;
  url << downloadUrl;
  if(seriesNumber.length() < 4)
	for(int i = seriesNumber.length(); i < 4; i++) url << '0';
  url << seriesNumber << '/';
  if(instanceNumber.length() < 8)
	for(int i = instanceNumber.length(); i < 8; i++) url << '0';
  url << instanceNumber;
  instance->setAttribute("DirectDownloadFile", url.str().c_str());
  url.clear();
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
		for(int i = 0; i < 1000; ++i)  // 10 seconds
		{
		  fh = ::CreateFile(indexFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		  if(fh == INVALID_HANDLE_VALUE)
		  {
			if(ERROR_SHARING_VIOLATION == ::GetLastError())
			{
			  cerr << "Waiting lock...\n";
			  ::Sleep(10);
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

		oldIndex.CreateInstance(__uuidof(DOMDocument30));
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
	return E_FAIL;
  }
  return S_OK;
}

HRESULT createDateIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom, const char *xslFile, string &indexFilePath, bool uniqueStudy)
{
  HRESULT hr;
  MSXML2::IXMLDOMDocumentPtr pXsl;
  hr = pXsl.CreateInstance(__uuidof(DOMDocument30));
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
	const char *header = "<?xml version=\"1.0\" encoding=\"gbk\"?>\n";
	_bstr_t xmlText(pXMLDom->transformNode(pXsl));
	MSXML2::IXMLDOMDocumentPtr dayDomPtr;
	dayDomPtr.CreateInstance(__uuidof(DOMDocument30));
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

HRESULT createKeyValueIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom, const char *tag, const char *queryValue)
{
  HRESULT hr;
  _bstr_t tagValue = pXMLDom->selectSingleNode(queryValue)->Gettext();
  unsigned int hash = hashCodeW(tagValue);
  if(tagValue.length() == 0) tagValue = "NULL";
  
  sprintf(buffer, "%s\\%s\\%02X\\%02X\\%02X\\%02X\\%s.xml", indexBase.c_str(), tag,
	hash >> 24 & 0xff, hash >> 16 & 0xff, hash >> 8 & 0xff, hash & 0xff, (const char*)tagValue);
  string indexPath = buffer;
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
		sprintf(buffer, "/wado_query/Patient/Study[@StudyInstanceUID='%s']", (const char*)studyUid);
		MSXML2::IXMLDOMNodePtr existStudy = oldIndex->selectSingleNode(buffer);
		if(existStudy) oldIndex->lastChild->firstChild->removeChild(existStudy); // /wado_query/Patient ->removeChild(existStudy)
		oldIndex->lastChild->firstChild->appendChild(newStudy); // /wado_query/Patient ->appendChild(newStudy)
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
  string::size_type p;
  string tagStudyUID(MK_TAG_STRING(DCM_StudyInstanceUID));
  while((p = tagStudyUID.find(',')) != string::npos) tagStudyUID.replace(p, 1, 0, '_');
  hr = createKeyValueIndex(pXMLDom, tagStudyUID.c_str(), "/wado_query/Patient/Study/@StudyInstanceUID");
  if (FAILED(hr))
  {
	cerr << "Failed to save StudyInstanceUID index\n";
	return hr;
  }

  // patient id index
  string tagPatientID(MK_TAG_STRING(DCM_PatientID));
  while((p = tagPatientID.find(',')) != string::npos) tagPatientID.replace(p, 1, 0, '_');
  hr = createKeyValueIndex(pXMLDom, tagPatientID.c_str(), "/wado_query/Patient/@PatientID");
  if (FAILED(hr))
  {
	cerr << "Failed to save PatientID index\n";
	return hr;
  }

  // main index OK, the following is other index.

  string tagStudyDate(MK_TAG_STRING(DCM_StudyDate));
  while((p = tagStudyDate.find(',')) != string::npos) tagStudyDate.replace(p, 1, 0, '_');
  //dayIndexFilePath = "<indexBase>/<tagStudyDate>/YYYY/MM/DD.xml"
  sprintf(buffer, "%s/%s/%s.xml", indexBase.c_str(), tagStudyDate.c_str(), studyDatePath.c_str());
  string dayIndexFilePath = buffer;
  hr = createDateIndex(pXMLDom, "xslt\\study.xsl", dayIndexFilePath, true);
  if (FAILED(hr))
	cerr << "Failed to create study date index: " << dayIndexFilePath << endl;

  // receive date index
  time_t t = time( NULL );
  struct tm *today = localtime( &t );
  strftime(buffer, BUFF_SIZE, "\\receive\\%Y\\%m\\%d.xml", today );
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

HRESULT generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath)
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
#ifndef _DEBUG
	if(hr == S_OK)
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
#endif
  }
  else
  {
	perror(inputFile);
	cerr << "error at open file: " << inputFile <<endl;
	hr = E_FAIL;
  }
  return hr;
}
