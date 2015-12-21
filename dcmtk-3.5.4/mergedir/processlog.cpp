#include "stdafx.h"
#include "dcmtk/ofstd/x_www_form_codec.h"

using namespace std;

static bool opt_verbose = true;
static ASSOC_SECTION assoc;
static FILE_SECTION file;
static PATIENT_SECTION patient;

static void clear_patient_section(PATIENT_SECTION &ps)
{
    ps.patientID[0] = '\0';
    ps.patientsName[0] = '\0';
    ps.birthday[0] = '\0';
    ps.height[0] = '\0';
    ps.weight[0] = '\0';
    ps.sex = ' ';
}

static int cmd_patient(char type, istringstream &cmdstrm)
{
    unsigned int tag = 0;
    string temp_patients_name;
    
    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00100010:
        cmdstrm >> temp_patients_name;
        if(strlen(patient.patientID) == 0) strcpy_s(patient.patientID, file.patientID);
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
        if(strlen(patient.patientID) == 0) strcpy_s(patient.patientID, file.patientID);
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.birthday << endl;
        break;
    case 0x00100040:
        cmdstrm >> patient.sex;
        if(patient.sex != 'M' && patient.sex != 'F' && patient.sex != 'O') patient.sex = ' ';
        else if(strlen(patient.patientID) == 0) strcpy_s(patient.patientID, file.patientID);
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.sex << endl;
        break;
    case 0x00101020:
        cmdstrm >> patient.height;
        if(strlen(patient.patientID) == 0) strcpy_s(patient.patientID, file.patientID);
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.height << endl;
        break;
    case 0x00101030:
        cmdstrm >> patient.weight;
        if(strlen(patient.patientID) == 0) strcpy_s(patient.patientID, file.patientID);
        if(opt_verbose)
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << patient.weight << endl;
        break;
    }
    return 1;
}

static int cmd_instance(char type, istringstream &cmdstrm)
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
    }
    return 1;
}

static void print_error_file_section(unsigned int tag, string &filename, FILE_SECTION &fs)
{
    if(filename.empty())
        cerr << "Unexpected commit F " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << ", old value:" << endl;
    else
        cerr << "Unexpected new F " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << ", old value:" << endl;
    cerr << "\tinFile: " << file.inFile << endl;
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

static int cmd_file(char type, istringstream &cmdstrm)
{
    unsigned int tag = 0;
    string filename;
    cmdstrm >> hex >> tag >> filename;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << endl;
    if(filename.empty())
    {   // commit file section 
        if(!file.inFile || tag != file.tag || strlen(file.filename) == 0 || strlen(file.patientID) == 0
            || strlen(file.studyUID) == 0 || strlen(file.seriesUID) == 0 || strlen(file.instanceUID) == 0
            || strlen(file.xfer) == 0) // error, print unexpected value
            print_error_file_section(tag, filename, file);
        else
        {   // commit file section
            //save_file();
            if(strcmp(file.patientID, patient.patientID) == 0)
            {
                //refresh_patient_info();
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
        clear_patient_section(patient);
        filename._Copy_s(file.filename, sizeof(file.filename), filename.length());
        file.tag = tag;
        file.inFile = true;
    }
    return 1;
}

static int cmd_assoc(char type, istringstream &cmdstrm)
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
        cerr << otherbuf << ": can't process" << endl;
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
        return cmd_assoc(type, cmdstrm);
    case TYPE_FILE:
        cmd_file(type, cmdstrm);
        break;
    case TYPE_PATIENT:
        if(file.inFile) cmd_patient(type, cmdstrm);
    case TYPE_STUDY:
    case TYPE_SERIES:
        cmdstrm >> hex >> tag >> value;
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << value << endl;
        break;
    case TYPE_INSTANCE:
        if(file.inFile) cmd_instance(type, cmdstrm);
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
    assoc.port = 0;
    clear_file_section(file);
    clear_patient_section(patient);

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
