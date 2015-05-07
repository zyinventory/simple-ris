#include "stdafx.h"
#import <msxml3.dll>
#include "dcmdynamic.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcddirif.h"     /* for class DicomDirInterface */

using namespace std;
using namespace MSXML2;

static HRESULT CreateInstanceNode(DcmDirectoryRecord *instanceRec, MSXML2::IXMLDOMElementPtr &series, MSXML2::IXMLDOMDocumentPtr &pXMLDom, const char *studyUID, const char *seriesUID)
{
	MSXML2::IXMLDOMElementPtr instance = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Instance", "http://www.weasis.org/xsd");
	if(instance == NULL) return CO_E_NOT_SUPPORTED;
	const char *buff = NULL, *instanceUID = NULL, *transferSyntax = NULL;
	if(instanceRec->findAndGetString(DCM_ReferencedSOPInstanceUIDInFile, instanceUID).bad()) return CO_E_NOT_SUPPORTED;
	else instance->setAttribute("SOPInstanceUID", instanceUID);
	if(instanceRec->findAndGetString(DCM_ReferencedTransferSyntaxUIDInFile, transferSyntax).bad()) return CO_E_NOT_SUPPORTED;
	else instance->setAttribute("TransferSyntaxUID", transferSyntax);

	char url[512];
	sprintf_s(url, "getindex.exe?requestType=WADO&studyUID=%s&seriesUID=%s&objectUID=%s&contentType=application%%2Fdicom&transferSyntax=%s", 
		studyUID, seriesUID, instanceUID, transferSyntax);
	instance->setAttribute("DirectDownloadFile", url);

	if(instanceRec->findAndGetString(DCM_InstanceNumber, buff).good()) instance->setAttribute("InstanceNumber", buff);
	series->appendChild(instance);
	return S_OK;
}

static HRESULT CreateSeriesNode(DcmDirectoryRecord *seriesRec, MSXML2::IXMLDOMElementPtr &study, MSXML2::IXMLDOMDocumentPtr &pXMLDom, const char *studyUID)
{
	MSXML2::IXMLDOMElementPtr series = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Series", "http://www.weasis.org/xsd");
	if(series == NULL) return CO_E_NOT_SUPPORTED;

	const char *buff = NULL, *seriesUID = NULL;
	if(seriesRec->findAndGetString(DCM_SeriesInstanceUID, seriesUID).bad()) return CO_E_NOT_SUPPORTED;
	else series->setAttribute("SeriesInstanceUID", seriesUID);
	if(seriesRec->findAndGetString(DCM_Modality, buff).bad()) return CO_E_NOT_SUPPORTED;
	else series->setAttribute("Modality", buff);
	if(seriesRec->findAndGetString(DCM_SeriesDescription, buff).good()) series->setAttribute("SeriesDescription", buff);
	if(seriesRec->findAndGetString(DCM_SeriesNumber, buff).good()) series->setAttribute("SeriesNumber", buff);
	study->appendChild(series);

	DcmDirectoryRecord *instanceRec = NULL;
	while(instanceRec = seriesRec->nextSub(instanceRec))
	{
		if(instanceRec->getRecordType() != ERT_Image) continue;
		CreateInstanceNode(instanceRec, series, pXMLDom, studyUID, seriesUID);
	}

	return S_OK;
}

static HRESULT CreateStudyNode(DcmDirectoryRecord *studyRec, MSXML2::IXMLDOMElementPtr &patient, MSXML2::IXMLDOMDocumentPtr &pXMLDom)
{
	MSXML2::IXMLDOMElementPtr study = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Study", "http://www.weasis.org/xsd");
	if(study == NULL) return CO_E_NOT_SUPPORTED;

	const char *buff = NULL, *studyUID = NULL;
	if(studyRec->findAndGetString(DCM_StudyInstanceUID, studyUID).bad()) return CO_E_NOT_SUPPORTED;
	else study->setAttribute("StudyInstanceUID", studyUID);
	if(studyRec->findAndGetString(DCM_StudyDate, buff).bad()) return CO_E_NOT_SUPPORTED;
	else study->setAttribute("StudyDate", buff);
	if(studyRec->findAndGetString(DCM_StudyTime, buff).good()) study->setAttribute("StudyTime", buff);
	if(studyRec->findAndGetString(DCM_StudyID, buff).good()) study->setAttribute("StudyID", buff);
	if(studyRec->findAndGetString(DCM_AccessionNumber, buff).good()) study->setAttribute("AccessionNumber", buff);
	if(studyRec->findAndGetString(DCM_StudyDescription, buff).good()) study->setAttribute("StudyDescription", buff);
	if(studyRec->findAndGetString(DCM_ReferringPhysiciansName, buff).good()) study->setAttribute("ReferringPhysicianName", buff);
	patient->appendChild(study);

	DcmDirectoryRecord *seriesRec = NULL;
	while(seriesRec = studyRec->nextSub(seriesRec))
	{
		if(seriesRec->getRecordType() != ERT_Series) continue;
		CreateSeriesNode(seriesRec, study, pXMLDom, studyUID);
	}
	return S_OK;
}

static HRESULT CreatePatientNode(DcmDirectoryRecord *patientRecord, MSXML2::IXMLDOMDocumentPtr &pXMLDom)
{
	MSXML2::IXMLDOMElementPtr patient, wado;
	wado = pXMLDom->selectSingleNode("/wado_query");
	if(wado == NULL) return CO_E_NOT_SUPPORTED;
	patient = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "Patient", "http://www.weasis.org/xsd");
	if(patient == NULL) return CO_E_NOT_SUPPORTED;

	const char *patientName = NULL, *patientID = NULL, *patientBirthDate = NULL, *patientBirthTime = NULL, *patientSex = NULL;
	if(patientRecord->findAndGetString(DCM_PatientsName, patientName).bad()) return CO_E_NOT_SUPPORTED;
	else patient->setAttribute("PatientName", patientName);
	if(patientRecord->findAndGetString(DCM_PatientID, patientID).bad()) return CO_E_NOT_SUPPORTED;
	else patient->setAttribute("PatientID", patientID);
	if(patientRecord->findAndGetString(DCM_PatientsBirthDate, patientBirthDate).good())
		patient->setAttribute("PatientBirthDate", patientBirthDate);
	if(patientRecord->findAndGetString(DCM_PatientsBirthTime, patientBirthTime).good())
		patient->setAttribute("PatientBirthTime", patientBirthTime);
	if(patientRecord->findAndGetString(DCM_PatientsSex, patientSex).good())
		patient->setAttribute("PatientSex", patientSex);

	wado->appendChild(patient);

	DcmDirectoryRecord *studyRec = NULL;
	while(studyRec = patientRecord->nextSub(studyRec))
	{
		if(studyRec->getRecordType() != ERT_Study) continue;
		CreateStudyNode(studyRec, patient, pXMLDom);
	}
	return S_OK;
}

static HRESULT CreateWADO(MSXML2::IXMLDOMDocumentPtr &pXMLDom)
{
	HRESULT hr = pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr)) return hr;
	pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
	pXMLDom->async = VARIANT_FALSE;

	MSXML2::IXMLDOMElementPtr wado;
	MSXML2::IXMLDOMProcessingInstructionPtr pi;
	pi = pXMLDom->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"GBK\"");
	if (pi != NULL)
	{
		pXMLDom->appendChild(pi);
		pi.Release();
	}

	pi = pXMLDom->createProcessingInstruction("xml-stylesheet", "type=\"text/xml\" href=\"../xslt/wadolist.xsl\"");
	if (pi != NULL)
	{
		pXMLDom->appendChild(pi);
		pi.Release();
	}

	wado = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "wado_query", "http://www.weasis.org/xsd");
	wado->setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	wado->setAttribute("requireOnlySOPInstanceUID", "false");
	pXMLDom->appendChild(wado);
	wado->setAttribute("wadoURL", "http://localhost/pacs/cgi-bin/");
	/*
	MSXML2::IXMLDOMElementPtr httpTag;
	httpTag = pXMLDom->selectSingleNode("/wado_query/httpTag");
	if( ! httpTag )
	{
		httpTag = pXMLDom->createNode(MSXML2::NODE_ELEMENT, "httpTag", "http://www.weasis.org/xsd");
		wado->appendChild(httpTag);
	}
	httpTag->setAttribute("key", "callingAE");
	httpTag->setAttribute("value", "DEVICE");
	*/
	return S_OK;
}

static HRESULT RootRecord(const char *dirfile, const char *xmlfile)
{
	DcmDicomDir dir(dirfile);
	DcmDirectoryRecord &root = dir.getRootRecord(), *patient = NULL;
	MSXML2::IXMLDOMDocumentPtr pXMLDom;
	HRESULT hr = CreateWADO(pXMLDom);
	if (FAILED(hr))
	{
		cerr << "Failed to CreateInstance on an XML DOM." << endl;
		return hr;
	}
	while(patient = root.nextSub(patient))
	{
		if(patient->getRecordType() != ERT_Patient) continue;
		hr = CreatePatientNode(patient, pXMLDom);
		if(FAILED(hr))
		{
			cerr << "CreatePatientNode() failed" << endl;
			break;
		}
	}
	hr = pXMLDom->save(xmlfile);
	return hr;
}

DCMDYNAMIC_API bool DicomDir2Xml(const char *dirfile, const char *xmlfile)
{
	if(dirfile == NULL || xmlfile == NULL) return CO_E_NOT_SUPPORTED;
	CoInitialize(NULL);
	HRESULT hr = S_OK;
	try
	{
		hr = RootRecord(dirfile, xmlfile);
	}
	catch(_com_error &ex) 
	{
		cerr << ex.ErrorMessage() << endl;
		hr = ex.Error();
	}
	CoUninitialize();
	return SUCCEEDED(hr);	
}
