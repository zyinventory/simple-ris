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

#if defined (HAVE_WINDOWS_H) || defined(HAVE_FNMATCH_H)
#define PATTERN_MATCHING_AVAILABLE
#endif

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define SHORTCOL 4
#define LONGCOL 23

OFBool	opt_verbose = OFFalse;

OFCondition mergeToDest(DcmDirectoryRecord *dest, DcmDirectoryRecord *src);

int main(int argc, char *argv[])
{
    const char *opt_output = DEFAULT_DICOMDIR_NAME;
	const char *opt_fileset = DEFAULT_FILESETID;
    const char *opt_descriptor = NULL;
    const char *opt_charset = DEFAULT_DESCRIPTOR_CHARSET;

	SetDebugLevel(( 0 ));
	OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
	OFCommandLine cmd;
	cmd.setOptionColumns(LONGCOL, SHORTCOL);
	cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

	cmd.addParam("dicomdir-in", "referenced DICOMDIR file", OFCmdParam::PM_MultiOptional);
	cmd.addGroup("input options:");
	cmd.addOption("--verbose",               "-v",     "verbose mode, print processing details");
	cmd.addOption("--output-file",           "+D", 1,  "[f]ilename : string",
                                                           "generate specific DICOMDIR file\n(default: " DEFAULT_DICOMDIR_NAME " in current directory)");
    cmd.addOption("--fileset-id",            "+F", 1,  "[i]d : string (default: " DEFAULT_FILESETID ")",
                                                       "use specific file set ID");
    cmd.addOption("--descriptor",            "+R", 1,  "[f]ilename : string",
                                                       "add a file set descriptor file ID\n(e.g. README, default: no descriptor)");
    cmd.addOption("--char-set",              "+C", 1,  "[c]har-set : string",
                                                           "add a specific character set for descriptor\n(default: \"" DEFAULT_DESCRIPTOR_CHARSET "\" if descriptor present)");

	/* evaluate command line */
    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::ExpandWildcards))
    {
		if (cmd.findOption("--verbose")) opt_verbose=OFTrue;
        /* print help text and exit */
        if (cmd.getArgCount() == 0)
            app.printUsage();
        if (cmd.findOption("--output-file"))
            app.checkValue(cmd.getValue(opt_output));
        if (cmd.findOption("--fileset-id"))
            app.checkValue(cmd.getValue(opt_fileset));
        if (cmd.findOption("--descriptor"))
            app.checkValue(cmd.getValue(opt_descriptor));
        if (cmd.findOption("--char-set"))
            app.checkValue(cmd.getValue(opt_charset));
	}

    OFList<OFString> fileNames;
    const char *param = NULL;

	const int paramCount = cmd.getParamCount();
    if (paramCount == 0) app.printUsage();
	for (int i = 1; i <= paramCount; i++)
	{
		cmd.getParam(i, param);
		fileNames.push_back(param);
	}

	/* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFOStringStream oss;
        oss << "no data dictionary loaded, check environment variable: "
            << DCM_DICT_ENVIRONMENT_VARIABLE << OFStringStream_ends;
        OFSTRINGSTREAM_GETSTR(oss, tmpString)
        app.printError(tmpString);  /* calls exit(1) */
        OFSTRINGSTREAM_FREESTR(tmpString)
        return 1;  /* DcmDicomDir class dumps core when no data dictionary */
    }

	OFCondition cond;
	DcmDicomDir *dest = new DcmDicomDir(opt_output, opt_fileset);
	if (dest != NULL)
		cond = dest->error();
	else
		{ CERR << "create or open output file " << opt_output << " error" << endl; exit(1); }
	DcmDirectoryRecord *destRoot = &(dest->getRootRecord());

	for(OFListIterator(OFString) curr = fileNames.begin(); curr != fileNames.end(); ++curr)
	{
		DcmDicomDir *src = new DcmDicomDir((*curr).c_str());
		if (src != NULL)
			cond = src->error();
		else
			{ CERR << "open input file " << *curr << " error" << endl; continue; }
		COUT << "open input file " << *curr << endl;
		DcmDirectoryRecord *srcRoot = &(src->getRootRecord());
		DcmStack resultStack;

		// find all image
		/*
		DcmUniqueIdentifier *ptrInstanceUID = NULL;
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
		*/
		// find patientId in patient
		resultStack.clear();
		DcmDirectoryRecord *patient = NULL;
		while(patient = srcRoot->nextSub(patient))
		{
			cond = mergeToDest(destRoot, patient);
		}
		delete src;
	}
	dest->write(EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL);
	delete dest;
}

OFCondition mergeToDest(DcmDirectoryRecord *dest, DcmDirectoryRecord *src)
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
		if(opt_verbose) CERR << "merge patient level ..." << endl;
		break;
	case ERT_Study:
		key = DCM_StudyInstanceUID;
		subKey = DCM_SeriesInstanceUID;
		upperType = ERT_Patient;
		if(opt_verbose) CERR << "merge study level ..." << endl;
		break;
	case ERT_Series:
		key = DCM_SeriesInstanceUID;
		subKey = DCM_ReferencedSOPInstanceUIDInFile;
		upperType = ERT_Study;
		if(opt_verbose) CERR << "merge series level ..." << endl;
		break;
	case ERT_Image:
		key = DCM_ReferencedSOPInstanceUIDInFile;
		subKey = DCM_ReferencedSOPInstanceUIDInFile;
		upperType = ERT_Series;
		if(opt_verbose) CERR << "merge instance level ..." << endl;
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