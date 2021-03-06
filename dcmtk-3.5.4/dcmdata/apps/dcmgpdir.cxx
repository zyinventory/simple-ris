/*
*
*  Copyright (C) 1994-2005, OFFIS
*
*  This software and supporting documentation were developed by
*
*    Kuratorium OFFIS e.V.
*    Healthcare Information and Communication Systems
*    Escherweg 2
*    D-26121 Oldenburg, Germany
*
*  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
*  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
*  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
*  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
*  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
*
*  Module:  dcmdata
*
*  Author:  Andrew Hewett, Joerg Riesmeier
*
*  Purpose:
*  Make a DICOMDIR according to the DICOM Part 11 Media Storage Application
*  Profiles. Supports the following profiles:
*  - General Purpose CD-R Interchange (STD-GEN-CD)
*  - General Purpose Interchange on DVD-RAM Media (STD-GEN-DVD-RAM)
*  If build with 'BUILD_DCMGPDIR_AS_DCMMKDIR' it also supports:
*  - General Purpose DVD with Compression Interchange (STD-GEN-DVD-JPEG/J2K)
*  - General Purpose USB and Flash Memory with Compression Interchange (STD-GEN-USB/MMC/CF/SD-JPEG/J2K)
*  - General Purpose MIME Interchange (STD-GEN-MIME)
*  - DVD Interchange with MPEG2 MP@ML (STD-DVD-MPEG2-MPML)
*  - Basic Cardiac X-Ray Angiographic Studies on CD-R Media (STD-XABC-CD)
*  - 1024 X-Ray Angiographic Studies on CD-R Media (STD-XA1K-CD)
*  - 1024 X-Ray Angiographic Studies on DVD Media (STD-XA1K-DVD)
*  - Dental Radiograph Interchange (STD-DEN-CD)
*  - CT/MR Studies on various Media (STD-CTMR-xxxx)
*  - Ultrasound Single Frame for Image Display (STD-US-ID-SF-xxxx)
*  - Ultrasound Single Frame with Spatial Calibration (STD-US-SC-SF-xxxx)
*  - Ultrasound Single Frame with Combined Calibration (STD-US-CC-SF-xxxx)
*  - Ultrasound Single & Multi-Frame for Image Display (STD-US-ID-MF-xxxx)
*  - Ultrasound Single & Multi-Frame with Spatial Calibration (STD-US-SC-MF-xxxx)
*  - Ultrasound Single & Multi-Frame with Combined Calibration (STD-US-CC-MF-xxxx)
*  - 12-lead ECG Interchange on Diskette (STD-WVFM-ECG-FD)
*  - Hemodynamic Waveform Interchange on Diskette (STD-WVFM-HD-FD)
*  There should be no need to set this compiler flag manually, just compile
*  dcmjpeg/apps/dcmmkdir.cc.
*
*  Last Update:      $Author: meichel $
*  Update Date:      $Date: 2005/12/08 15:40:48 $
*  CVS/RCS Revision: $Revision: 1.81 $
*  Status:           $State: Exp $
*
*  CVS/RCS Log at end of file
*
*/


#include "dcmtk/config/osconfig.h"     /* make sure OS specific configuration is included first */

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"        /* for dcmtk version name */
#include "dcmtk/dcmdata/dcddirif.h"     /* for class DicomDirInterface */
#include "dcmtk/ofstd/ofstd.h"        /* for class OFStandard */
#include "dcmtk/ofstd/ofconapp.h"     /* for class OFConsoleApplication */
#include "dcmtk/ofstd/ofcond.h"       /* for class OFCondition */
#include "dcmtk/dcmdata/dcdebug.h"

#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
#include "dcmtk/dcmimage/diregist.h"     /* include to support color images */
#include "dcmtk/dcmdata/dcrledrg.h"     /* for DcmRLEDecoderRegistration */
#include "dcmtk/dcmjpeg/djdecode.h"     /* for dcmjpeg decoders */
#include "dcmtk/dcmjpeg/dipijpeg.h"     /* for dcmimage JPEG plugin */
#include "dcmtk/dcmjpeg/ddpiimpl.h"     /* for class DicomDirImageImplementation */
#endif

#include "dcmtk/ofstd/oftimer.h"        /* for windows.h */
#include <set>
#include <numeric>
#include <process.h>
#include "commonlib.h"
#include "dcmtk/dcmdata/xml_index.h"

using namespace handle_context;

int opt_verbose = 0;

#ifdef WITH_ZLIB
#include <zlib.h>         /* for zlibVersion() */
#endif

#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
#define OFFIS_CONSOLE_APPLICATION "dcmmkdir"
#define OFFIS_CONSOLE_DESCRIPTION "Create a DICOMDIR file"
#else
#define OFFIS_CONSOLE_APPLICATION "dcmgpdir"
#define OFFIS_CONSOLE_DESCRIPTION "Create a general purpose DICOMDIR"
#endif

#if defined (HAVE_WINDOWS_H) || defined(HAVE_FNMATCH_H)
#define PATTERN_MATCHING_AVAILABLE
#endif

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

static char timeBuffer[32], fnbuf[4096], last_file_name[MAX_PATH] = "";
//GE CT Impl Class UID
static OFString GE_ImplementationClassUID("1.2.840.113619.6.286"), GE_MediaStorageSOPInstanceUID("1.2.840.113619.6.286.%Y%m%d.%H%M%S.");

#define SHORTCOL 4
#define LONGCOL 23

// ********************************************

static bool GEMediaStorageSOPInstanceUID(char *buf, uint buf_len)
{
	time_t now = time(NULL);
	struct tm calendar;
	errno_t err = localtime_s(&calendar, &now);
	if(!err)
	{
        size_t pathLen = strftime(buf, buf_len, GE_MediaStorageSOPInstanceUID.c_str(), &calendar);
		if( ! pathLen ) return false;
		sprintf_s(buf + pathLen, buf_len - pathLen, "%d", _getpid());
		return true;
	}
	return false;
}

static void checkValueGE(DcmMetaInfo *metainfo, const DcmTagKey &atagkey, const E_TransferSyntax oxfer)
{
	DcmStack stack;
    DcmTag tag(atagkey);

    DcmTagKey xtag = tag.getXTag();
    DcmElement *elem = NULL;

	if (xtag == DCM_MediaStorageSOPInstanceUID)    // (0002,0003)
    {
        elem = new DcmUniqueIdentifier(tag);
        metainfo->insert(elem, OFTrue);
		char buf[65];
		GEMediaStorageSOPInstanceUID(buf, sizeof(buf));
		OFstatic_cast(DcmUniqueIdentifier *, elem)->putString(buf);
        DCM_dcmdataDebug(2, ("DcmFileFormat::checkValue() use new generated SOPInstanceUID [%s]", buf));
    }
	else if (xtag == DCM_ImplementationClassUID)        // (0002,0012)
    {
        elem = new DcmUniqueIdentifier(tag);
        metainfo->insert(elem, OFTrue);
        OFstatic_cast(DcmUniqueIdentifier *, elem)->putString(GE_ImplementationClassUID.c_str());
    }
	else if (xtag == DCM_ImplementationVersionName)     // (0002,0013)
    {
        elem = new DcmShortString(tag);
        metainfo->insert(elem, OFTrue);
        const char uid[] = "AW4_6_05_003_SLE";
        OFstatic_cast(DcmShortString *, elem)->putString(uid);
    }
	else if (xtag == DCM_SourceApplicationEntityTitle)     // (0002,0016)
    {
        elem = new DcmApplicationEntity(tag);
        metainfo->insert(elem, OFTrue);
        const char uid[] = "EK-Series";
		elem->putString(uid);
    }
}

static size_t normalize_charsets(const char *charset, set<string> &charsets)
{
    char *ctx = NULL, *tokens = NULL, *token = NULL;
    size_t charset_len = strlen(charset);
    tokens = new char[charset_len + 1];
    strcpy_s(tokens, charset_len + 1, charset);
    token = strtok_s(tokens, "\\", &ctx);
    while(token != NULL)
    {
        if(strcmp(token, CHARSET_UTF8) == 0 || strcmp(token, CHARSET_UTF8_ALIAS) == 0)
            charsets.insert(charsets.end(), CHARSET_UTF8);
        else if(strcmp(token, CHARSET_ISO_IR_100) == 0 || strcmp(token, CHARSET_ISO_IR_100_ALIAS) == 0)
            charsets.insert(charsets.end(), CHARSET_ISO_IR_100);
        else if(strcmp(token, CHARSET_ISO_IR_87) == 0  || strcmp(token, CHARSET_ISO_IR_87_ALIAS) == 0)
            charsets.insert(charsets.end(), CHARSET_ISO_IR_87);
        else if(strcmp(token, CHARSET_ISO_IR_159) == 0 || strcmp(token, CHARSET_ISO_IR_159_ALIAS) == 0)
            charsets.insert(charsets.end(), CHARSET_ISO_IR_159);
        else if(strcmp(token, CHARSET_ISO_IR_165) == 0 || strcmp(token, CHARSET_ISO_IR_165_ALIAS) == 0)
            charsets.insert(charsets.end(), CHARSET_ISO_IR_165);
        else if(strcmp(token, CHARSET_ISO_IR_149) == 0 || strcmp(token, CHARSET_ISO_IR_149_ALIAS) == 0)
            charsets.insert(charsets.end(), CHARSET_ISO_IR_149);
        else // CHARSET_GB18030 and others
            charsets.insert(charsets.end(), token);

        token = strtok_s(NULL, "\\", &ctx);
    }
    if(tokens) delete tokens; tokens = NULL;
    return charsets.size();
}

static NOTIFY_FILE_CONTEXT nfc;
static char association_buff[1024];

static void fallback_b32_patientsname(const char *patientsName)
{
    size_t b32len = strlen(patientsName) + 5;
    char *buff = new char[b32len];
    strcpy_s(buff, b32len, "E32:");
    EncodeBase32(patientsName, buff + 4, b32len - 4);
    strncpy_s(nfc.patient.patientsName, buff, sizeof(nfc.patient.patientsName) - 1);
    if(buff) delete buff;
}

static void fill_notify_from_dcmdataset(DcmDataset *dataset)
{
    istringstream ia(association_buff);
    ia  >> nfc.assoc.path >> nfc.file.filename >> nfc.file.xfer 
        >> nfc.assoc.id >> nfc.assoc.store_assoc_id >> nfc.assoc.callingAE >> nfc.assoc.callingAddr >> nfc.assoc.calledAE 
        >> dec >> nfc.assoc.port >> nfc.assoc.expected_xfer >> nfc.assoc.auto_publish >> nfc.assoc.calledAddr;
    
    const char *patientID = NULL, *patientsName = NULL, *patientsBirthDate = NULL, 
        *patientsSex = NULL, *patientsSize = NULL, *patientsWeight = NULL, *sopClassUID = NULL,
        *studyUID = NULL, *studyDate = NULL, *studyTime = NULL, *accessionNumber = NULL, *studyID = NULL,
        *seriesUID = NULL, *modality = NULL, *instanceUID = NULL, *charset = NULL;
    set<OFString> charsets;
    // file level
    dataset->findAndGetString(DCM_SpecificCharacterSet, charset);
    if(charset) normalize_charsets(charset, charsets);
    string charset_fixed;
    charset_fixed = accumulate(charsets.cbegin(), charsets.cend(), charset_fixed,
        [](string &init, const string &val) -> string& {
            if(init.length()) init.append(1, '\\');
            return init.append(val);
        });
    if(charset_fixed.length() == 0) charset_fixed = CHARSET_ISO_IR_100;

	dataset->findAndGetString(DCM_PatientID, patientID);
    if(patientID)
    {
        strcpy_s(nfc.file.patientID, patientID);
        strcpy_s(nfc.patient.patientID, patientID);
    }
    dataset->findAndGetString(DCM_StudyInstanceUID, studyUID);
    if(studyUID)
    {
        strcpy_s(nfc.file.studyUID, studyUID);
        strcpy_s(nfc.study.studyUID, studyUID);
    }
    dataset->findAndGetString(DCM_SeriesInstanceUID, seriesUID);
    if(seriesUID)
    {
        strcpy_s(nfc.file.seriesUID, seriesUID);
        strcpy_s(nfc.series.seriesUID, seriesUID);
    }
    dataset->findAndGetString(DCM_SOPInstanceUID, instanceUID);
    if(instanceUID) strcpy_s(nfc.file.instanceUID, instanceUID);

    dataset->findAndGetString(DCM_SOPClassUID, sopClassUID);
    if(sopClassUID) strcpy_s(nfc.file.sopClassUID, sopClassUID);

    dataset->findAndGetSint32(DCM_InstanceNumber, nfc.file.number);

    DcmXfer xfer(dataset->getOriginalXfer());
    strcpy_s(nfc.file.xfer_new, xfer.getXferShortName());
    nfc.file.isEncapsulated = xfer.isEncapsulated();
    
    //patient level
    dataset->findAndGetString(DCM_PatientsName, patientsName);
    if(patientsName)
    {
        if(charsets.count(CHARSET_UTF8) > 0)
        {
            strcpy_s(nfc.file.charset, CHARSET_UTF8);
            if(0 == UTF8ToGBK(patientsName, nfc.patient.patientsName, sizeof(nfc.patient.patientsName)))
                fallback_b32_patientsname(patientsName);
        }
        else if(charsets.count(CHARSET_ISO_IR_165) > 0 || charsets.count(CHARSET_ISO_IR_87) > 0 || charsets.count(CHARSET_ISO_IR_149) > 0)
        {   //CJK
            strncpy_s(nfc.file.charset, charset_fixed.c_str(), sizeof(nfc.file.charset) - 1);
            size_t gbklen = strlen(patientsName) + 1;
            char *gbkbuff = new char[gbklen];
            if(0 == AutoCharToGBK(gbkbuff, gbklen, patientsName))
                fallback_b32_patientsname(patientsName);
            else
                strncpy_s(nfc.patient.patientsName, gbkbuff, sizeof(nfc.patient.patientsName) - 1);
            if(gbkbuff) delete gbkbuff;
        }
        else//GB18030 or ISO-IR-100
        {
            strncpy_s(nfc.patient.patientsName, patientsName, sizeof(nfc.patient.patientsName) - 1);
            strncpy_s(nfc.file.charset, charset_fixed.c_str(), sizeof(nfc.file.charset) - 1);
        }
    }
    else
    {
        strcpy_s(nfc.patient.patientsName, "(NULL)");
        strncpy_s(nfc.file.charset, charset_fixed.c_str(), sizeof(nfc.file.charset) - 1);
    }
    ValidateGBK(reinterpret_cast<unsigned char*>(nfc.patient.patientsName), sizeof(nfc.patient.patientsName));

    dataset->findAndGetString(DCM_PatientsBirthDate, patientsBirthDate);
    if(patientsBirthDate) normalize_dicom_date(sizeof(nfc.patient.birthday), nfc.patient.birthday, patientsBirthDate);

    dataset->findAndGetString(DCM_PatientsSex, patientsSex);
    if(patientsSex) strcpy_s(nfc.patient.sex, patientsSex);

    dataset->findAndGetString(DCM_PatientsSize, patientsSize);
    if(patientsSize) strcpy_s(nfc.patient.height, patientsSize);

    dataset->findAndGetString(DCM_PatientsWeight, patientsWeight);
    if(patientsWeight) strcpy_s(nfc.patient.weight, patientsWeight);

    //study level
    dataset->findAndGetString(DCM_StudyDate, studyDate);
    if(studyDate) normalize_dicom_date(sizeof(nfc.study.studyDate), nfc.study.studyDate, studyDate);

    dataset->findAndGetString(DCM_StudyTime, studyTime);
    if(studyTime) strcpy_s(nfc.study.studyTime, studyTime);

    dataset->findAndGetString(DCM_StudyID, studyID);
    if(studyID) strcpy_s(nfc.study.studyID, studyID);

    dataset->findAndGetString(DCM_AccessionNumber, accessionNumber);
    if(accessionNumber) strcpy_s(nfc.study.accessionNumber, accessionNumber);

    //series level
    dataset->findAndGetString(DCM_Modality, modality);
    if(modality) strcpy_s(nfc.series.modality, modality);
    dataset->findAndGetSint32(DCM_SeriesNumber, nfc.series.number);

    NotifyFileContextStorePath(&nfc.file);
}

int main(int argc, char *argv[])
{
	int opt_debug = 0;
	OFBool opt_write = OFTrue;
	OFBool opt_append = OFFalse;
	OFBool opt_recurse = OFFalse;
	E_EncodingType opt_enctype = EET_ExplicitLength;
	E_GrpLenEncoding opt_glenc = EGL_withoutGL;
	const char *opt_output = DEFAULT_DICOMDIR_NAME;
	const char *opt_fileset = DEFAULT_FILESETID;
	const char *opt_descriptor = NULL;
	const char *opt_charset = DEFAULT_DESCRIPTOR_CHARSET;
	const char *opt_directory = NULL;
	const char *opt_pattern = NULL;
    const char *opt_pipename = NULL;
	DicomDirInterface::E_ApplicationProfile opt_profile = DicomDirInterface::AP_GeneralPurpose;

	//if( ! SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN) ) displayErrorToCerr("SetPriorityClass");

#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
	// register global decompression codecs (no verbose/debug mode set)
	DcmRLEDecoderRegistration::registerCodecs();
	DJDecoderRegistration::registerCodecs();
#endif

	/* DICOMDIR interface (checks for JPEG/RLE availability) */
	DicomDirInterface ddir;

	SetDebugLevel(( 0 ));

	/* set-up command line parameters and options */
	OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
	OFCommandLine cmd;
	cmd.setOptionColumns(LONGCOL, SHORTCOL);
	cmd.setParamColumn(LONGCOL + SHORTCOL + 4);

	cmd.addParam("dcmfile-in", "referenced DICOM file", OFCmdParam::PM_MultiOptional);

	cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
	cmd.addOption("--help",                     "-h",     "print this help text and exit");
	cmd.addOption("--version",                            "print version information and exit", OFTrue /* exclusive */);
	cmd.addOption("--verbose",                  "-v",     "verbose mode, print processing details");
	cmd.addOption("--quiet",                    "-q",     "quiet mode, print no warnings and errors");
	cmd.addOption("--debug",                    "-d",     "debug mode, print debug information");

	cmd.addGroup("input options:");
	cmd.addSubGroup("DICOMDIR identifiers:");
	cmd.addOption("--output-file",           "+D", 1,  "[f]ilename : string",
		"generate specific DICOMDIR file\n(default: " DEFAULT_DICOMDIR_NAME " in current directory)");
	cmd.addOption("--fileset-id",            "+F", 1,  "[i]d : string (default: " DEFAULT_FILESETID ")",
		"use specific file set ID");
	cmd.addOption("--descriptor",            "+R", 1,  "[f]ilename : string",
		"add a file set descriptor file ID\n(e.g. README, default: no descriptor)");
	cmd.addOption("--char-set",              "+C", 1,  "[c]har-set : string",
		"add a specific character set for descriptor\n(default: \"" DEFAULT_DESCRIPTOR_CHARSET "\" if descriptor present)");
	cmd.addSubGroup("type 1 attributes:");
	cmd.addOption("--strict",                "-I",     "exit with error if DICOMDIR type 1 attributes\nare missing in DICOM file (default)");
	cmd.addOption("--invent",                "+I",     "invent DICOMDIR type 1 attributes\nif missing in DICOM file");
	cmd.addSubGroup("reading:");
	cmd.addOption("--input-directory",       "+id", 1, "[d]irectory : string",
		"read referenced DICOM files from directory d\n(default for --recurse: current directory)");
	cmd.addOption("--keep-filenames",        "-m",     "expect filenames to be in DICOM format (def.)");
	cmd.addOption("--map-filenames",         "+m",     "map to DICOM filenames (lowercase->uppercase,\nand remove trailing period)");
	cmd.addOption("--no-recurse",            "-r",     "do not recurse within directories (default)");
	cmd.addOption("--recurse",               "+r",     "recurse within filesystem directories");
#ifdef PATTERN_MATCHING_AVAILABLE
	cmd.addOption("--pattern",               "+p", 1,  "[p]attern : string (only with --recurse)",
		"pattern for filename matching (wildcards)");
#endif
	cmd.addSubGroup("checking:");
	cmd.addOption("--no-consistency-check",  "-W",     "do not check files for consistency");
	cmd.addOption("--warn-inconsist-files",  "+W",     "warn about inconsistent files (default)");
	cmd.addOption("--abort-inconsist-file",  "-a",     "abort on first inconsistent file");
	cmd.addOption("--invent-patient-id",     "+Ipi",   "invent new PatientID in case of inconsistent\nPatientsName attributes");
#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
	cmd.addOption("--no-encoding-check",     "-Nec",   "do not reject images with non-standard\npixel encoding (just warn)");
	cmd.addOption("--no-resolution-check",   "-Nrc",   "do not reject images with non-standard\nspatial resolution (just warn)");
	cmd.addOption("--no-xfer-check",         "-Nxc",   "do not reject images with non-standard\ntransfer syntax (just warn)");
	cmd.addSubGroup("icon images:");
	cmd.addOption("--add-icon-image",        "+X",     "add monochrome icon image on IMAGE level\n(default for cardiac profiles)");
	cmd.addOption("--icon-image-size",       "-Xs", 1, "[s]ize : integer (1..128)",
		"width and height of the icon image (in pixel)\n(fixed: 128 for XA, 64 for CT/MR profile)");
	cmd.addOption("--icon-file-prefix",      "-Xi", 1, "[p]refix : string",
		"use PGM image 'prefix'+'dcmfile-in' as icon\n(default: create icon from DICOM image)");
	cmd.addOption("--default-icon",          "-Xd", 1, "[f]ilename : string",
		"use specified PGM image if icon cannot be\ncreated automatically (default: black image)");
#endif
	cmd.addGroup("output options:");
#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
	cmd.addSubGroup("profiles:");
	cmd.addOption("--general-purpose",       "-Pgp",   "General Purpose Interchange on CD-R or\nDVD-RAM Media (STD-GEN-CD/DVD-RAM, default)");
	cmd.addOption("--general-purpose-dvd",   "-Pdv",   "General Purpose DVD with Compression\nInterchange (STD-GEN-DVD-JPEG/J2K)");
	cmd.addOption("--general-purpose-mime",  "-Pmi",   "General Purpose MIME Interchange\n(STD-GEN-MIME)");
	cmd.addOption("--usb-and-flash",         "-Pfl",   "General Purpose USB/Flash Memory with Compr.\nInterchange (STD-GEN-USB/MMC/CF/SD-JPEG/J2K)");
	cmd.addOption("--mpeg2-mpml-dvd",        "-Pmp",   "DVD Interchange with MPEG2 Main Profile @\nMain Level (STD-DVD-MPEG2-MPML)");
	cmd.addOption("--basic-cardiac",         "-Pbc",   "Basic Cardiac X-Ray Angiographic Studies on\nCD-R Media (STD-XABC-CD)");
	cmd.addOption("--xray-angiographic",     "-Pxa",   "1024 X-Ray Angiographic Studies on CD-R Media\n(STD-XA1K-CD)");
	cmd.addOption("--xray-angiographic-dvd", "-Pxd",   "1024 X-Ray Angiographic Studies on DVD Media\n(STD-XA1K-DVD)");
	cmd.addOption("--dental-radiograph",     "-Pde",   "Dental Radiograph Interchange (STD-DEN-CD)");
	cmd.addOption("--ct-and-mr",             "-Pcm",   "CT/MR Studies (STD-CTMR-xxxx)");
	cmd.addOption("--ultrasound-id-sf",      "-Pus",   "Ultrasound Single Frame for Image Display\n(STD-US-ID-SF-xxxx)");
	cmd.addOption("--ultrasound-sc-sf",                "Ultrasound Single Frame with Spatial\nCalibration (STD-US-SC-SF-xxxx)");
	cmd.addOption("--ultrasound-cc-sf",                "Ultrasound Single Frame with Combined\nCalibration (STD-US-CC-SF-xxxx)");
	cmd.addOption("--ultrasound-id-mf",      "-Pum",   "Ultrasound Single & Multi-Frame for Image\nDisplay (STD-US-ID-MF-xxxx)");
	cmd.addOption("--ultrasound-sc-mf",                "Ultrasound Single & Multi-Frame with Spatial\nCalibration (STD-UD-SC-MF-xxxx)");
	cmd.addOption("--ultrasound-cc-mf",                "Ultrasound Single & Multi-Frame with Combined\nCalibration (STD-UD-CC-MF-xxxx)");
	cmd.addOption("--12-lead-ecg",           "-Pec",   "12-lead ECG Interchange on Diskette\n(STD-WVFM-ECG-FD)");
	cmd.addOption("--hemodynamic-waveform",  "-Phd",   "Hemodynamic Waveform Interchange on Diskette\n(STD-WVFM-HD-FD)");
#endif
	cmd.addSubGroup("writing:");
	cmd.addOption("--replace",               "-A",     "replace existing DICOMDIR (default)");
	cmd.addOption("--append",                "+A",     "append to existing DICOMDIR, if doesn't exist DICOMDIR, create new DICOMDIR");
	cmd.addOption("--discard",               "-w",     "do not write out DICOMDIR");
	cmd.addOption("--no-backup",             "-nb",    "do not create a backup of existing DICOMDIR");
	cmd.addSubGroup("post-1993 value representations:");
	cmd.addOption("--enable-new-vr",         "+u",     "enable support for new VRs (UN/UT) (default)");
	cmd.addOption("--disable-new-vr",        "-u",     "disable support for new VRs, convert to OB");
	cmd.addSubGroup("group length encoding:");
	cmd.addOption("--group-length-remove",   "-g",     "write without group length elements (default)");
	cmd.addOption("--group-length-create",   "+g",     "write with group length elements");
	cmd.addSubGroup("length encoding in sequences and items:");
	cmd.addOption("--length-explicit",       "+e",     "write with explicit lengths (default)");
	cmd.addOption("--length-undefined",      "-e",     "write with undefined lengths");
	cmd.addSubGroup("index:");
	cmd.addOption("--viewer",						1, "viewer : string", "which viewer will append to DVD, default : eFilm");
    cmd.addOption("--pipe-name",             "-pn", 1, "pipe name : string", "named pipe name, receive file name");

	/* evaluate command line */
	prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
	if (app.parseCommandLine(cmd, argc, argv, OFCommandLine::ExpandWildcards))
	{
		/* print help text and exit */
		if (cmd.getArgCount() == 0)
			app.printUsage();

		/* check exclusive options first */
		if (cmd.getParamCount() == 0)
		{
			if (cmd.findOption("--version"))
			{
				app.printHeader(OFTrue /*print host identifier*/);          // uses ofConsole.lockCerr()
				CERR << endl << "External libraries used:";
#if !defined(WITH_ZLIB) && !defined(BUILD_DCMGPDIR_AS_DCMMKDIR)
				CERR << " none" << endl;
#else
				CERR << endl;
#endif
#ifdef WITH_ZLIB
				CERR << "- ZLIB, Version " << zlibVersion() << endl;
#endif
#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
				CERR << "- " << DiJPEGPlugin::getLibraryVersionString() << endl;
#endif
				return 0;
			}
		}

		/* general options */
		cmd.beginOptionBlock();
		if (cmd.findOption("--verbose"))
        {
			ddir.enableVerboseMode();
            opt_verbose = 1;
        }
		if (cmd.findOption("--quiet"))
		{
			ddir.enableVerboseMode(OFFalse);
			app.setQuietMode();
		}
		cmd.endOptionBlock();

		if (cmd.findOption("--debug"))
			opt_debug = 5;

		/* input options */
		if (cmd.findOption("--output-file"))
			app.checkValue(cmd.getValue(opt_output));
		if (cmd.findOption("--fileset-id"))
			app.checkValue(cmd.getValue(opt_fileset));
		if (cmd.findOption("--descriptor"))
			app.checkValue(cmd.getValue(opt_descriptor));
		if (cmd.findOption("--char-set"))
			app.checkValue(cmd.getValue(opt_charset));

		cmd.beginOptionBlock();
		if (cmd.findOption("--strict"))
			ddir.enableInventMode(OFFalse);
		if (cmd.findOption("--invent"))
			ddir.enableInventMode(OFTrue);
		cmd.endOptionBlock();

		if (cmd.findOption("--input-directory"))
			app.checkValue(cmd.getValue(opt_directory));

		cmd.beginOptionBlock();
		if (cmd.findOption("--keep-filenames"))
			ddir.enableMapFilenamesMode(OFFalse);
		if (cmd.findOption("--map-filenames"))
			ddir.enableMapFilenamesMode(OFTrue);
		cmd.endOptionBlock();

		cmd.beginOptionBlock();
		if (cmd.findOption("--no-recurse"))
			opt_recurse = OFFalse;
		if (cmd.findOption("--recurse"))
			opt_recurse = OFTrue;
		if (opt_directory == NULL)
			opt_directory = ".";
		cmd.endOptionBlock();

#ifdef PATTERN_MATCHING_AVAILABLE
		if (cmd.findOption("--pattern"))
		{
			app.checkDependence("--pattern", "--recurse", opt_recurse);
			app.checkValue(cmd.getValue(opt_pattern));
		}
#endif

		cmd.beginOptionBlock();
		if (cmd.findOption("--no-consistency-check"))
			ddir.disableConsistencyCheck();
		if (cmd.findOption("--warn-inconsist-files"))
			ddir.enableAbortMode(OFFalse);
		if (cmd.findOption("--abort-inconsist-file"))
			ddir.enableAbortMode(OFTrue);
		cmd.endOptionBlock();
		if (cmd.findOption("--invent-patient-id"))
			ddir.enableInventPatientIDMode();

#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
		if (cmd.findOption("--no-encoding-check"))
			ddir.disableEncodingCheck();
		if (cmd.findOption("--no-resolution-check"))
			ddir.disableResolutionCheck();
		if (cmd.findOption("--no-xfer-check"))
			ddir.disableTransferSyntaxCheck();
		if (cmd.findOption("--add-icon-image"))
			ddir.enableIconImageMode();
		if (cmd.findOption("--icon-image-size"))
		{
			OFCmdUnsignedInt iconSize = 0;
			app.checkValue(cmd.getValueAndCheckMinMax(iconSize, 1, 128));
			ddir.setIconSize(OFstatic_cast(unsigned int, iconSize));
		}
		if (cmd.findOption("--icon-file-prefix"))
		{
			const char *iconPrefix = NULL;
			app.checkValue(cmd.getValue(iconPrefix));
			ddir.setIconPrefix(iconPrefix);
		}
		if (cmd.findOption("--default-icon"))
		{
			const char *defaultIcon = NULL;
			app.checkValue(cmd.getValue(defaultIcon));
			ddir.setDefaultIcon(defaultIcon);
		}
#endif

		/* output options */
#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
		cmd.beginOptionBlock();
		if (cmd.findOption("--general-purpose"))
			opt_profile = DicomDirInterface::AP_GeneralPurpose;
		if (cmd.findOption("--general-purpose-dvd"))
			opt_profile = DicomDirInterface::AP_GeneralPurposeDVD;
		if (cmd.findOption("--general-purpose-mime"))
			opt_profile = DicomDirInterface::AP_GeneralPurposeMIME;
		if (cmd.findOption("--usb-and-flash"))
			opt_profile = DicomDirInterface::AP_USBandFlash;
		if (cmd.findOption("--mpeg2-mp-at-ml"))
			opt_profile = DicomDirInterface::AP_MPEG2MPatML;
		if (cmd.findOption("--basic-cardiac"))
			opt_profile = DicomDirInterface::AP_BasicCardiac;
		if (cmd.findOption("--xray-angiographic"))
			opt_profile = DicomDirInterface::AP_XrayAngiographic;
		if (cmd.findOption("--xray-angiographic-dvd"))
			opt_profile = DicomDirInterface::AP_XrayAngiographicDVD;
		if (cmd.findOption("--dental-radiograph"))
			opt_profile = DicomDirInterface::AP_DentalRadiograph;
		if (cmd.findOption("--ct-and-mr"))
			opt_profile = DicomDirInterface::AP_CTandMR;
		if (cmd.findOption("--ultrasound-id-sf"))
			opt_profile = DicomDirInterface::AP_UltrasoundIDSF;
		if (cmd.findOption("--ultrasound-sc-sf"))
			opt_profile = DicomDirInterface::AP_UltrasoundSCSF;
		if (cmd.findOption("--ultrasound-cc-sf"))
			opt_profile = DicomDirInterface::AP_UltrasoundCCSF;
		if (cmd.findOption("--ultrasound-id-mf"))
			opt_profile = DicomDirInterface::AP_UltrasoundIDMF;
		if (cmd.findOption("--ultrasound-sc-mf"))
			opt_profile = DicomDirInterface::AP_UltrasoundSCMF;
		if (cmd.findOption("--ultrasound-cc-mf"))
			opt_profile = DicomDirInterface::AP_UltrasoundCCMF;
		if (cmd.findOption("--12-lead-ecg"))
			opt_profile = DicomDirInterface::AP_TwelveLeadECG;
		if (cmd.findOption("--hemodynamic-waveform"))
			opt_profile = DicomDirInterface::AP_HemodynamicWaveform;
		cmd.endOptionBlock();
#endif

		cmd.beginOptionBlock();
		if (cmd.findOption("--replace"))
		{
			opt_write = OFTrue;
			opt_append = OFFalse;
		}
		if (cmd.findOption("--append"))
		{
			opt_write = OFTrue;
			opt_append = OFTrue;
		}
		if (cmd.findOption("--discard"))
		{
			opt_write = OFFalse;
			opt_append = OFFalse;
		}
		cmd.endOptionBlock();
		if (cmd.findOption("--no-backup"))
			ddir.disableBackupMode();

		cmd.beginOptionBlock();
		if (cmd.findOption("--enable-new-vr"))
		{
			dcmEnableUnknownVRGeneration.set(OFTrue);
			dcmEnableUnlimitedTextVRGeneration.set(OFTrue);
		}
		if (cmd.findOption("--disable-new-vr"))
		{
			dcmEnableUnknownVRGeneration.set(OFFalse);
			dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
		}
		cmd.endOptionBlock();

		cmd.beginOptionBlock();
		if (cmd.findOption("--group-length-create"))
			opt_glenc = EGL_withGL;
		if (cmd.findOption("--group-length-remove"))
			opt_glenc = EGL_withoutGL;
		cmd.endOptionBlock();

		cmd.beginOptionBlock();
		if (cmd.findOption("--length-explicit"))
			opt_enctype = EET_ExplicitLength;
		if (cmd.findOption("--length-undefined"))
			opt_enctype = EET_UndefinedLength;
		cmd.endOptionBlock();

		/* post check */
		if ((opt_profile == DicomDirInterface::AP_BasicCardiac) ||
			(opt_profile == DicomDirInterface::AP_XrayAngiographic) ||
			(opt_profile == DicomDirInterface::AP_CTandMR))
		{
			app.checkConflict("--icon-image-size", "--basic-cardiac, --xray-angiographic or --ct-and-mr", cmd.findOption("--icon-image-size"));
		}
        if (cmd.findOption("--pipe-name"))
			app.checkValue(cmd.getValue(opt_pipename));
	}

	/* set debug mode and stream for log messages */
	SetDebugLevel((opt_debug));
	if (!app.quietMode())
		ddir.setLogStream(&ofConsole);

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

	/* create list of input files */
	OFList<OFString> fileNames;
	OFString pathname;
	const char *param = NULL;
	bool readStdin = false, readPipe = false;
	const int count = cmd.getParamCount();
	if (opt_recurse && ddir.verboseMode()) time_header_out(CERR) << "determining input files ..." << endl;
	/* no parameters? */
	if (count == 0)
	{
		if (opt_recurse)
			OFStandard::searchDirectoryRecursively("", fileNames, opt_pattern, opt_directory);
		else
			app.printError("Missing parameter dcmfile-in");
	} else {
		/* iterate over all input filenames */
		for (int i = 1; i <= count; i++)
		{
			cmd.getParam(i, param);
			if(*param == '-')
			{
				readStdin = true;
				if(ddir.verboseMode()) time_header_out(CERR) << "read file path from stdin" << endl;
				break;
			}
            if(*param == '#' && opt_pipename)
			{
				readPipe = true;
				if(ddir.verboseMode()) time_header_out(CERR) << "read file path from pipe " << opt_pipename << endl;
                if(!opt_directory) opt_directory = ".";
				break;
			}
			/* add input directory */
			OFStandard::combineDirAndFilename(pathname, opt_directory, param, OFTrue /*allowEmptyDirName*/);
			/* search directory recursively (if required) */
			if (opt_recurse && OFStandard::dirExists(pathname))
				OFStandard::searchDirectoryRecursively(param, fileNames, opt_pattern, opt_directory);
			else
				fileNames.push_back(param);
		}
	}

	// remove DICOMDIR for avoiding warning message.
	// algorithm copied from OFList<T>::remove
	OFListIterator(OFString) first = fileNames.begin();
	int clientId = _getpid();
    HANDLE hPipe = INVALID_HANDLE_VALUE;

    char buff[MAX_PATH];
    sprintf_s(buff, "%s\\etc\\settings.ini", GetPacsBase());
    if(LoadSettings(buff, CERR, ddir.verboseMode()))
    {
        if(GetSetting("DicomdirImplClassUID", buff, sizeof(buff)))
        {
            GE_ImplementationClassUID = buff;
            GE_MediaStorageSOPInstanceUID = buff;
            GE_MediaStorageSOPInstanceUID += ".%Y%m%d.%H%M%S.";
        }
    }

    if(readPipe)
    {
        bool pipeStandby = false;
        char pipe_name[MAX_PATH] = "\\\\.\\pipe\\";
        strcat_s(pipe_name, opt_pipename);

        if(ddir.verboseMode())
            time_header_out(CERR) << "dcmmkdir " << clientId << ": StudyUID is " << opt_directory << endl;

        for(int i = 0; i < 100; ++i)
        {
            if(WaitNamedPipe(pipe_name, NMPWAIT_USE_DEFAULT_WAIT))
                pipeStandby = true;
            if(pipeStandby)
            {
                hPipe = CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if(hPipe != INVALID_HANDLE_VALUE) break;
            }
        }
        if(hPipe == INVALID_HANDLE_VALUE)
        {
            time_header_out(CERR) << "dcmmkdir " << clientId << ": can't WaitNamedPipe(" << pipe_name << ")" << endl;
            return -1;
        }
        DWORD dwMode = PIPE_READMODE_MESSAGE;
        BOOL fSuccess = SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
        if(!fSuccess)
        {
            char msg[32];
            DWORD gle = GetLastError();
            sprintf_s(msg, "dcmmkdir %d", clientId);
            displayErrorToCerr(msg, gle);
            return -2;
        }
    }
    else if(!readStdin)
	{
		while(first != fileNames.end())
		{
			if ((*first).find(OFString("DICOMDIR")) != OFString_npos) first = fileNames.erase(first);
			else ++first;
		}

		/* check whether there are any input files */
		if (fileNames.empty())
			app.printError("no input files: DICOMDIR not created");
	}
	PrepareFileDir(opt_output);

#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
	// add image support to DICOMDIR class
	DicomDirImageImplementation imagePlugin;
	ddir.addImageSupport(&imagePlugin);
#endif

	OFCondition result;
	/* create new general purpose DICOMDIR or append to existing one */
	if (opt_append && OFStandard::fileExists(opt_output))
		result = ddir.appendToDicomDir(opt_profile, opt_output);
	else
		result = ddir.createNewDicomDir(opt_profile, opt_output, opt_fileset);

	if(ddir.verboseMode()) time_header_out(CERR) << "dcmmkdir " << clientId << ": begin to add files" << endl;

	// If scu split study into series and one association per series,
	// dcmmkdir must collect csv files as fileNameList, otherwise xml index will be incorrect.
	OFList<OFString> fileNameList;
	if (result.good())
	{
		/* set fileset descriptor and character set */
		result = ddir.setFilesetDescriptor(opt_descriptor, opt_charset);
		if (result.good())
		{
			OFListIterator(OFString) iter = fileNames.begin();
			OFListIterator(OFString) last = fileNames.end();
			/* collect 'bad' files */
			OFList<OFString> badFiles;
			unsigned int goodFiles = 0;
            if(readPipe)
            {
                fnbuf[0] = '\0';
                char *dir = NULL, *pfn = NULL;
				DWORD cbWritten = 0, cbToWrite = 0;
                cbToWrite = sprintf_s(last_file_name, "bind %d %s", clientId, opt_directory); // last_file_name: <client_pid> <study_uid>
                while(true)
                {
                    DWORD cbRead = 0, gle = 0;
                    // pull loop: write last_file_name, indicate that dcmmkdir is ready to read next file.
                    if(!WriteFile(hPipe, last_file_name, cbToWrite, &cbWritten, NULL))
                    {
                        char msg[32];
                        gle = GetLastError();
                        sprintf_s(msg, "dcmmkdir %d WriteFile()", clientId);
                        displayErrorToCerr(msg, gle);
                        break;
                    }
                    if(ddir.verboseMode()) time_header_out(CERR) << "dcmmkdir " << clientId << " WriteFile(): " << last_file_name << endl;

                    gle = 0;
                    do
                    {
                        if(ReadFile(hPipe, fnbuf, sizeof(fnbuf), &cbRead, NULL))
                            gle = 0;
                        else
                            gle = GetLastError();
                    } while(gle == ERROR_MORE_DATA);
                    if(gle)
                    {
                        if(gle == ERROR_PIPE_NOT_CONNECTED)
                        {
                            time_header_out(CERR) << "dcmmkdir " << clientId << ": named pipe has been closed" << endl;
                        }
                        else
                        {
                            char msg[32];
                            sprintf_s(msg, "dcmmkdir %d ReadFile()", clientId);
                            displayErrorToCerr(msg, gle);
                        }
                        break;  // while(true)
                    }
                    fnbuf[cbRead] = '\0';
                    if(ddir.verboseMode()) time_header_out(CERR) << "dcmmkdir " << clientId << " ReadFile(): " << fnbuf << endl;
                    
                    if(strncmp(COMMAND_CLOSE_PIPE, fnbuf, sizeof(COMMAND_CLOSE_PIPE) - 1) == 0)  // server close pipe
                    {
                        if(ddir.verboseMode()) time_header_out(CERR) << "dcmmkdir " << clientId << ": server close named pipe" << endl;
                        DisconnectNamedPipe(hPipe);
                        break;  // while(true)
                    }

                    memset(&nfc, 0, sizeof(nfc));

                    // fnbuf: <study uid>|<unique filename>|<file size receive><LF><assoc_text>
                    char *assoc_text = strchr(fnbuf, '\n');
                    //split association info
                    if(assoc_text)
                    {
                        *assoc_text++ = '\0';
                        strcpy_s(association_buff, assoc_text);
                    }
                    else association_buff[0] = '\0';
                    // split studyUID(fnbuf) and unique filename(pfn)
                    pfn = strchr(fnbuf, '|');
					if(pfn)
					{
						*pfn++ = '\0';
						dir = fnbuf; // study uid is input directory(same as +id option)

                        char *receive_size = strchr(pfn, '|');
                        if(receive_size)
                        {
                            *receive_size++ = '\0';
                            nfc.file.file_size_receive = _atoi64(receive_size);
                        }
					}
					else pfn = fnbuf;
                    // split studyUID(fnbuf) and filename(pfn)

                    /* add files to the DICOMDIR */
                    result = ddir.addDicomFile(pfn, dir && *dir != '\0' ? dir : opt_directory, fill_notify_from_dcmdataset);
					if (result.bad())
					{
                        // save filename for response message that is written to sender
                        sprintf_s(last_file_name, "%s|Unknown", pfn);
						badFiles.push_back(pfn);
						if (!ddir.abortMode())
						{
							/* ignore inconsistent file, just warn (already done inside "ddir") */
							result = EC_Normal;
						}
                        time_header_out(CERR) << "dcmmkdir " << clientId << " add bad file " << pfn << endl;
					}
                    else
                    {
                        if(ddir.verboseMode()) time_header_out(CERR) << "file size receive: " << nfc.file.file_size_receive << endl;
                        DcmXfer dcmxfer(nfc.file.xfer_new);
                        // save filename for response message that is written to sender
						cbToWrite = sprintf_s(last_file_name, "%s|%s|%s", pfn, dcmxfer.getXferShortName(),nfc.patient.patientsName);
						++goodFiles;
						if(ddir.verboseMode()) time_header_out(CERR) << "dcmmkdir " << clientId << " add good file " << pfn << endl;
                    }
                } // while(true)
                CloseHandle(hPipe);
            }
            else if(readStdin)
			{
				while(!(cin.getline(fnbuf, sizeof(fnbuf)).fail()))
				{
					char *dir = NULL, *pfn = strchr(fnbuf, '|');
					if(pfn)
					{
						*pfn++ = '\0';
						dir = fnbuf;
					}
					else pfn = fnbuf;
					if(dir) dir = trim(dir);
					/* add files to the DICOMDIR */
					result = ddir.addDicomFile(trim(pfn), dir && *dir != '\0' ? dir : opt_directory);
					if (result.bad())
					{
						badFiles.push_back(pfn);
						if (!ddir.abortMode())
						{
							/* ignore inconsistent file, just warn (already done inside "ddir") */
							result = EC_Normal;
						}
					} else
						++goodFiles;
				}
				if(ddir.verboseMode()) time_header_out(CERR) << "dcmmkdir " << clientId << ": no more files, stop waiting for stdin" << endl;
			}
			else
			{
				/* iterate over all input filenames */
				while ((iter != last) && result.good())
				{
					/* add files to the DICOMDIR */
					result = ddir.addDicomFile(iter->c_str(), opt_directory);//, fill_notify_from_dcmdataset);
					if (result.bad())
					{
						badFiles.push_back(*iter);
						if (!ddir.abortMode())
						{
							/* ignore inconsistent file, just warn (already done inside "ddir") */
							result = EC_Normal;
						}
					} else ++goodFiles;
					++iter;
				}
			}

			if(ddir.verboseMode()) time_header_out(CERR) << "dicomdir maker: leave adding files, begin to write DICOMDIR" << endl;

			/* evaluate result of file checking/adding procedure */
			if (goodFiles == 0)
			{
				app.printWarning("no good files: DICOMDIR not created", "error");
				result = EC_IllegalCall;
			}
			else if (!badFiles.empty())
			{
				OFOStringStream oss;
				oss << badFiles.size() << " file(s) cannot be added to DICOMDIR: ";
				iter = badFiles.begin();
				last = badFiles.end();
				while (iter != last)
				{
					oss << endl << OFString(strlen(OFFIS_CONSOLE_APPLICATION) + 2, ' ') << (*iter);
					++iter;
				}
				oss << OFStringStream_ends;
				OFSTRINGSTREAM_GETSTR(oss, tmpString)
					app.printWarning(tmpString);
				OFSTRINGSTREAM_FREESTR(tmpString)
			}

			DcmMetaInfo *metinf = ddir.getDicomDir()->getDirFileFormat().getMetaInfo();
			checkValueGE(metinf, DCM_MediaStorageSOPInstanceUID, DICOMDIR_DEFAULT_TRANSFERSYNTAX);
			checkValueGE(metinf, DCM_ImplementationClassUID, DICOMDIR_DEFAULT_TRANSFERSYNTAX);
			checkValueGE(metinf, DCM_ImplementationVersionName, DICOMDIR_DEFAULT_TRANSFERSYNTAX);
			checkValueGE(metinf, DCM_SourceApplicationEntityTitle, DICOMDIR_DEFAULT_TRANSFERSYNTAX);

            /* write DICOMDIR file */
			if (result.good() && opt_write)
            {
				result = ddir.writeDicomDir(opt_enctype, opt_glenc);
                if(opt_verbose || result.bad())
                    time_header_out(CERR) << "write dicomdir " << result.text() << endl;
            }
            else if(result.bad())
                time_header_out(CERR) << "skip writing dicomdir, reason: " << result.text() << endl;

			if(ddir.verboseMode()) time_header_out(CERR) << "dicomdir maker: DICOMDIR is written" << endl;
		}
	}

#ifdef BUILD_DCMGPDIR_AS_DCMMKDIR
	// deregister global decompression codecs
	DcmRLEDecoderRegistration::cleanup();
	DJDecoderRegistration::cleanup();
#endif

#ifdef _DEBUG
	dcmDataDict.clear();  /* useful for debugging with dmalloc */
#endif
	return result.status();
}


/*
* CVS/RCS Log:
* $Log: dcmgpdir.cc,v $
* Revision 1.81  2005/12/08 15:40:48  meichel
* Changed include path schema for all DCMTK header files
*
* Revision 1.80  2005/11/28 15:28:54  meichel
* File dcdebug.h is not included by any other header file in the toolkit
*   anymore, to minimize the risk of name clashes of macro debug().
*
* Revision 1.79  2005/06/13 14:36:07  joergr
* Added new options to disable check on pixel encoding and transfer syntax.
*
* Revision 1.78  2005/03/09 17:56:20  joergr
* Added support for new Media Storage Application Profiles according to DICOM
* PS 3.12-2004. Removed support for non-standard conformant "No profile".
*
* Revision 1.77  2004/05/06 16:37:47  joergr
* Added typecasts to keep Sun CC 2.0.1 quiet.
*
* Revision 1.76  2004/01/16 10:52:58  joergr
* Removed acknowledgements with e-mail addresses from CVS log.
*
* Revision 1.75  2003/08/12 15:22:05  joergr
* Replaced call of OFCommandLine::getValueAndCheckMin() by OFCommandLine::
* getValueAndCheckMinMax() - warning reported by MSVC 5.
*
* Revision 1.74  2003/08/12 14:34:00  joergr
* Adapted implementation to use new DICOMDIR class. Added new command line
* options (e.g. --input-directory or --pattern).
*
* Revision 1.73  2003/05/20 08:50:19  joergr
* Added support for SOP Class "Chest CAD SR" (Supplement 65).
*
* Revision 1.72  2003/03/12 17:32:38  meichel
* Updated DcmObject::print() flags
*
* Revision 1.71  2002/11/27 12:07:17  meichel
* Adapted module dcmdata to use of new header file ofstdinc.h
*
* Revision 1.70  2002/11/26 14:03:02  joergr
* Numerous code purifications, e.g. made local functions "static".
*
* Revision 1.69  2002/11/26 08:43:00  meichel
* Replaced all includes for "zlib.h" with <zlib.h>
*   to avoid inclusion of zlib.h in the makefile dependencies.
*
* Revision 1.68  2002/11/04 16:39:18  joergr
* Added new command line option preventing the creation of a backup of an
* existing DICOMDIR.
*
* Revision 1.67  2002/09/23 17:52:03  joergr
* Prepared code for future support of 'config.guess' host identifiers.
*
* Revision 1.66  2002/09/23 13:50:41  joergr
* Added new command line option "--version" which prints the name and version
* number of external libraries used.
*
* Revision 1.65  2002/08/21 10:14:15  meichel
* Adapted code to new loadFile and saveFile methods, thus removing direct
*   use of the DICOM stream classes.
*
* Revision 1.64  2002/08/13 09:56:44  joergr
* Added new profile (NONE) based on STD-GEN-xxxx which allows DICOM objects
* of any transfer syntax to be referenced from a DICOMDIR.  NB: there's no
* equivilent application profile in the DICOM standard.
*
* Revision 1.63  2002/07/11 16:08:26  joergr
* Added support for CT/MR application profile.  Added general support for
* monochrome icon images.
* Added new command line flags to handle inconsistent header information
* (patient ID and name).
*
* Revision 1.62  2002/07/02 16:52:14  joergr
* Minor fixes to keep MSVC6 quiet.
*
* Revision 1.61  2002/07/02 16:16:16  joergr
* Added support for ultrasound and waveform media storage application profiles.
* Added Mammography CAD SR to the list of supported SOP classes.
*
* Revision 1.60  2002/04/16 13:38:54  joergr
* Added configurable support for C++ ANSI standard includes (e.g. streams).
*
* Revision 1.59  2002/04/11 12:35:54  joergr
* Replaced direct call of system routines by new standard date and time
* functions.
* Use the new standard file system routines like fileExists() etc.
*
* Revision 1.58  2001/12/06 14:03:16  joergr
* Minor "stylistic" changes.
*
* Revision 1.57  2001/11/29 16:51:45  joergr
* Added new command line option to dcmmkdir that allows to ignore non-standard
* conformant spatial resolutions for images (e.g. images larger than 1024*1024
* for the cardiac profiles).
*
* Revision 1.56  2001/11/19 17:53:36  joergr
* Implemented performance optimization for the generation of icon images of
* compressed multi-frame images.
*
* Revision 1.55  2001/11/19 12:43:17  joergr
* Re-added dcmgpdir tool to dcmdata module.
*
* Revision 1.1  2001/11/13 17:57:14  joergr
* Replaced utility dcmgpdir with dcmmkdir which supports other Media Storage
* Application Profiles in addition to the General Purpose one.
*
*
*/
