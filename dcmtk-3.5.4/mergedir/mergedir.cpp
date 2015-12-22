#include <list>
#include <string>
#include <io.h>
#include <string.h>
#include <fcntl.h>  //_O_BINARY

#include "dcmtk/config/osconfig.h"     /* make sure OS specific configuration is included first */
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"        /* for dcmtk version name */
#include "dcmtk/ofstd/ofstd.h"        /* for class OFStandard */
#include "dcmtk/ofstd/ofconapp.h"     /* for class OFConsoleApplication */
#include "dcmtk/ofstd/ofcond.h"       /* for class OFCondition */
#include "dcmdynamic.h"

void call_process_log(string &sessionId);

#define OFFIS_CONSOLE_APPLICATION "mergedir"
#define OFFIS_CONSOLE_DESCRIPTION "Merge DICOMDIR"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define SHORTCOL 4
#define LONGCOL 23

OFBool	opt_verbose = OFFalse;

int main(int argc, char *argv[])
{
	const char *opt_output = DEFAULT_DICOMDIR_NAME;
	const char *opt_fileset = "DCMTK_MEDIA_DEMO";
    const char *opt_descriptor = NULL;
    const char *opt_charset = "ISO_IR 100";

	_setmode( _fileno( stdout ), _O_BINARY );

	OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
	OFCommandLine cmd;

	cmd.setOptionColumns(LONGCOL, SHORTCOL);
	cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

	cmd.addParam("dicomdir-in", "referenced DICOMDIR file", OFCmdParam::PM_MultiOptional);
	cmd.addGroup("input options:");
	cmd.addOption("--verbose",               "-v",     "verbose mode, print processing details");
	cmd.addOption("--output-file",           "+D", 1,  "[f]ilename : string",
                                                           "generate specific DICOMDIR file\n(default: " DEFAULT_DICOMDIR_NAME " in current directory)");
    cmd.addOption("--fileset-id",            "+F", 1,  "[i]d : string (default: DCMTK_MEDIA_DEMO)",
                                                       "use specific file set ID");
    cmd.addOption("--descriptor",            "+R", 1,  "[f]ilename : string",
                                                       "add a file set descriptor file ID\n(e.g. README, default: no descriptor)");
    cmd.addOption("--char-set",              "+C", 1,  "[c]har-set : string",
                                                           "add a specific character set for descriptor\n(default: \"ISO_IR 100\" if descriptor present)");

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
	else
		return -1;

    std::list<std::string> fileNames;
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
        return -2;  /* DcmDicomDir class dumps core when no data dictionary */
    }

    for_each(fileNames.begin(), fileNames.end(), call_process_log);

    return 0;
	//return MergeDicomDir(fileNames, opt_output, opt_fileset, CERR, opt_verbose);
	//return DicomDir2Xml(fileNames.front().c_str(), opt_output) ? 0 : -1;
}
