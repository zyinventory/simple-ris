#pragma once

#include "stdafx.h"

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

bool connectDicomDB();
bool commitDicomDB();
bool rollbackDicomDB();
bool insertImageInfoToDB(PImgDataset);
bool getManageNumber(char * const outImageManageNum, const char * const studyUid, int currentStudyDateNumber);
