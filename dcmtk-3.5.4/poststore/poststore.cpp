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

using namespace std;
#import <msxml3.dll>
using namespace MSXML2;

const char UNIT_SEPARATOR = 0x1F;
const streamsize BUFF_SIZE = 1024;
const int RMDIR_WAIT_SECONDS = 10;
char buffer[BUFF_SIZE + 1];
string baseurl("http://localhost/pacs/");
string path("archdir/");
string savePath;

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
  if(! patientStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string patientsName(buffer);
  if(! patientStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string birthdate(buffer);

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string sex(buffer);

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string studyUID(buffer);
  if(! patientStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string studyDate(buffer);
  if(! patientStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE);
  string accNum(buffer);

  //paht += "YYYY/MM/DD/<study uid>/"
  path.append(studyDate.substr(0, 4)).append(1, '/').append(studyDate.substr(4, 2)).append(1, '/').append(studyDate.substr(6, 2));
  path.append(1, '/').append(studyUID).append(1, '/');

  savePath = path;
  string::size_type p;
  while((p = savePath.find('/')) != string::npos) savePath.replace(p, 1, 1, '\\');
  savePath.append("index.xml");

  HRESULT hr;
  hr = pXMLDom.CreateInstance(__uuidof(DOMDocument30));
  if (FAILED(hr))
  {
	clog << "Failed to CreateInstance on an XML DOM.\n";
	return hr;
  }
  pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
  pXMLDom->async = VARIANT_FALSE;

  MSXML2::IXMLDOMElementPtr wado;
  if( pXMLDom->load(savePath.c_str()) == VARIANT_TRUE )
  {
	wado = pXMLDom->selectSingleNode("/wado_query");
  }
  else
  {
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
  }
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
  if(! instStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string seriesNumber(buffer);
  if(! instStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string modality(buffer);
  if(! instStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  instStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string instanceUID(buffer);
  if(! instStrm.good()) { clog << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

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
  string url(path);
  url.append(seriesUID).append(1, '/').append(instanceUID).append(".DCM");
  instance->setAttribute("DirectDownloadFile", url.c_str());
  return S_OK;
}

HRESULT createStudyDateIndex(MSXML2::IXMLDOMDocumentPtr pXMLDom)
{
  HRESULT hr;
  MSXML2::IXMLDOMDocumentPtr pXsl;
  hr = pXsl.CreateInstance(__uuidof(DOMDocument30));
  if (FAILED(hr))
  {
	clog << "Failed to CreateInstance on an XSL DOM.\n";
	return hr;
  }
  pXsl->preserveWhiteSpace = VARIANT_FALSE;
  pXsl->async = VARIANT_FALSE;

  if(pXsl->load("xslt\\study.xsl") == VARIANT_FALSE)
  {
	clog << "Failed to load XSL DOM: xslt\\study.xsl\n";
	return E_FAIL;
  }

  string errorMessage;
  string dayIndexFilePath("archdir/");
  HANDLE fh;
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
	dayIndexFilePath.append(day->attributes->getNamedItem("StudyDate")->text).append("/index_day.xml");

	fh = ::CreateFile(dayIndexFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fh == INVALID_HANDLE_VALUE)
	{
	  if(ERROR_FILE_EXISTS == ::GetLastError())
	  {
		for(int i = 0; i < 1000; ++i)  // 10 seconds
		{
		  fh = ::CreateFile(dayIndexFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		  if(fh == INVALID_HANDLE_VALUE)
		  {
			if(ERROR_SHARING_VIOLATION == ::GetLastError())
			{
			  clog << "Waiting lock...\n";
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

		MSXML2::IXMLDOMDocumentPtr oldIndex;
		oldIndex.CreateInstance(__uuidof(DOMDocument30));
		oldIndex->load(dayIndexFilePath.c_str());
		string query("/Day/Study[text()='");
		query.append(day->firstChild->text).append("']");
		MSXML2::IXMLDOMNodePtr existStudy = oldIndex->selectSingleNode(query.c_str());
		if(existStudy) oldIndex->lastChild->removeChild(existStudy);
		oldIndex->lastChild->appendChild(day->firstChild);
		::SetEndOfFile(fh);
		if( ! ( WriteFile(fh, header, strlen(header), &written, NULL) && 
		  WriteFile(fh, (const char *)oldIndex->lastChild->xml, strlen((const char *)oldIndex->lastChild->xml), &written, NULL) ) )
		{
		  throw "Failed to write file ";
		}
	  }
	  else
	  {
		throw "Failed to create file ";
	  }
	}
	else
	{
	  if( ! ( WriteFile(fh, header, strlen(header), &written, NULL) && 
		WriteFile(fh, (const char *)day->xml, strlen((const char *)day->xml), &written, NULL) ) )
	  {
		throw "Failed to write file ";
	  }
	}
	::CloseHandle(fh);
  }
  catch(_com_error &ex) 
  {
	clog << "Failed to transform XML+XSLT: " << ex.ErrorMessage() << '\n';
	return ex.Error();
  }
  catch(char * message)
  {
	if(fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
	clog << message << dayIndexFilePath << '\n';
	return E_FAIL;
  }
  return S_OK;
}

HRESULT processInputStream(istream& istrm)
{
  istrm.getline(buffer, BUFF_SIZE);
  if( ! istrm.good() )
  {
	clog << "Failed to read input file\n";
	return E_FAIL;
  }

  HRESULT hr;
  MSXML2::IXMLDOMDocumentPtr pXMLDom;
  MSXML2::IXMLDOMElementPtr study;
  
  hr = getStudyNode(buffer, pXMLDom, study);
  if (FAILED(hr)) 
  {
	clog << "Failed to create DOM\n";
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

  hr = pXMLDom->save(savePath.c_str());
  if (FAILED(hr))
  {
	clog << "Failed to save DOM to tree.xml\n";
	return hr;
  }

  return createStudyDateIndex(pXMLDom);
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
	clog << messageHeader << param <<endl;
  }

  return ! opFail;
}

int _tmain(int argc, _TCHAR* argv[])
{
  char* inputFile;
  CoInitialize(NULL);
  HRESULT hr;
  if(argc == 1 || strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "-h") == 0 
	|| strcmp(argv[1], "--help") == 0)
  {
	cout << "usage: poststore baseurl infile\n"
	  << "baseurl: base url, must start with http://.\n"
	  << "infile: input file, - is stdin." << endl;
	return 0;
  }

  if(argc == 2)
  {
	if( strncmp(argv[1], "http://", 7) == 0 )
	{
	  baseurl = argv[1];
	  inputFile = "-";
	}
	else
	  inputFile = argv[1];
  }
  else
  {
	baseurl = argv[1];
	inputFile = argv[2];
  }

  if(inputFile == "-")
  {
	hr = processInputStream(cin);
  }
  else
  {
	ifstream infile(inputFile);
	if(infile)
	{
	  hr = processInputStream(infile);
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
	  clog << "error at open file: " << inputFile <<endl;
	  hr = E_FAIL;
	}
  }
  CoUninitialize();
  if(FAILED(hr)) cout << (char)7 << endl;
  return static_cast<int>(hr);
}
