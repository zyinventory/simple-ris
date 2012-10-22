// poststore.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>

using namespace std;
#import <msxml3.dll>
using namespace MSXML2;

const char UNIT_SEPARATOR = 0x1F;
const int STATUS_Normal = 0, STATUS_Error = -1, STATUS_EOF = 1;
const streamsize BUFF_SIZE = 1024;
char buffer[BUFF_SIZE + 1];
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
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string patientsName(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string studyUID(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE, UNIT_SEPARATOR);
  string studyDate(buffer);
  if(! patientStrm.good()) { cerr << "Failed to resolve input data: " << inputLine << endl; return E_FAIL; }

  patientStrm.getline(buffer, BUFF_SIZE);
  string accNum(buffer);

  //paht += "YYYY/MM/DD/<study uid>/"
  path.append(studyDate.substr(0, 4)).append(1, '/').append(studyDate.substr(4, 2)).append(1, '/').append(studyDate.substr(6, 2)).append(1, '/');
  path.append(studyUID).append(1, '/');

  savePath = path;
  string::size_type p;
  while((p = savePath.find('/')) != string::npos) savePath.replace(p, 1, 1, '\\');
  savePath.append("index.xml");

  HRESULT hr;
  hr = pXMLDom.CreateInstance(__uuidof(DOMDocument30));
  if (FAILED(hr))
  {
	cerr << "Failed to CreateInstance on an XML DOM.\n";
	return hr;
  }
  pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
  pXMLDom->async = VARIANT_FALSE;

  if( pXMLDom->load(savePath.c_str()) == VARIANT_TRUE )
  {
	study = pXMLDom->selectSingleNode("/wado_query/Patient/Study");
  }
  else
  {
	MSXML2::IXMLDOMProcessingInstructionPtr pi;
	pi = pXMLDom->createProcessingInstruction("xml", "version='1.0' encoding='GBK'");
	if (pi != NULL)
	{
	  pXMLDom->appendChild(pi);
	  pi.Release();
	}

	MSXML2::IXMLDOMElementPtr wado;
	wado = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "wado_query", "http://www.weasis.org/xsd");
	wado->setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	wado->setAttribute("wadoURL", "http://localhost/");
	wado->setAttribute("requireOnlySOPInstanceUID", "false");
	pXMLDom->appendChild(wado);

	MSXML2::IXMLDOMElementPtr patient;
	patient = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Patient", "http://www.weasis.org/xsd");
	patient->setAttribute("PatientID", patientId.c_str());
	patient->setAttribute("PatientName", patientsName.c_str());
	wado->appendChild(patient);

	study = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Study", "http://www.weasis.org/xsd");
	study->setAttribute("StudyInstanceUID", studyUID.c_str());
	study->setAttribute("StudyDate", studyDate.c_str());
	study->setAttribute("AccessionNumber", accNum.c_str());
	patient->appendChild(study);
  }
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
	series->setAttribute("SeriesNumber", seriesNumber.c_str());
	series->setAttribute("Modality", modality.c_str());
	study->appendChild(series);
  }

  instance = series->selectSingleNode(instanceQuery.c_str());
  if( ! instance )
  {
	instance = study->ownerDocument->createNode(MSXML2::NODE_ELEMENT, "Instance", "http://www.weasis.org/xsd");
	instance->setAttribute("SOPInstanceUID", instanceUID.c_str());
	instance->setAttribute("InstanceNumber", instanceNumber.c_str());
	string url(path);
	url.append(seriesUID).append(1, '/').append(instanceUID).append(".DCM");
	instance->setAttribute("DirectDownloadFile", url.c_str());
	series->appendChild(instance);
  }
  return S_OK;
}

HRESULT processInputStream(istream& istrm)
{
  istrm.getline(buffer, BUFF_SIZE);

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

  hr = pXMLDom->save(savePath.c_str());
  if (FAILED(hr))
  {
	cerr << "Failed to save DOM to tree.xml\n";
	return hr;
  }

  if (pXMLDom) pXMLDom.Release();
  return S_OK;
}

int _tmain(int argc, _TCHAR* argv[])
{
  CoInitialize(NULL);
  HRESULT hr;
  if(argc == 1)
  {
	hr = processInputStream(cin);
  }
  else
  {
	ifstream infile(argv[1]);
	if(infile)
	{
	  hr = processInputStream(infile);
	  infile.close();
	  remove(argv[1]);
	}
	else
	{
	  cerr << "Can't open file " << argv[1] << endl;
	  hr = E_FAIL;
	}
  }
  CoUninitialize();
  if(FAILED(hr)) cout << (char)7 << endl;
  return static_cast<int>(hr);
}
