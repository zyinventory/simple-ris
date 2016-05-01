#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"
#include "../ofstd/include/dcmtk/ofstd/x_www_form_codec.h"

using namespace std;
using namespace handle_context;

static char linebuff[1024];

int cmd_instance(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog)
{
    unsigned int tag = 0;
    string temp;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00100020:
        cmdstrm >> temp;
        if(temp.empty())
            time_header_out(flog) << "Unexpected empty patientID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
            x_www_form_codec<char>::decode(temp.c_str(), lc.file.patientID, sizeof(lc.file.patientID));
        }
        break;
    case 0x0020000D:
        cmdstrm >> lc.file.studyUID;
        if(strlen(lc.file.studyUID) == 0)
            time_header_out(flog) << "Unexpected empty studyUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.studyUID << endl;
        break;
    case 0x0020000E:
        cmdstrm >> lc.file.seriesUID;
        if(strlen(lc.file.seriesUID) == 0)
            time_header_out(flog) << "Unexpected empty seriesUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.seriesUID << endl;
        break;
    case 0x00080018:
        cmdstrm >> lc.file.instanceUID;
        if(strlen(lc.file.instanceUID) == 0)
            time_header_out(flog) << "Unexpected empty instanceUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.instanceUID << endl;
        break;
    case 0x00020010:
        cmdstrm >> lc.file.xfer >> lc.file.isEncapsulated;
        if(strlen(lc.file.xfer) == 0)
            time_header_out(flog) << "Unexpected empty xfer " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(opt_verbose)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.xfer << " " << lc.file.isEncapsulated << endl;
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    return 0;
}

int cmd_patient(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog)
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
            time_header_out(flog) << "Unexpected empty patient's name " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            x_www_form_codec<char>::decode(temp_patients_name.c_str(), lc.patient.patientsName, sizeof(lc.patient.patientsName));
            if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp_patients_name << endl;
        }
        break;
    case 0x00100030:
        cmdstrm >> lc.patient.birthday;
        dirty = true;
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.birthday << endl;
        break;
    case 0x00100040:
        cmdstrm >> lc.patient.sex;
        if(lc.patient.sex[0] != 'M' && lc.patient.sex[0] != 'F' && lc.patient.sex[0] != 'O')
        {
            lc.patient.sex[0] = ' ';
            lc.patient.sex[1] = '\0';
        }
        else if(strlen(lc.patient.patientID) == 0) dirty = true;
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.sex << endl;
        break;
    case 0x00101020:
        cmdstrm >> lc.patient.height;
        dirty = true;
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.height << endl;
        break;
    case 0x00101030:
        cmdstrm >> lc.patient.weight;
        dirty = true;
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.weight << endl;
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    if(dirty) strcpy_s(lc.patient.patientID, lc.file.patientID);
    return 0;
}


int cmd_study(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog)
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
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyDate << endl;
        break;
    case 0x00080030:
        cmdstrm >> lc.study.studyTime;
        dirty = true;
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyTime << endl;
        break;
    case 0x00080050:
        cmdstrm >> temp;
        dirty = true;
        x_www_form_codec<char>::decode(temp.c_str(), lc.study.accessionNumber, sizeof(lc.study.accessionNumber));
        if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    if(dirty)
        strcpy_s(lc.study.studyUID, lc.file.studyUID);
    return 0;
}

int cmd_series(const std::string &type, std::istringstream &cmdstrm, handle_context::CMOVE_NOTIFY_CONTEXT &lc, std::ostream &flog)
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
            time_header_out(flog) << "Unexpected empty modality " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            dirty = true;
            if(opt_verbose) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.series.modality << endl;
        }
        break;
    default:
        {
            char otherbuf[1024] = "";
            cmdstrm.getline(otherbuf, sizeof(otherbuf));
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
    if(dirty) strcpy_s(lc.series.seriesUID, lc.file.seriesUID);
    return 0;
}

void save_notify_context_to_ostream(const CMOVE_NOTIFY_CONTEXT &cnc, ostream &output)
{
    output << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_COMPRESS_OK << " " << cnc.src_notify_filename << endl;
    output << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << cnc.file_seq
        << " " << cnc.file.filename << " " << cnc.file.unique_filename << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00100020 ";
    x_www_form_codec<ostream>::encode(cnc.file.patientID, &output);
    output << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 0020000D " << cnc.file.studyUID << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 0020000E " << cnc.file.seriesUID << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00080018 " << cnc.file.instanceUID << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00020010 " << cnc.file.xfer << " " << cnc.file.isEncapsulated << " " << cnc.file.xfer_new << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00100010 ";
    x_www_form_codec<ostream>::encode(cnc.patient.patientsName, &output);
    output << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00100030 " << cnc.patient.birthday << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00100040 " << cnc.patient.sex << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00101020 " << cnc.patient.height << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00101030 " << cnc.patient.weight << endl;
    output << NOTIFY_LEVEL_STUDY << " 00080020 " << cnc.study.studyDate << endl;
    output << NOTIFY_LEVEL_STUDY << " 00080030 " << cnc.study.studyTime << endl;
    output << NOTIFY_LEVEL_STUDY << " 00080050 ";
    x_www_form_codec<ostream>::encode(cnc.study.accessionNumber, &output);
    output << endl;
    output << NOTIFY_LEVEL_SERIES << " 00080060 " << cnc.series.modality << endl;
}

void send_all_compress_ok_notify(const string &association_base, ostream &flog)
{
    char notify_file_name[MAX_PATH];
    string prefix(association_base);
    prefix.append("\\state\\");
    size_t pos = in_process_sequence_dll(notify_file_name, sizeof(notify_file_name), prefix.c_str());
    sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out, _SH_DENYRW);
    if(ntf.good())
    {
        ntf << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << endl;
        ntf.close();
        if(opt_verbose) time_header_out(flog) << "watch_notify() send all compress OK notify " << notify_file_name << " OK." << endl;
    }
    else
    {
        time_header_out(flog) << "watch_notify() send all compress OK notify " << notify_file_name << " failed:" << endl;
        flog << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << endl;
    }
}