// dcmdynamic.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "dcmdynamic.h"

#include "dcmtk/config/osconfig.h"     /* make sure OS specific configuration is included first */
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"        /* for dcmtk version name */
#include "dcmtk/dcmdata/dcddirif.h"     /* for class DicomDirInterface */
#include "dcmtk/ofstd/ofstd.h"        /* for class OFStandard */
#include "dcmtk/ofstd/ofconapp.h"     /* for class OFConsoleApplication */
#include "dcmtk/ofstd/ofcond.h"       /* for class OFCondition */
#include "dcmtk/dcmdata/dcdebug.h"

#define OFFIS_CONSOLE_APPLICATION "mergedir"
#define OFFIS_CONSOLE_DESCRIPTION "Merge DICOMDIR"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

using namespace std;

static void printAllImageUID(DcmDirectoryRecord *rootRecord)
{
	OFCondition result;
	DcmUniqueIdentifier *ptrInstanceUID = NULL;
	DcmStack resultStack;
	resultStack.push(rootRecord);
	while(rootRecord->search(DCM_ReferencedSOPInstanceUIDInFile, resultStack, ESM_afterStackTop).good())
	{
		DcmObject * object = resultStack.top();
		if(object && (ptrInstanceUID = OFdynamic_cast(DcmUniqueIdentifier*, object)))
		{
			OFString uid;
			result = ptrInstanceUID->getOFString(uid, 0);
			if(result.good())
			{
				COUT << uid << endl;
			}
		}
	}
}

static bool opt_verbose = false;

static OFCondition mergeToDest(DcmDirectoryRecord *dest, DcmDirectoryRecord *src)
{
	DcmTagKey key, subKey;
	E_DirRecType srcType, upperType;
	if(src == NULL || dest == NULL) return EC_IllegalParameter;
	srcType = src->getRecordType();
	switch(srcType)
	{
	case ERT_Patient:
		key = DCM_PatientID;
		subKey = DCM_StudyInstanceUID;
		upperType = ERT_root;
		if(opt_verbose) COUT << "merge patient level ..." << endl;
		break;
	case ERT_Study:
		key = DCM_StudyInstanceUID;
		subKey = DCM_SeriesInstanceUID;
		upperType = ERT_Patient;
		if(opt_verbose) COUT << "merge study level ..." << endl;
		break;
	case ERT_Series:
		key = DCM_SeriesInstanceUID;
		subKey = DCM_ReferencedSOPInstanceUIDInFile;
		upperType = ERT_Study;
		if(opt_verbose) COUT << "merge series level ..." << endl;
		break;
	case ERT_Image:
		key = DCM_ReferencedSOPInstanceUIDInFile;
		subKey = DCM_ReferencedSOPInstanceUIDInFile;
		upperType = ERT_Series;
		if(opt_verbose) COUT << "merge instance level ..." << endl;
		break;
	default:
		CERR << "src's record type is unexpected:" << endl;
		CERR << "src:" << endl;
		src->print(CERR);
		return EC_IllegalParameter;
	}

	if(upperType != dest->getRecordType())
	{
		CERR << "src's and dest's record type are mismatched:" << endl;
		CERR << "dest:" << endl;
		dest->print(CERR);
		CERR << "src:" << endl;
		src->print(CERR);
		return EC_IllegalParameter;
	}

	OFString srcUid, destSubUid;
	DcmElement *elementId = NULL, *destElementId = NULL;
	if(EC_Normal == src->findAndGetElement(key, elementId) && 
		elementId->getOFString(srcUid, 0).good())
	{
		DcmDirectoryRecord *destSub = NULL;
		while(destSub = dest->nextSub(destSub))
		{
			if(destSub->getRecordType() != srcType)
			{
				CERR << "src's and destSub's record type are mismatched:" << endl;
				CERR << "src:" << endl;
				src->print(CERR);
				CERR << "destSub:" << endl;
				destSub->print(CERR);
				continue;
			}

			if(EC_Normal == destSub->findAndGetElement(key, destElementId) && 
				destElementId->getOFString(destSubUid, 0).good())
			{
				if(srcUid == destSubUid)
				{
					if(srcType == ERT_Image)
					{
						CERR << "skip same image " << srcUid << endl;
						return EC_Normal;
					}
					DcmDirectoryRecord *srcSub = NULL;
					while(srcSub = src->nextSub(srcSub))
					{
						if(mergeToDest(destSub, srcSub).bad())
						{
							CERR << "mergeToDest failed:" << endl;
							CERR << "srcSub:" << endl;
							srcSub->print(CERR);
							CERR << "destSub:" << endl;
							destSub->print(CERR);
						}
					}
					return EC_Normal;
				}
				// else continue;
			}
			else
			{
				CERR << "Can not find " << key.toString() << " in destSub:" << endl;
				destSub->print(CERR);
			}
		}

		if(opt_verbose)
		{
			CERR << "insert new record:" << endl;
			src->print(CERR);
		}
		return dest->insertSub(OFdynamic_cast(DcmDirectoryRecord*, src->clone()));
	}
	else
	{
		CERR << "Can not find " << key.toString() << " in src:" << endl;
		src->print(CERR);
	}
	return EC_IllegalParameter;
}

DCMDYNAMIC_API int MergeDicomDir(list<string> &fileNames, const char *opt_output, const char *opt_fileset)
{
	int errCount = 0;
	OFCondition cond;
	DcmDicomDir *dest = new DcmDicomDir(opt_output, opt_fileset);
	if (dest != NULL)
	{
		cond = dest->error();
		if(cond.bad()) ++errCount;
	}
	else
	{
		CERR << "create or open output file " << opt_output << " error" << endl;
		return -3;
	}

	DcmDirectoryRecord *destRoot = &(dest->getRootRecord());

	for(list<string>::iterator curr = fileNames.begin(); curr != fileNames.end(); ++curr)
	{
		DcmDicomDir *src = new DcmDicomDir((*curr).c_str());
		if (src != NULL)
		{
			cond = src->error();
			if(cond.bad()) ++errCount;
		}
		else
		{
			CERR << "open input file " << *curr << " error" << endl;
			++errCount;
			continue;
		}
		if(opt_verbose) COUT << "open input file " << *curr << endl;

		DcmDirectoryRecord *srcRoot = &(src->getRootRecord());
		DcmStack resultStack;

		size_t bufferlen = (*curr).length() + 1;
		char filepathbuffer[512];
		(*curr).copy(filepathbuffer, (*curr).length());
		filepathbuffer[bufferlen - 1] = '\0';
		char *sppos = NULL;
		if(sppos = strrchr(filepathbuffer, '\\'))
			*sppos = '\0';
		else
			filepathbuffer[0] = '\0';
		strcat_s(filepathbuffer, "\\*");		

		_finddata_t fileinfo;
		int nextfound = 0;
		intptr_t searchHandle = _findfirst(filepathbuffer, &fileinfo);
		while(nextfound == 0 && searchHandle != -1)
		{
			if(fileinfo.attrib & _A_SUBDIR)
				COUT << "dir: ";
			else
				COUT << "file: ";
			COUT << fileinfo.name << ", last modify: ";
			struct tm lastmodify;
			localtime_s(&lastmodify, &fileinfo.time_write);
			COUT << lastmodify.tm_year + 1900 << '-' << lastmodify.tm_mon + 1 << '-' << lastmodify.tm_mday << ' '
				<< lastmodify.tm_hour << ':' << lastmodify.tm_min << ':' << lastmodify.tm_sec << endl;
			nextfound = _findnext(searchHandle, &fileinfo);
		}
		if(searchHandle != -1) _findclose(searchHandle);

		// find patientId in patient
		resultStack.clear();
		DcmDirectoryRecord *patient = NULL;
		while(patient = srcRoot->nextSub(patient))
		{
			cond = mergeToDest(destRoot, patient);
			if(cond.bad()) ++errCount;
		}
		delete src;
	}
	cond = dest->write(EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL);
	printAllImageUID(destRoot);
	delete dest;
	if(cond.bad()) ++errCount;
	return errCount;
}
