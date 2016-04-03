#include "stdafx.h"
#include "../ofstd/include/dcmtk/ofstd/x_www_form_codec.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;

void x_www_form_codec_encode(const char *data, ostream *ostrm)
{
    x_www_form_codec<ostream>::encode(data, ostrm);
}

static int cmd_series(const char *type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
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
            if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.series.modality << endl;
        }
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    if(dirty) strcpy_s(lc.series.seriesUID, lc.file.seriesUID);
    return 1;
}

static int cmd_study(const char *type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
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
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    if(dirty)
        strcpy_s(lc.study.studyUID, lc.file.studyUID);
    return 1;
}

static int cmd_patient(const char *type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
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
        if(lc.patient.sex[0] != 'M' && lc.patient.sex[0] != 'F' && lc.patient.sex[0] != 'O')
        {
            lc.patient.sex[0] = ' ';
            lc.patient.sex[1] = '\0';
        }
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
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.weight << endl;
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    if(dirty) strcpy_s(lc.patient.patientID, lc.file.patientID);
    return 1;
}

static int cmd_instance(const char *type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
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
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
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

    cerr << "\ttag: " << hex << uppercase << setw(8) << setfill('0') << fs.tag << endl;
    cerr << "\tfileName: " << fs.filename << endl;
    cerr << "\tpatientID: " << fs.patientID << endl;
    cerr << "\tstudyUID: " << fs.studyUID << endl;
    cerr << "\tseriesUID: " << fs.seriesUID << endl;
    cerr << "\tinstanceUID: " << fs.instanceUID << endl;
    cerr << "\txfer: " << fs.xfer << endl;
}

static int cmd_file(const char *type, istringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    string filename;
    cmdstrm >> hex >> tag >> filename;
    if(tag >= NOTIFY_FILE_SEQ_START)
    {
        if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << filename << endl;
        lc.file.tag = tag;
        if(!filename.empty()) filename._Copy_s(lc.file.filename, sizeof(lc.file.filename), filename.length());
    }
    else
        cerr << "cmd_file() invalid file cmd: " << cmdstrm.str() << endl;

    return 1;
}

static int cmd_assoc_store(const char *type, stringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    cmdstrm >> hex >> tag;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag;
    int status = 1;
    switch(tag)
    {
    case NOTIFY_ASSOC_ESTA:
        cmdstrm >> lc.assoc.id >> lc.assoc.callingAE >> lc.assoc.callingAddr >> lc.assoc.calledAE >> dec >> lc.assoc.port >> lc.assoc.calledAddr;
        lc.assoc.assoc_disconn = false;
        if(opt_verbose) cerr << " " << lc.assoc.callingAE << " " << lc.assoc.callingAddr<< " " << lc.assoc.calledAE<< " " << dec << lc.assoc.port << " " << lc.assoc.calledAddr << endl;
        break;
    case NOTIFY_ASSOC_RELEASE:
        lc.assoc.assoc_disconn = true;
        lc.assoc.releaseOK = true;
        break;
    case NOTIFY_ASSOC_ABORT:
        lc.assoc.assoc_disconn = true;
        lc.assoc.releaseOK = false;
        status = 0;
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            if(!opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " ";
            cerr << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    return status;
}

static int cmd_move(const char *type, stringstream &cmdstrm, CMOVE_LOG_CONTEXT &lc)
{
    unsigned int tag = 0;
    cmdstrm >> hex >> tag;
    if(opt_verbose) cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
    switch(tag)
    {
    case NOTIFY_ASSOC_ESTA:
        lc.is_move = true;
        lc.move_disconn = false;
        break;
    case NOTIFY_ASSOC_RELEASE:
        lc.move_disconn = true;
        lc.moveOK = true;
        break;
    case NOTIFY_ASSOC_ABORT:
        lc.move_disconn = true;
        lc.moveOK = false;
        cerr << "cmd_move() move association abort" << endl;
        break;
    default:
        cerr << "cmd_move() failed: " << cmdstrm.str() << endl;
    }
    return lc.move_disconn ? 0 : 1;
}

static CMOVE_LOG_CONTEXT  static_lc;

void clear_log_context(CMOVE_LOG_CONTEXT *plc)
{
    if(plc == NULL) plc = &static_lc;
    plc->hprocess = INVALID_HANDLE_VALUE;
    plc->hthread = INVALID_HANDLE_VALUE;
    plc->log = INVALID_HANDLE_VALUE;
    plc->is_move = false;
    plc->moveOK = false;
    memset(&plc->file, 0, sizeof(CMOVE_FILE_SECTION));
    memset(&plc->patient, 0, sizeof(CMOVE_PATIENT_SECTION));
    plc->patient.sex[0] = ' ';
    plc->patient.sex[1] = '\0';
    memset(&plc->study, 0, sizeof(CMOVE_STUDY_SECTION));
    memset(&plc->series, 0, sizeof(CMOVE_SERIES_SECTION));
}

int process_cmd(stringstream &cmdstrm, int ftype, const string &filename)
{
    if(ftype == CHAR4_TO_INT(NOTIFY_FILE_TAG))
    {
        char *cmd = new char[1024];
        do{
            cmdstrm.getline(cmd, 1024);
            if(cmdstrm.gcount())
            {
                istringstream linestrm(cmd);
                string type;
                linestrm >> type;
                if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_INSTANCE))
                    cmd_instance(type.c_str(), linestrm, static_lc);
                else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_PATIENT))
                    cmd_patient(type.c_str(), linestrm, static_lc);
                else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_STUDY))
                    cmd_study(type.c_str(), linestrm, static_lc);
                else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_SERIES))
                    cmd_series(type.c_str(), linestrm, static_lc);
                else if(STRING_PRE4_TO_INT(type) == ftype)
                    cmd_file(type.c_str(), linestrm, static_lc);
                else 
                    cerr << "process_cmd() can't recognize line: " << linestrm.str() << endl;
            }
        } while(cmdstrm.good());
        if(cmd) delete[] cmd;

        // test before commit file section 
        if(strlen(static_lc.file.filename) == 0 || strlen(static_lc.file.patientID) == 0
            || strlen(static_lc.file.studyUID) == 0 || strlen(static_lc.file.seriesUID) == 0 || strlen(static_lc.file.instanceUID) == 0
            || strlen(static_lc.file.xfer) == 0) // error, print unexpected value
            cerr << "process_cmd() this file is not file_notify:" << endl << cmdstrm.str();
        else // OK, commit file section
        {
            if(strlen(static_lc.file.unique_filename) == 0) static_lc.file.StorePath('\\');
            compress_queue_to_workers(&static_lc);
        }
        clear_log_context();
    }
    else if(ftype == CHAR4_TO_INT(NOTIFY_MOVE_TAG))
    {
        string type;
        cmdstrm >> type;
        int status = cmd_move(type.c_str(), cmdstrm, static_lc);
        if(static_lc.move_disconn && static_lc.moveOK)
            ; // todo: trigger move OK event
        return status;
    }
    else if(ftype == CHAR4_TO_INT(NOTIFY_STORE_TAG))
    {
        string type;
        cmdstrm >> type;
        cmd_assoc_store(type.c_str(), cmdstrm, static_lc);
        if(!static_lc.is_move)
        {
            if(static_lc.assoc.assoc_disconn)
            {
                if(static_lc.assoc.releaseOK)
                    ;  // todo: trigger store OK event
                return 0;
            }
        }
    }
    else if(ftype == CHAR4_TO_INT(NOTIFY_ACKN_TAG))
    {
        //if(type == CHAR4_TO_INT(NOTIFY_ACKN_ITEM))
    }
    else
        cerr << "can't recognize file " << filename << endl << cmdstrm.str() << endl;
    return 1;
}
