// bridgetest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <dcmtk/ofstd/ofdatime.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <bridge.h>

_declspec(dllimport) WorklistRecord WorklistInBridge;

int _tmain(int argc, _TCHAR* argv[])
{
  DcmDataset *datasetPtrArray[ MAX_WLM_NUMBER ];

  Uint32 datasetNumber = 0;

  if(connectDicomDB() /* && GetWorklistFromDB(datasetPtrArray, &datasetNumber) */)
  {
	std::cout << "Test Number: " << WorklistInBridge.AccessionNumber << ',' << WorklistInBridge.PatientsNameCh << endl;
	commitDicomDB();
	return 0;
  }
  else
    return -1;
}
