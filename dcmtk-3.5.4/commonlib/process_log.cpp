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

bool opt_verbose = false;

static const char *sessionId;
static CMOVE_LOG_CONTEXT  lc;

static void clear_log_context(CMOVE_LOG_CONTEXT &lc)
{
    memset(&lc, 0, sizeof(CMOVE_LOG_CONTEXT));
    lc.hprocess = INVALID_HANDLE_VALUE;
    lc.hthread = INVALID_HANDLE_VALUE;
    lc.log = INVALID_HANDLE_VALUE;
    lc.patient.sex = ' ';
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
        cmdstrm >> lc.file.xfer >> lc.file.isEncapsulated;
        if(strlen(lc.file.xfer) == 0)
            cerr << "Unexpected empty xfer " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.xfer << " " << lc.file.isEncapsulated << endl;
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

bool commit_file_to_workers(CMOVE_LOG_CONTEXT *lc);

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
            commit_file_to_workers(&lc);
        lc.file.inFile = false;
    }
    else
    {   // new filename
        if(lc.file.inFile) // error, print unexpected value
            print_error_file_section(tag, filename, lc.file);
        // enter file section, clear all UID
        clear_log_context(lc);
        filename._Copy_s(lc.file.filename, sizeof(lc.file.filename), filename.length());
        lc.file.filename[filename.length()] = '\0';
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

bool run_index();
char pacs_base[MAX_PATH];

static char buff[1024];
static size_t gpos = 0;

static bool try_read_line(ifstream &tail)
{
    tail.getline(buff + gpos, sizeof(buff) - gpos);
    streamsize gcnt = tail.gcount();
    if(gcnt > 0) gpos += static_cast<size_t>(gcnt);
    if(tail.fail() || tail.eof())
    {
        cerr << "read " << setw(2) << setfill(' ') << gcnt << " bytes";
        if(tail.eof()) cerr << ", encouter eof";
        cerr << endl;
        tail.clear();
        return false;
    }
    else
        return true;
}

COMMONLIB_API void process_log(const char *sessId, bool verbose)
{
    
    opt_verbose = verbose;
    sessionId = sessId;
    string fn("\\storedir\\");
    fn.append(sessionId);
    if(ChangeToPacsWebSub(pacs_base, MAX_PATH, fn.c_str()))
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
    clear_log_context(lc);

    while(ret && waitTime <= 10 * 1000)
    {
        if(try_read_line(tail))
        {
            ret = process_cmd(buff, gpos);
            gpos = 0;
        }
        else
        {
            waitTime += 100;
            Sleep(100);
        }
    }
    tail.close();
    while(commit_file_to_workers(NULL));
    while(run_index());
}