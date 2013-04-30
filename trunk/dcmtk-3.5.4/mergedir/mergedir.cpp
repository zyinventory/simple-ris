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

int main(int argc, char *argv[])
{
    const char *opt_output = DEFAULT_DICOMDIR_NAME;

	SetDebugLevel(( 0 ));
	OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
	OFCommandLine cmd;
	cmd.setOptionColumns(LONGCOL, SHORTCOL);
	cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

	cmd.addParam("dicomdir-in", "referenced DICOMDIR file", OFCmdParam::PM_MultiOptional);
	cmd.addGroup("input options:");
	cmd.addOption("--output-file",           "-o", 1,  "[f]ilename : string",
                                                           "generate specific DICOMDIR file\n(default: " DEFAULT_DICOMDIR_NAME " in current directory)");

	/* evaluate command line */
    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::ExpandWildcards))
    {
        /* print help text and exit */
        if (cmd.getArgCount() == 0)
            app.printUsage();
        if (cmd.findOption("--output-file"))
            app.checkValue(cmd.getValue(opt_output));
	}

    OFList<OFString> fileNames;
    const char *param = NULL;
    const int count = cmd.getParamCount();

    if (count == 0) app.printUsage();
	for (int i = 1; i <= count; i++)
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

	OFCondition result;
	DcmDicomDir *DicomDir = new DcmDicomDir(fileNames.front().c_str());
	if (DicomDir != NULL)
		result = DicomDir->error();
	else
		result = EC_MemoryExhausted;
	DcmDirectoryRecord *rootRecord = &(DicomDir->getRootRecord());
	DcmDirectoryRecord *recPatient = rootRecord->nextSub(NULL);
	if(recPatient && recPatient->getRecordType() == ERT_Patient)
	{
		DcmElement *patientId = NULL;
		recPatient->findAndGetElement(DCM_PatientID, patientId);
		if(patientId)
		{
			OFString pid;
			patientId->getOFString(pid, 0);
			COUT << pid << endl;
		}
	}
	delete DicomDir;
}
