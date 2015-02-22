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

#define OFFIS_CONSOLE_APPLICATION "dcmdynamic"
#define OFFIS_CONSOLE_DESCRIPTION "DcmDynamic DLL"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

using namespace std;

static void printAllImageUID(DcmDirectoryRecord *rootRecord, ostream &outstrm)
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
				outstrm << uid << endl;
			}
		}
	}
}

static bool opt_verbose = false;

static OFCondition mergeToDest(DcmDirectoryRecord *dest, DcmDirectoryRecord *src, ostream &errlog)
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
		if(opt_verbose) errlog << "merge patient level ..." << endl;
		break;
	case ERT_Study:
		key = DCM_StudyInstanceUID;
		subKey = DCM_SeriesInstanceUID;
		upperType = ERT_Patient;
		if(opt_verbose) errlog << "merge study level ..." << endl;
		break;
	case ERT_Series:
		key = DCM_SeriesInstanceUID;
		subKey = DCM_ReferencedSOPInstanceUIDInFile;
		upperType = ERT_Study;
		if(opt_verbose) errlog << "merge series level ..." << endl;
		break;
	case ERT_Image:
		key = DCM_ReferencedSOPInstanceUIDInFile;
		subKey = DCM_ReferencedSOPInstanceUIDInFile;
		upperType = ERT_Series;
		if(opt_verbose) errlog << "merge instance level ..." << endl;
		break;
	default:
		errlog << "src's record type is unexpected:" << endl;
		errlog << "src:" << endl;
		src->print(errlog);
		return EC_IllegalParameter;
	}

	if(upperType != dest->getRecordType())
	{
		errlog << "src's and dest's record type are mismatched:" << endl;
		errlog << "dest:" << endl;
		dest->print(errlog);
		errlog << "src:" << endl;
		src->print(errlog);
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
				errlog << "src's and destSub's record type are mismatched:" << endl;
				errlog << "src:" << endl;
				src->print(errlog);
				errlog << "destSub:" << endl;
				destSub->print(errlog);
				continue;
			}

			if(EC_Normal == destSub->findAndGetElement(key, destElementId) && 
				destElementId->getOFString(destSubUid, 0).good())
			{
				if(srcUid == destSubUid)
				{
					if(srcType == ERT_Image)
					{
						if(opt_verbose) errlog << "skip same image " << srcUid << endl;
						return EC_Normal;
					}
					DcmDirectoryRecord *srcSub = NULL;
					while(srcSub = src->nextSub(srcSub))
					{
						if(mergeToDest(destSub, srcSub, errlog).bad())
						{
							errlog << "mergeToDest failed:" << endl;
							errlog << "srcSub:" << endl;
							srcSub->print(errlog);
							errlog << "destSub:" << endl;
							destSub->print(errlog);
						}
					}
					return EC_Normal;
				}
				// else continue;
			}
			else
			{
				errlog << "Can not find " << key.toString() << " in destSub:" << endl;
				destSub->print(errlog);
			}
		}

		if(opt_verbose) errlog << "insert new record: " << key.toString() << endl;
		return dest->insertSub(OFdynamic_cast(DcmDirectoryRecord*, src->clone()));
	}
	else
	{
		errlog << "Can not find " << key.toString() << " in src:" << endl;
		src->print(errlog);
	}
	return EC_IllegalParameter;
}

DCMDYNAMIC_API int MergeDicomDir(list<string> &fileNames, const char *opt_output, const char *opt_fileset, ostream &errlog, bool verbose)
{
	opt_verbose = verbose;
	int errCount = 0;
	OFCondition cond;
	remove(opt_output);
	DcmDicomDir *dest = new DcmDicomDir(opt_output, opt_fileset);
	if (dest != NULL)
	{
		cond = dest->error();
		if(cond.bad())
		{
			++errCount;
			errlog << cond.text() << endl;
		}
	}
	else
	{
		errlog << "create or open output file " << opt_output << " error" << endl;
		return -3;
	}

	DcmDirectoryRecord *destRoot = &(dest->getRootRecord());

	for(list<string>::iterator curr = fileNames.begin(); curr != fileNames.end(); ++curr)
	{
		DcmDicomDir *src = new DcmDicomDir((*curr).c_str());
		if (src != NULL)
		{
			cond = src->error();
			if(cond.bad())
			{
				++errCount;
				errlog << cond.text() << endl;
			}
		}
		else
		{
			errlog << "open input file " << *curr << " error" << endl;
			++errCount;
			continue;
		}
		if(opt_verbose) errlog << "open input file " << *curr << endl;

		DcmDirectoryRecord *srcRoot = &(src->getRootRecord());

		// find patientId in patient
		DcmDirectoryRecord *patient = NULL;
		while(patient = srcRoot->nextSub(patient))
		{
			cond = mergeToDest(destRoot, patient, errlog);
			if(cond.bad())
			{
				++errCount;
				errlog << cond.text() << endl;
			}
		}
		delete src;
	}
	if(opt_verbose) errlog << "start writing " << dest->getDirFileName() << endl;
	cond = dest->write(EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL);
	if(opt_verbose) errlog << "write complete" << endl;
	//printAllImageUID(destRoot, errlog);
	delete dest;
	if(cond.bad())
	{
		++errCount;
		errlog << cond.text() << endl;
	}
	return errCount;
}
