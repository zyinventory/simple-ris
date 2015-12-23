#include "stdafx.h"
#include "../ofstd/include/dcmtk/ofstd/x_www_form_codec.h"
#include "commonlib.h"

#define TYPE_ASSOC      'T'
#define TYPE_FILE       'F'
#define TYPE_PATIENT    'P'
#define TYPE_STUDY      'S'
#define TYPE_SERIES     'E'
#define TYPE_INSTANCE   'I'

#define ASSOC_ESTA  0x00010010
#define FILE_START  0x00011000
#define ASSOC_TERM  0xFFFFFFFF
#define ASSOC_ABORT 0xFFFFFFFD

using namespace std;

static bool opt_verbose = false;
static const char *sessionId;
static CMOVE_LOG_CONTEXT  lc;

static void clear_series_section(CMOVE_SERIES_SECTION &ps)
{
    ps.seriesUID[0] = '\0';
    ps.modality[0] = '\0';
}

static int cmd_series(char type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    string temp;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00080060:
        cmdstrm >> lc.series.modality;
        if(strlen(lc.series.modality) == 0)
            cerr << "Unexpected empty modality " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            dirty = true;
            if(opt_verbose)
                cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.series.modality << endl;
        }
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    if(dirty && strlen(lc.series.seriesUID) == 0) strcpy_s(lc.series.seriesUID, lc.file.seriesUID);
    return 1;
}

static void clear_study_section(CMOVE_STUDY_SECTION &ps)
{
    ps.studyUID[0] = '\0';
    ps.accessionNumber[0] = '\0';
    ps.studyDate[0] = '\0';
    ps.studyTime[0] = '\0';
}

static int cmd_study(char type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    string temp;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00080020:
        cmdstrm >> lc.study.studyDate;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyDate << endl;
        break;
    case 0x00080030:
        cmdstrm >> lc.study.studyTime;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyTime << endl;
        break;
    case 0x00080050:
        cmdstrm >> temp;
        dirty = true;
        x_www_form_codec<char>::decode(temp.c_str(), lc.study.accessionNumber, sizeof(lc.study.accessionNumber));
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    if(dirty && strlen(lc.study.studyUID) == 0) strcpy_s(lc.study.studyUID, lc.file.studyUID);
    return 1;
}

static void clear_patient_section(CMOVE_PATIENT_SECTION &ps)
{
    ps.patientID[0] = '\0';
    ps.patientsName[0] = '\0';
    ps.birthday[0] = '\0';
    ps.height[0] = '\0';
    ps.weight[0] = '\0';
    ps.sex = ' ';
}

static int cmd_patient(char type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    string temp_patients_name;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00100010:
        cmdstrm >> temp_patients_name;
        dirty = true;
        if(temp_patients_name.empty())
            cerr << "Unexpected empty patient's name " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            x_www_form_codec<char>::decode(temp_patients_name.c_str(), lc.patient.patientsName, sizeof(lc.patient.patientsName));
            if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp_patients_name << endl;
        }
        break;
    case 0x00100030:
        cmdstrm >> lc.patient.birthday;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.birthday << endl;
        break;
    case 0x00100040:
        cmdstrm >> lc.patient.sex;
        if(lc.patient.sex != 'M' && lc.patient.sex != 'F' && lc.patient.sex != 'O') lc.patient.sex = ' ';
        else if(strlen(lc.patient.patientID) == 0) dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.sex << endl;
        break;
    case 0x00101020:
        cmdstrm >> lc.patient.height;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.height << endl;
        break;
    case 0x00101030:
        cmdstrm >> lc.patient.weight;
        dirty = true;
        if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.weight << endl;
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    if(dirty && strlen(lc.patient.patientID) == 0) strcpy_s(lc.patient.patientID, lc.file.patientID);
    return 1;
}

static int cmd_instance(char type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    string temp;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00100020:
        cmdstrm >> temp;
        if(temp.empty())
            cerr << "Unexpected empty patientID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
            x_www_form_codec<char>::decode(temp.c_str(), lc.file.patientID, sizeof(lc.file.patientID));
        }
        break;
    case 0x0020000D:
        cmdstrm >> lc.file.studyUID;
        if(strlen(lc.file.studyUID) == 0)
            cerr << "Unexpected empty studyUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.studyUID << endl;
        break;
    case 0x0020000E:
        cmdstrm >> lc.file.seriesUID;
        if(strlen(lc.file.seriesUID) == 0)
            cerr << "Unexpected empty seriesUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.seriesUID << endl;
        break;
    case 0x00080018:
        cmdstrm >> lc.file.instanceUID;
        if(strlen(lc.file.instanceUID) == 0)
            cerr << "Unexpected empty instanceUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.instanceUID << endl;
        break;
    case 0x00020010:
        cmdstrm >> lc.file.xfer;
        if(strlen(lc.file.xfer) == 0)
            cerr << "Unexpected empty xfer " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.xfer << endl;
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    return 1;
}

static void print_error_file_section(unsigned int tag, string &filename, CMOVE_FILE_SECTION &fs)
{
    if(filename.empty())
        cerr << "Unexpected commit F " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << ", old value:" << endl;
    else
        cerr << "Unexpected new F " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << ", old value:" << endl;
    cerr << "\tinFile: " << fs.inFile << endl;
    cerr << "\ttag: " << hex << uppercase << setw(8) << setfill('0') << fs.tag << endl;
    cerr << "\tfileName: " << fs.filename << endl;
    cerr << "\tpatientID: " << fs.patientID << endl;
    cerr << "\tstudyUID: " << fs.studyUID << endl;
    cerr << "\tseriesUID: " << fs.seriesUID << endl;
    cerr << "\tinstanceUID: " << fs.instanceUID << endl;
    cerr << "\txfer: " << fs.xfer << endl;
}

static void clear_file_section(CMOVE_FILE_SECTION &fs)
{
    fs.filename[0] = '\0';
    fs.tag = 0;
    fs.patientID[0] = '\0';
    fs.studyUID[0] = '\0';
    fs.seriesUID[0] = '\0';
    fs.instanceUID[0] = '\0';
    fs.xfer[0] = '\0';
    fs.inFile = false;
}

static int cmd_file(char type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    string filename;
    cmdstrm >> hex >> tag >> filename;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << endl;
    if(filename.empty())
    {   // test before commit file section 
        if(!lc.file.inFile || tag != lc.file.tag || strlen(lc.file.filename) == 0 || strlen(lc.file.patientID) == 0
            || strlen(lc.file.studyUID) == 0 || strlen(lc.file.seriesUID) == 0 || strlen(lc.file.instanceUID) == 0
            || strlen(lc.file.xfer) == 0) // error, print unexpected value
            print_error_file_section(tag, filename, lc.file);
        else // OK, commit file section
        {
            cerr << "commit file" << endl;
        }
        lc.file.inFile = false;
    }
    else
    {   // new filename
        if(lc.file.inFile) // error, print unexpected value
            print_error_file_section(tag, filename, lc.file);
        // enter file section, clear all UID
        clear_file_section(lc.file);
        clear_patient_section(lc.patient);
        clear_study_section(lc.study);
        clear_series_section(lc.series);

        filename._Copy_s(lc.file.filename, sizeof(lc.file.filename), filename.length());
        lc.file.tag = tag;
        lc.file.inFile = true;
    }
    return 1;
}

static int cmd_assoc(char type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    cmdstrm >> hex >> tag;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag;
    switch(tag)
    {
    case ASSOC_ESTA:
        cmdstrm >> lc.assoc.callingAE >> lc.assoc.callingAddr >> lc.assoc.calledAE >> dec >> lc.assoc.port >> lc.assoc.calledAddr;
        if(opt_verbose) cerr << " " << lc.assoc.callingAE << " " << lc.assoc.callingAddr<< " " << lc.assoc.calledAE<< " " << dec << lc.assoc.port << " " << lc.assoc.calledAddr << endl;
        break;
    case ASSOC_TERM:
    case ASSOC_ABORT:
        if(opt_verbose) cerr << endl;
        return 0;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        if(!opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " ";
        cerr << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    return 1;
}

static int process_cmd(const char *buf, size_t buf_len)
{
    char type = '\0';
    istringstream cmdstrm(buf);
    cmdstrm >> type;

    switch(type)
    {
    case TYPE_ASSOC:
        return cmd_assoc(type, cmdstrm, lc);
    case TYPE_FILE:
        cmd_file(type, cmdstrm, lc);
        break;
    case TYPE_PATIENT:
        if(lc.file.inFile) cmd_patient(type, cmdstrm, lc);
        break;
    case TYPE_STUDY:
        if(lc.file.inFile) cmd_study(type, cmdstrm, lc);
        break;
    case TYPE_SERIES:
        if(lc.file.inFile) cmd_series(type, cmdstrm, lc);
        break;
    case TYPE_INSTANCE:
        if(lc.file.inFile) cmd_instance(type, cmdstrm, lc);
        break;
    default:
        cerr << "can't recognize command: " << buf << endl;
    }
    return 1;
}

static char buff[1024], pacs_base[MAX_PATH];

COMMONLIB_API void process_log(const char *sessId, bool verbose)
{
    size_t gpos = 0;
    opt_verbose = verbose;
    sessionId = sessId;
    string fn("\\storedir\\");
    fn.append(sessionId);
    if(GetPacsBase(pacs_base, MAX_PATH, fn.c_str()))
    {
        cerr << "无法切换工作目录" << endl;
        return;
    }
    ifstream tail("cmove.txt", ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << fn << endl;
        return;
    }

    int ret = 1, waitTime = 0;
    lc.assoc.port = 0;
    clear_file_section(lc.file);
    clear_patient_section(lc.patient);
    clear_study_section(lc.study);
    clear_series_section(lc.series);

    while(ret && waitTime <= 10 * 1000)
    {
        tail.getline(buff + gpos, sizeof(buff) - gpos);
        streamsize gcnt = tail.gcount();
        if(gcnt > 0)
        {
            waitTime = 0;
            gpos += static_cast<size_t>(gcnt);
            if(!tail.fail()) 
            {
                ret = process_cmd(buff, gpos);
                gpos = 0;
            }
        }
        if(tail.fail() || tail.eof())
        {
            if(tail.fail()) cerr << " fail";
            if(tail.eof()) cerr << " eof";
            cerr << endl;
            tail.clear();
            waitTime += 100;
            Sleep(100);
        }
    }
    tail.close();
}
