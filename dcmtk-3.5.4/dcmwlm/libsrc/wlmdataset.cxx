#include "dcmtk/dcmnet/dicom.h"
#include <dcmtk/dcmdata/dcdatset.h>
#include "dcmtk/dcmdata/dcvras.h"     // for DcmAgeString
#include "dcmtk/dcmdata/dcvrlo.h"     // for DcmLongString
#include "dcmtk/dcmdata/dcvrae.h"	  // for DcmApplicationEntity
#include "dcmtk/dcmdata/dcvrda.h"	  // for DcmDate
#include "dcmtk/dcmdata/dcvrds.h"	  // for DcmDecimalString
#include "dcmtk/dcmdata/dcvrcs.h"	  // for DcmCodeString
#include "dcmtk/dcmdata/dcvrpn.h"	  // for DcmPersonName
#include "dcmtk/dcmdata/dcvrsh.h"	  // for DcmShortString
#include "dcmtk/dcmdata/dcvrtm.h"	  // for DcmTime
#include "dcmtk/dcmdata/dcdeftag.h"	  // for DCM_OffendingElement, ...
#include "dcmtk/dcmdata/dcsequen.h"	  // for DcmSequenceOfItems
#include "dcmtk/dcmwlm/wltypdef.h"
#include "dcmtk/dcmwlm/wlmdbim.h"
#include "bridge.h"

static void DumpPosition(DcmTag& tag, const char *failedPosition, DcmDataset *&dataset, DcmItem *item = NULL)
{
  CERR << tag.getTagName() << failedPosition << endl;
  if(item != NULL) delete item;
  if(dataset != NULL) delete dataset;
  dataset = NULL;
}

static void TranslateIndicatorMessage(short indicator, DcmTag& tag, OFBool type1 = OFFalse)
{
  if(indicator > 0)
	CERR << WARNING << tag.getTagName() << VALUE_LENGTH_EXCEED << indicator << endl;
  else if(indicator == -2)
	CERR << WARNING << tag.getTagName() << VALUE_LENGTH_UNKNOWN << endl;
  else if(indicator == -1 && type1)
	CERR << WARNING << tag.getTagName() << TYPE_1_IS_NULL << endl;
}

static OFCondition SetDefaultValue(DcmElement *pElement, const char *value, short indicator = -1)
{
  DcmVR vr(pElement->getVR());
  char cpValue[DIC_NODENAME_LEN + 1]; // 129
  Uint32 vlen = min(vr.getMaxValueLength(), DIC_NODENAME_LEN);

  DcmTag tag = pElement->getTag();
  strncpy(cpValue, value, vlen + 1);
  cpValue[vlen] = '\0';

  if(indicator < 0) // NULL
  	CERR << WARNING << tag.getTagName() << NVL_VALUE << cpValue << endl;
  else // not support Chinese
	CERR << WARNING << tag.getTagName() << ASCII_VALUE << cpValue << endl;

  return pElement->putString(cpValue);
}

void WlmDBInteractionManager::WlmDataset(PWorklistRecord pWorklist, PIndicatorWorklistRecord pIndicator, WlmDBInteractionManager *dbim)
{
  if(pWorklist == NULL || pIndicator == NULL) return;

  rtrim(pWorklist->ScheduledStationAETitle, DIC_AE_LEN);
  rtrim(pWorklist->SchdldProcStepStartDate, DIC_DA_LEN);
  rtrim(pWorklist->SchdldProcStepStartTime, DIC_TM_LEN);
  rtrim(pWorklist->Modality, DIC_CS_LEN);
  rtrim(pWorklist->SchdldProcStepDescription, DIC_LO_LEN);
  rtrim(pWorklist->SchdldProcStepLocation, DIC_SH_LEN);
  rtrim(pWorklist->SchdldProcStepID, DIC_SH_LEN);
  rtrim(pWorklist->RequestedProcedureID, DIC_SH_LEN);
  rtrim(pWorklist->RequestedProcedureDescription, DIC_LO_LEN);
  rtrim(pWorklist->StudyInstanceUID, DIC_UI_LEN);
  rtrim(pWorklist->AccessionNumber, DIC_SH_LEN);
  rtrim(pWorklist->RequestingPhysician, DIC_PN_LEN);
  rtrim(pWorklist->AdmissionID, DIC_LO_LEN);
  rtrim(pWorklist->PatientsNameEn, DIC_PN_LEN);
  rtrim(pWorklist->PatientsNameCh, DIC_PN_LEN);
  rtrim(pWorklist->PatientID, DIC_LO_LEN);
  rtrim(pWorklist->PatientsBirthDate, DIC_DA_LEN);
  rtrim(pWorklist->PatientsSex, DIC_CS_LEN);
  rtrim(pWorklist->PatientsWeight, DIC_DS_LEN);
  rtrim(pWorklist->AdmittingDiagnosesDescription, DIC_LO_LEN);
  rtrim(pWorklist->PatientsAge, DIC_AS_LEN);

  OFCondition cond = EC_Normal;
  DcmDataset *dataset = new DcmDataset();

  DcmElement *pElement = NULL;
  OFBool requestChinese = OFFalse;
  cond = dbim->searchMaskIdentifiers->findAndGetElement(DCM_SpecificCharacterSet, pElement, OFTrue);
  if(cond.good())
  {
	OFString charset;
	cond = pElement->getOFString(charset, 0);
	requestChinese = cond.good() && charset == CHARSET_GB18030;
  }

  {
	DcmTag tag(DCM_SpecificCharacterSet);	//0008,0005 O 1
	DcmCodeString *charset = new DcmCodeString(tag);
	if(pWorklist->SupportChinese == 0)
	  cond = charset->putString(CHARSET_ISO_IR_100);
	else if(pWorklist->SupportChinese == 1)
	  cond = charset->putString(CHARSET_GB18030);
	else
	{
	  if(requestChinese)
		cond = charset->putString(CHARSET_GB18030);
	  else
		cond = charset->putString(CHARSET_ISO_IR_100);
	}
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(charset);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_AccessionNumber);	//0008,0050 O 2
	DcmShortString *accessNum = new DcmShortString(tag);
	TranslateIndicatorMessage(pIndicator->AccessionNumber, tag);
	if(pIndicator->AccessionNumber >= 0) cond = accessNum->putString(pWorklist->AccessionNumber);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(accessNum, OFTrue);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  dataset->insert(new DcmPersonName(DcmTag(DCM_ReferringPhysiciansName)));  // 0008,0090 O 2

  {
	DcmTag tag(DCM_AdmittingDiagnosesDescription);  //0008,1080 O 3
	if(pIndicator->AdmittingDiagnosesDescription >= 0)
	{
	  if(pWorklist->SupportChinese == 1 || IsASCII(pWorklist->AdmittingDiagnosesDescription))
	  {
		TranslateIndicatorMessage(pIndicator->AdmittingDiagnosesDescription, tag);
		DcmLongString *admissionDiag = new DcmLongString(tag);
		cond = admissionDiag->putString(pWorklist->AdmittingDiagnosesDescription);
		if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
		cond = dataset->insert(admissionDiag);
		if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
	  }
	}
  }

  cond = dataset->insert(new DcmSequenceOfItems(DcmTag(DCM_ReferencedStudySequence))); //0008,1110 O 2
  if(cond.bad()) { DumpPosition(DcmTag(DCM_ReferencedStudySequence), INSERT_FAILED, dataset); return; }

  cond = dataset->insert(new DcmSequenceOfItems(DcmTag(DCM_ReferencedPatientSequence))); //0008,1120 O 2
  if(cond.bad()) { DumpPosition(DcmTag(DCM_ReferencedPatientSequence), INSERT_FAILED, dataset); return; }

  {
	DcmTag tag(DCM_PatientsName);  // 0010,0010 R 1
	DcmPersonName *patientName = new DcmPersonName(tag);
	OFString pn;
	if(pWorklist->SupportChinese == 0 && pIndicator->PatientsNameEn >= 0)
	  pn.append(pWorklist->PatientsNameEn);
	else if(pWorklist->SupportChinese == 1)
	{
	  if(pWorklist->DicomPersonName == 1)
	  {
		if(pIndicator->PatientsNameEn >= 0) pn.append(pWorklist->PatientsNameEn);
		pn.append(1, '=');
	  }
	  if(pIndicator->PatientsNameCh >= 0) pn.append(pWorklist->PatientsNameCh);
	}
	normalizeString(pn, !MULTIPART, DELETE_LEADING, DELETE_TRAILING);
	if(pn.length() == 0)
	  cond = SetDefaultValue(patientName, ANONYMOUS);
	else
	  cond = patientName->putString(pn.c_str());
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(patientName);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_PatientID);  //0010,0020 R 1
	TranslateIndicatorMessage(pIndicator->PatientID, tag, OFTrue);
	DcmLongString *patientId = new DcmLongString(tag);
	if(pIndicator->PatientID >= 0)
	  cond = patientId->putString(pWorklist->PatientID);
	else
	  cond = SetDefaultValue(patientId, ANONYMOUS);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(patientId);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_PatientsBirthDate);  //0010,0030 O 2
	DcmDate *birthDate = new DcmDate(tag);
	TranslateIndicatorMessage(pIndicator->PatientsBirthDate, tag);
	if(pIndicator->PatientsBirthDate >= 0) cond = birthDate->putString(pWorklist->PatientsBirthDate);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(birthDate);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_PatientsSex);  ////0010,0040 O 2
	DcmCodeString *patientSex = new DcmCodeString(tag);
	TranslateIndicatorMessage(pIndicator->PatientsSex, tag);
	if(pIndicator->PatientsSex >= 0) cond = patientSex->putString(pWorklist->PatientsSex);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(patientSex);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_PatientsAge);  //0010,1010 O 3
	DcmAgeString *patientAge = new DcmAgeString(tag);
	TranslateIndicatorMessage(pIndicator->PatientsAge, tag);
	if(pIndicator->PatientsAge >= 0) cond = patientAge->putString(pWorklist->PatientsAge);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(patientAge);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_PatientsWeight);  //0010,1030 O 2
	DcmDecimalString *patientWeight = new DcmDecimalString(tag);
	TranslateIndicatorMessage(pIndicator->PatientsWeight, tag);
	if(pIndicator->PatientsWeight >= 0) cond = patientWeight->putString(pWorklist->PatientsWeight);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(patientWeight);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_StudyInstanceUID);  //0020,000d O 1
	TranslateIndicatorMessage(pIndicator->StudyInstanceUID, tag, OFTrue);
	if(pIndicator->StudyInstanceUID >= 0)
	{
	  DcmUniqueIdentifier *studyInstUid = new DcmUniqueIdentifier(tag);
	  cond = studyInstUid->putString(pWorklist->StudyInstanceUID);
	  if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	  cond = dataset->insert(studyInstUid);
	  if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
	}
  }

  {
	DcmTag tag(DCM_RequestingPhysician);  // 0032,1032 O 2
	DcmPersonName *reqPhyName = new DcmPersonName(tag);
	TranslateIndicatorMessage(pIndicator->RequestingPhysician, tag);
	if(pIndicator->RequestingPhysician >= 0 && pWorklist->SupportChinese == 1) cond = reqPhyName->putString(pWorklist->RequestingPhysician);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(reqPhyName);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_RequestedProcedureDescription);  //0032,1060 O 1
	DcmLongString *reqProcDesc = new DcmLongString(tag);
	TranslateIndicatorMessage(pIndicator->RequestedProcedureDescription, tag, OFTrue);
	if(pIndicator->RequestedProcedureDescription >= 0)
	{
	  if(pWorklist->SupportChinese == 1 || IsASCII(pWorklist->RequestedProcedureDescription))
		cond = reqProcDesc->putString(pWorklist->RequestedProcedureDescription);
	  else
		cond = SetDefaultValue(reqProcDesc, tag.getTagName(), pIndicator->RequestedProcedureDescription);
	}
	else
	  cond = SetDefaultValue(reqProcDesc, tag.getTagName());
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(reqProcDesc);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_AdmissionID);  //0038,0010 O 2
	DcmLongString *admissionId = new DcmLongString(tag);
	TranslateIndicatorMessage(pIndicator->AdmissionID, tag);
	if(pIndicator->AdmissionID >= 0) cond = admissionId->putString(pWorklist->AdmissionID);
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(admissionId);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  {
	DcmTag tag(DCM_RequestedProcedurePriority);  //0040,1003 O 2
	DcmShortString *reqProcPrior = new DcmShortString(tag);
	cond = reqProcPrior->putString(MEDIUM); //STAT, HIGH, ROUTINE, MEDIUM, LOW 
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(reqProcPrior);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  DcmSequenceOfItems *schdlProcSQ = new DcmSequenceOfItems(DcmTag(DCM_ScheduledProcedureStepSequence)); //0040,0100 R 1
  {
	DcmItem *item = new DcmItem();
	
	{
	  DcmTag tag(DCM_Modality);	//0008,0060 R 1
	  TranslateIndicatorMessage(pIndicator->Modality, tag, OFTrue);
	  if(pIndicator->Modality >= 0)
	  {
		DcmCodeString *modality = new DcmCodeString(tag);
		cond = modality->putString(pWorklist->Modality);
		if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
		cond = item->insert(modality);
		if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	  }
	}

	{
	  DcmTag tag(DCM_ScheduledStationAETitle);  //0040,0001 R 1
	  TranslateIndicatorMessage(pIndicator->ScheduledStationAETitle, tag, OFTrue);
	  if(pIndicator->ScheduledStationAETitle >= 0)
	  {
		DcmApplicationEntity *schdlStationAE = new DcmApplicationEntity(tag);
		cond = schdlStationAE->putString(pWorklist->ScheduledStationAETitle);
		if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
		cond = item->insert(schdlStationAE);
		if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	  }
	}

	{
	  DcmTag tag(DCM_ScheduledProcedureStepStartDate);	//0040,0002 R 1
	  TranslateIndicatorMessage(pIndicator->SchdldProcStepStartDate, tag, OFTrue);
	  if(pIndicator->SchdldProcStepStartDate >= 0)
	  {
		DcmDate *schdlDate = new DcmDate(tag);
		cond = schdlDate->putString(pWorklist->SchdldProcStepStartDate);
		if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
		cond = item->insert(schdlDate);
		if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	  }
	}

	{
	  DcmTag tag(DCM_ScheduledProcedureStepStartTime);  //0040,0003 R 1
	  TranslateIndicatorMessage(pIndicator->SchdldProcStepStartTime, tag, OFTrue);
	  if(pIndicator->SchdldProcStepStartTime >= 0)
	  {
		DcmTime *schdlTime = new DcmTime(tag);
		cond = schdlTime->putString(pWorklist->SchdldProcStepStartTime);
		if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
		cond = item->insert(schdlTime);
		if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	  }
	}

	cond = item->insert(new DcmPersonName(DCM_ScheduledPerformingPhysiciansName));  // 0040,0006 R 2
	if(cond.bad()) { DumpPosition(DcmTag(DCM_ScheduledPerformingPhysiciansName), INSERT_FAILED, dataset, item); return; }

	{
	  DcmTag tag(DCM_ScheduledProcedureStepDescription);  //0040,0007 O 1
	  TranslateIndicatorMessage(pIndicator->SchdldProcStepDescription, tag, OFTrue);
	  DcmLongString *schdlProcStepDesc = new DcmLongString(tag);
	  if(pIndicator->SchdldProcStepDescription >= 0)
	  {
		if(pWorklist->SupportChinese == 1 || IsASCII(pWorklist->SchdldProcStepDescription))
		  cond = schdlProcStepDesc->putString(pWorklist->SchdldProcStepDescription);
		else
		  cond = SetDefaultValue(schdlProcStepDesc, tag.getTagName(), pIndicator->SchdldProcStepDescription);
	  }
	  else
		cond = SetDefaultValue(schdlProcStepDesc, tag.getTagName());
	  if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
	  cond = item->insert(schdlProcStepDesc);
	  if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	}

	{
	  DcmTag tag(DCM_ScheduledProcedureStepID);  //0040,0009 O 1
	  TranslateIndicatorMessage(pIndicator->SchdldProcStepID, tag, OFTrue);
	  DcmShortString *schdlProcStepId = new DcmShortString(tag);
	  if(pIndicator->SchdldProcStepID >= 0)
	  {
		if(pWorklist->SupportChinese == 1 || IsASCII(pWorklist->SchdldProcStepID))
		  cond = schdlProcStepId->putString(pWorklist->SchdldProcStepID);
		else
		  cond = SetDefaultValue(schdlProcStepId, tag.getTagName(), pIndicator->SchdldProcStepID);
	  }
	  else
		cond = SetDefaultValue(schdlProcStepId, tag.getTagName());
	  if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
	  cond = item->insert(schdlProcStepId);
	  if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	}

  	{
	  DcmTag tag(DCM_ScheduledProcedureStepLocation);  //0040,0011 O 2
	  TranslateIndicatorMessage(pIndicator->SchdldProcStepLocation, tag);
	  DcmShortString *schdlProcStepLocation = new DcmShortString(tag);
	  if(pIndicator->SchdldProcStepLocation >= 0)
	  {
		if(pWorklist->SupportChinese == 1 || IsASCII(pWorklist->SchdldProcStepLocation))
		  cond = schdlProcStepLocation->putString(pWorklist->SchdldProcStepLocation);
	  }
	  if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset, item); return; }
	  cond = item->insert(schdlProcStepLocation);
	  if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
	}

	cond = schdlProcSQ->append(item);
	if(cond.bad()) { DcmTag tag(DCM_ScheduledProcedureStepSequence); DumpPosition(tag, INSERT_FAILED, dataset, item); return; }
  }
  cond = dataset->insert(schdlProcSQ);
  if(cond.bad()) { DcmTag tag(DCM_ScheduledProcedureStepSequence); DumpPosition(tag, INSERT_FAILED, dataset); return; }

  {
	DcmTag tag(DCM_RequestedProcedureID);  //0040,1001 O 1
	TranslateIndicatorMessage(pIndicator->RequestedProcedureID, tag, OFTrue);
	DcmShortString *reqProcId = new DcmShortString(tag);
	if(pIndicator->RequestedProcedureID >= 0)
	{
	  if(pWorklist->SupportChinese == 1 || IsASCII(pWorklist->RequestedProcedureID))
		cond = reqProcId->putString(pWorklist->RequestedProcedureID);
	  else
		cond = SetDefaultValue(reqProcId, tag.getTagName(), pIndicator->RequestedProcedureID);
	}
	if(cond.bad()) { DumpPosition(tag, CONSTRUCT_FAILED, dataset); return; }
	cond = dataset->insert(reqProcId);
	if(cond.bad()) { DumpPosition(tag, INSERT_FAILED, dataset); return; }
  }

  dbim->DetermineMatchingRecordOne(dataset);
}
