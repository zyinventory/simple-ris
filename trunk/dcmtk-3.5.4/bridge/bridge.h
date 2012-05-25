#pragma once

#include <dcmtk/dcmnet/dicom.h>
#include "common.h"

#define __BRIDGE_DEFINE 1

#define DIC_AS_LEN		4
#define DIC_DA_LEN		8
#define DIC_TM_LEN		16
typedef char DIC_AS[DIC_AS_LEN + 1];
typedef char DIC_DA[DIC_DA_LEN + 1];
typedef char DIC_TM[DIC_TM_LEN + 1];

bool commitDicomDB();
bool rollbackDicomDB();

// ------------- Store SCP -----------------

typedef struct tagImgDataset
{
  const char *pHddRoot;
  const char *pPath;
  const char *pImgManageNum;
  int fileDate, fileTime, insertDate, insertTime;
  LARGE_INTEGER fileSize;
  const char *pTransferSyntaxUid;
  const char *pSopClaUid;
  const char *pSopInsUid;
  int stuDat;
  int serDat;
  int stuTim;
  int serTim;
  const char *pAccNum;
  const char *pSeriesModality;
  const char *pStudyModality;
  const char *pManufacturer;
  const char *pInstNam;
  const char *pRefPhyNam;
  const char *pStationName;
  const char *pStuDes;
  const char *pSerDes;
  const char *pInstDepartName;
  const char *pPhyRecNam;
  const char *pPerformPhyName;
  const char *pPhyReadStuName;
  const char *pOperateName;
  const char *pManufactModNam;
  const char *pPatNam;
  const char *pPatNamKan;
  const char *pPatNamKat;
  const char *pPatId;
  int patBirDat;
  const char *pPatSex;
  const char *pPatAge;
  const char *pPatSiz;
  const char *pPatWei;
  const char *pContBolus;
  const char *pBodParExam;
  const char *pProtocolNam;
  const char *pPatPos;
  const char *pViewPos;
  const char *pStuInsUid;
  const char *pSerInsUid;
  const char *pStuId;
  const char *pSerNum;
  const char *pImaNum;
  const char *pReqPhysician;
  const char *pReqService;
} ImgDataset, *PImgDataset;

bool insertImageInfoToDB(PImgDataset);
bool getManageNumber(char * const outImageManageNum, const char * const studyUid, int currentStudyDateNumber, ostream* pLogStream = NULL);

// ------------- WLM Condition -----------------

#define MAX_WLM_NUMBER 100

typedef struct tagWlmCondition
{
  const char *pScheduleStationAE;
  const char *pModality;
  const char *pLowerScheduleDate;
  const char *pUpperScheduleDate;
} WlmCondition, *PWlmCondition;	

typedef struct tagWorklistRecord
{
  DIC_AE ScheduledStationAETitle;
  DIC_DA SchdldProcStepStartDate;
  DIC_TM SchdldProcStepStartTime;
  DIC_CS Modality;
  DIC_LO SchdldProcStepDescription;
  DIC_SH SchdldProcStepLocation;
  DIC_SH SchdldProcStepID;
  DIC_SH RequestedProcedureID;
  DIC_LO RequestedProcedureDescription;
  DIC_UI StudyInstanceUID;
  DIC_SH AccessionNumber;
  DIC_PN RequestingPhysician;
  DIC_LO AdmissionID;
  DIC_PN PatientsNameEn;
  DIC_PN PatientsNameCh;
  DIC_LO PatientID;
  DIC_DA PatientsBirthDate;
  DIC_CS PatientsSex;
  DIC_DS PatientsWeight;
  DIC_LO AdmittingDiagnosesDescription;
  DIC_AS PatientsAge;
  int SupportChinese;
  int DicomPersonName;
} WorklistRecord, *PWorklistRecord;

typedef struct tagIndicatorWorklistRecord
{
  short ScheduledStationAETitle;
  short SchdldProcStepStartDate;
  short SchdldProcStepStartTime;
  short Modality;
  short SchdldProcStepDescription;
  short SchdldProcStepLocation;
  short SchdldProcStepID;
  short RequestedProcedureID;
  short RequestedProcedureDescription;
  short StudyInstanceUID;
  short AccessionNumber;
  short RequestingPhysician;
  short AdmissionID;
  short PatientsNameEn;
  short PatientsNameCh;
  short PatientID;
  short PatientsBirthDate;
  short PatientsSex;
  short PatientsWeight;
  short AdmittingDiagnosesDescription;
  short PatientsAge;
  short SupportChinese;
  short DicomPersonName;
} IndicatorWorklistRecord, *PIndicatorWorklistRecord;

class WlmDBInteractionManager;
typedef void (*FetchWorklistCallback)(PWorklistRecord pWorklist, PIndicatorWorklistRecord pIndicator, WlmDBInteractionManager *dbim);
bool GetWorklistFromDB(FetchWorklistCallback /* fetchCallBack */, WlmDBInteractionManager *dbim);
