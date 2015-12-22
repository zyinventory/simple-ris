#include "stdafx.h"
#include "dcmtk/ofstd/x_www_form_codec.h"

using namespace std;

static bool opt_verbose = true;
static ASSOC_SECTION assocBuff;
static FILE_SECTION fileBuff;
static PATIENT_SECTION patientBuff;
static STUDY_SECTION studyBuff;
static SERIES_SECTION seriesBuff;

static void clear_series_section(SERIES_SECTION &ps)
{
    ps.seriesUID[0] = '\0';
    ps.modality[0] = '\0';
}

static int cmd_series(char type, istringstream &cmdstrm, SERIES_SECTION &series)
{
    unsigned int tag = 0;
    string temp;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00080060:
        cmdstrm >> series.modality;
        if(strlen(series.modality) == 0)
            cerr << "Unexpected empty modality " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            dirty = true;
            if(opt_verbose)
                cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << series.modality << endl;
        }
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    if(dirty && strlen(series.seriesUID) == 0) strcpy_s(series.seriesUID, fileBuff.seriesUID);
    return 1;
}

static void clear_study_section(STUDY_SECTION &ps)
{
    ps.studyUID[0] = '\0';
    ps.accessionNumber[0] = '\0';
    ps.studyDate[0] = '\0';
    ps.studyTime[0] = '\0';
}

static int cmd_study(char type, istringstream &cmdstrm, STUDY_SECTION &study)
{
    unsigned int tag = 0;
    string temp;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00080020:
        cmdstrm >> study.studyDate;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << study.studyDate << endl;
        break;
    case 0x00080030:
        cmdstrm >> study.studyTime;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << study.studyTime << endl;
        break;
    case 0x00080050:
        cmdstrm >> temp;
        dirty = true;
        x_www_form_codec<char>::decode(temp.c_str(), study.accessionNumber, sizeof(study.accessionNumber));
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    if(dirty && strlen(study.studyUID) == 0) strcpy_s(study.studyUID, fileBuff.studyUID);
    return 1;
}

static void clear_patient_section(PATIENT_SECTION &ps)
{
    ps.patientID[0] = '\0';
    ps.patientsName[0] = '\0';
    ps.birthday[0] = '\0';
    ps.height[0] = '\0';
    ps.weight[0] = '\0';
    ps.sex = ' ';
}

static int cmd_patient(char type, istringstream &cmdstrm, PATIENT_SECTION &patient)
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
            x_www_form_codec<char>::decode(temp_patients_name.c_str(), patient.patientsName, sizeof(patient.patientsName));
            if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp_patients_name << endl;
        }
        break;
    case 0x00100030:
        cmdstrm >> patient.birthday;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.birthday << endl;
        break;
    case 0x00100040:
        cmdstrm >> patient.sex;
        if(patient.sex != 'M' && patient.sex != 'F' && patient.sex != 'O') patient.sex = ' ';
        else if(strlen(patient.patientID) == 0) dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.sex << endl;
        break;
    case 0x00101020:
        cmdstrm >> patient.height;
        dirty = true;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.height << endl;
        break;
    case 0x00101030:
        cmdstrm >> patient.weight;
        dirty = true;
        if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.weight << endl;
        break;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
            << " " << otherbuf << ", encounter error" << endl;
        break;
    }
    if(dirty && strlen(patient.patientID) == 0) strcpy_s(patient.patientID, fileBuff.patientID);
    return 1;
}

static int cmd_instance(char type, istringstream &cmdstrm, FILE_SECTION &file)
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
            x_www_form_codec<char>::decode(temp.c_str(), file.patientID, sizeof(file.patientID));
        }
        break;
    case 0x0020000D:
        cmdstrm >> file.studyUID;
        if(strlen(file.studyUID) == 0)
            cerr << "Unexpected empty studyUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << file.studyUID << endl;
        break;
    case 0x0020000E:
        cmdstrm >> file.seriesUID;
        if(strlen(file.seriesUID) == 0)
            cerr << "Unexpected empty seriesUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << file.seriesUID << endl;
        break;
    case 0x00080018:
        cmdstrm >> file.instanceUID;
        if(strlen(file.instanceUID) == 0)
            cerr << "Unexpected empty instanceUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << file.instanceUID << endl;
        break;
    case 0x00020010:
        cmdstrm >> file.xfer;
        if(strlen(file.xfer) == 0)
            cerr << "Unexpected empty xfer " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << file.xfer << endl;
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

static void print_error_file_section(unsigned int tag, string &filename, FILE_SECTION &fs)
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

static void clear_file_section(FILE_SECTION &fs)
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

static int cmd_file(char type, istringstream &cmdstrm, FILE_SECTION &file)
{
    unsigned int tag = 0;
    string filename;
    cmdstrm >> hex >> tag >> filename;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << endl;
    if(filename.empty())
    {   // test before commit file section 
        if(!file.inFile || tag != file.tag || strlen(file.filename) == 0 || strlen(file.patientID) == 0
            || strlen(file.studyUID) == 0 || strlen(file.seriesUID) == 0 || strlen(file.instanceUID) == 0
            || strlen(file.xfer) == 0) // error, print unexpected value
            print_error_file_section(tag, filename, file);
        else
        {   // OK, commit file section
            //save_file();
            if(strcmp(file.patientID, patientBuff.patientID) == 0)
            {   //refresh_patient_info();
                cerr << "refresh patient info" << endl;
            }
        }
        file.inFile = false;
    }
    else
    {   // new filename
        if(file.inFile) // error, print unexpected value
            print_error_file_section(tag, filename, file);
        // enter file section, clear all UID
        clear_file_section(file);
        clear_patient_section(patientBuff);
        clear_study_section(studyBuff);
        clear_series_section(seriesBuff);

        filename._Copy_s(file.filename, sizeof(file.filename), filename.length());
        file.tag = tag;
        file.inFile = true;
    }
    return 1;
}

static int cmd_assoc(char type, istringstream &cmdstrm, ASSOC_SECTION &assoc)
{
    unsigned int tag = 0;
    cmdstrm >> hex >> tag;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag;
    switch(tag)
    {
    case ASSOC_ESTA:
        cmdstrm >> assoc.callingAE >> assoc.callingAddr >> assoc.calledAE >> dec >> assoc.port >> assoc.calledAddr;
        if(opt_verbose) cerr << " " << assoc.callingAE << " " << assoc.callingAddr<< " " << assoc.calledAE<< " " << dec << assoc.port << " " << assoc.calledAddr << endl;
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
    unsigned int tag;
    string value;
    
    istringstream cmdstrm(buf);
    cmdstrm >> type;

    switch(type)
    {
    case TYPE_ASSOC:
        return cmd_assoc(type, cmdstrm, assocBuff);
    case TYPE_FILE:
        cmd_file(type, cmdstrm, fileBuff);
        break;
    case TYPE_PATIENT:
        if(fileBuff.inFile) cmd_patient(type, cmdstrm, patientBuff);
        break;
    case TYPE_STUDY:
        if(fileBuff.inFile) cmd_study(type, cmdstrm, studyBuff);
        break;
    case TYPE_SERIES:
        if(fileBuff.inFile) cmd_series(type, cmdstrm, seriesBuff);
        break;
    case TYPE_INSTANCE:
        if(fileBuff.inFile) cmd_instance(type, cmdstrm, fileBuff);
        break;
    default:
        cerr << "can't recognize command: " << buf << endl;
    }
    return 1;
}

static char buff[1024];
void process_log(const string &sessionId)
{
    size_t gpos = 0;
    string fn(sessionId);
    fn.append(1, '\\').append("cmove.txt");
    ifstream tail(fn, ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << fn << endl;
        return;
    }

    int ret = 1, waitTime = 0;
    assocBuff.port = 0;
    clear_file_section(fileBuff);
    clear_patient_section(patientBuff);
    clear_study_section(studyBuff);
    clear_series_section(seriesBuff);

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
