#include "stdafx.h"
#include "../ofstd/include/dcmtk/ofstd/x_www_form_codec.h"

using namespace std;
using namespace handle_context;

static char linebuff[1024];

int x_www_form_codec_encode_to_ostream(const char *str, ostream *output)
{
    return x_www_form_codec<ostream>::encode(str, output);
}

int cmd_instance(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog)
{
    unsigned int tag = 0;
    string temp;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00080005:
        if(cmdstrm.getline(lc.file.charset, sizeof(lc.file.charset)).fail())
            strcpy_s(lc.file.charset, "ISO_IR 100");
        else
        {
            temp = lc.file.charset;
            STRING_TRIM(temp);
            strcpy_s(lc.file.charset, temp.c_str());
        }
        if(strlen(lc.file.charset) == 0)
            time_header_out(flog) << "Unexpected empty charset " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(debug_mode)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.charset << endl;
        break;
    case 0x00100020:
        cmdstrm >> temp;
        if(temp.empty())
            time_header_out(flog) << "Unexpected empty patientID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
            x_www_form_codec<char>::decode(temp.c_str(), lc.file.patientID, sizeof(lc.file.patientID));
        }
        break;
    case 0x0020000D:
        cmdstrm >> lc.file.studyUID;
        if(strlen(lc.file.studyUID) == 0)
            time_header_out(flog) << "Unexpected empty studyUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(debug_mode)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.studyUID << endl;
        break;
    case 0x0020000E:
        cmdstrm >> lc.file.seriesUID;
        if(strlen(lc.file.seriesUID) == 0)
            time_header_out(flog) << "Unexpected empty seriesUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(debug_mode)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.seriesUID << endl;
        break;
    case 0x00200013:
        cmdstrm >> dec >> lc.file.number;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << dec << lc.file.number << endl;
        break;
    case 0x00080018:
        cmdstrm >> lc.file.instanceUID;
        if(strlen(lc.file.instanceUID) == 0)
            time_header_out(flog) << "Unexpected empty instanceUID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(debug_mode)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.instanceUID << endl;
        break;
    case 0x00020010:
        cmdstrm >> lc.file.xfer >> lc.file.isEncapsulated;
        if(strlen(lc.file.xfer) == 0)
            time_header_out(flog) << "Unexpected empty xfer " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(debug_mode)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.xfer << " " << lc.file.isEncapsulated << endl;
        break;
    case 0x00080016:
        cmdstrm >> lc.file.sopClassUID;
        if(strlen(lc.file.sopClassUID) == 0)
            time_header_out(flog) << "Unexpected empty SOP Class UID " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else if(debug_mode)
            time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.file.sopClassUID << endl;
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

int cmd_patient(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog)
{
    unsigned int tag = 0;
    string temp;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00100010:
        cmdstrm >> temp;
        dirty = true;
        if(temp.empty())
            time_header_out(flog) << "Unexpected empty patient's name " << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << endl;
        else
        {
            x_www_form_codec<char>::decode(temp.c_str(), lc.patient.patientsName, sizeof(lc.patient.patientsName));
            if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
        }
        break;
    case 0x00100030:
        cmdstrm >> temp;
        normalize_dicom_date(sizeof(lc.patient.birthday), lc.patient.birthday, temp.c_str());
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.birthday << endl;
        break;
    case 0x00100040:
        cmdstrm >> lc.patient.sex;
        if(lc.patient.sex[0] != 'M' && lc.patient.sex[0] != 'F' && lc.patient.sex[0] != 'O')
        {
            lc.patient.sex[0] = ' ';
            lc.patient.sex[1] = '\0';
        }
        else if(strlen(lc.patient.patientID) == 0) dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.sex << endl;
        break;
    case 0x00101020:
        cmdstrm >> lc.patient.height;
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.height << endl;
        break;
    case 0x00101030:
        cmdstrm >> lc.patient.weight;
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.patient.weight << endl;
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


int cmd_study(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog)
{
    unsigned int tag = 0;
    string temp;
    bool dirty = false;

    cmdstrm >> hex >> tag;
    switch(tag)
    {
    case 0x00080020:
        cmdstrm >> temp;
        normalize_dicom_date(sizeof(lc.study.studyDate), lc.study.studyDate, temp.c_str());
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyDate << endl;
        break;
    case 0x00080030:
        cmdstrm >> lc.study.studyTime;
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyTime << endl;
        break;
    case 0x00080050:
        cmdstrm >> temp;
        dirty = true;
        x_www_form_codec<char>::decode(temp.c_str(), lc.study.accessionNumber, sizeof(lc.study.accessionNumber));
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << temp << endl;
        break;
    case 0x00200010:
        cmdstrm >> lc.study.studyID;
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.study.studyID << endl;
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

int cmd_series(const std::string &type, std::istringstream &cmdstrm, handle_context::NOTIFY_FILE_CONTEXT &lc, std::ostream &flog)
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
            if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.series.modality << endl;
        }
        break;
    case 0x00200011:
        cmdstrm >> dec >> lc.series.number;
        dirty = true;
        if(debug_mode) time_header_out(flog) << type << " " << hex << uppercase << setw(8) << setfill('0') << tag << " " << lc.series.number << endl;
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

void save_notify_context_to_ostream(const NOTIFY_FILE_CONTEXT &cnc, bool compress_ok, ostream &output)
{
    output << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase 
        << (compress_ok ? NOTIFY_COMPRESS_OK : NOTIFY_COMPRESS_FAIL) << " " << cnc.src_notify_filename << endl;
    output << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << cnc.file_seq
        << " " << cnc.file.filename << " " << cnc.file.unique_filename << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00080005 " << cnc.file.charset << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00100020 ";
    x_www_form_codec<ostream>::encode(cnc.file.patientID, &output);
    output << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 0020000D " << cnc.file.studyUID << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 0020000E " << cnc.file.seriesUID << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00080018 " << cnc.file.instanceUID << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00020010 " << cnc.file.xfer << " " << cnc.file.isEncapsulated << " " << cnc.file.xfer_new << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00200013 " << dec << cnc.file.number << endl;
    output << NOTIFY_LEVEL_INSTANCE << " 00080016 " << cnc.file.sopClassUID << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00100010 ";
    if(strcmp("ISO_IR 192", cnc.file.charset) == 0)
    {
        size_t utf8buf_len = (strlen(cnc.patient.patientsName) + 1) * 2;
        char *utf8buf = new char[utf8buf_len];
        if(GBKToUTF8(cnc.patient.patientsName, utf8buf, utf8buf_len))
            x_www_form_codec<ostream>::encode(utf8buf, &output);
        else
            x_www_form_codec<ostream>::encode(cnc.patient.patientsName, &output);
        if(utf8buf) delete[] utf8buf;
    }
    else
        x_www_form_codec<ostream>::encode(cnc.patient.patientsName, &output);
    output << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00100030 " << cnc.patient.birthday << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00100040 " << cnc.patient.sex << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00101020 " << cnc.patient.height << endl;
    output << NOTIFY_LEVEL_PATIENT << " 00101030 " << cnc.patient.weight << endl;
    output << NOTIFY_LEVEL_STUDY << " 00200010 " << dec << cnc.study.studyID << endl;
    output << NOTIFY_LEVEL_STUDY << " 00080020 " << cnc.study.studyDate << endl;
    output << NOTIFY_LEVEL_STUDY << " 00080030 " << cnc.study.studyTime << endl;
    output << NOTIFY_LEVEL_STUDY << " 00080050 ";
    x_www_form_codec<ostream>::encode(cnc.study.accessionNumber, &output);
    output << endl;
    output << NOTIFY_LEVEL_SERIES << " 00080060 " << cnc.series.modality << endl;
    output << NOTIFY_LEVEL_SERIES << " 00200011 " << dec << cnc.series.number << endl;
}

static bool process_notify_file(std::istream &ifs, unsigned int seq, NOTIFY_FILE_CONTEXT *pclc, ostream &flog)
{
    if(seq >= NOTIFY_FILE_SEQ_START)
    {
        pclc->file_seq = seq;
        ifs >> pclc->file.filename;
        if(opt_verbose) time_header_out(flog) << NOTIFY_FILE_TAG << " " << hex << uppercase << setw(8) << setfill('0') << seq 
            << " " << pclc->file.filename << endl;
    }
    else
    {
        time_header_out(flog) << "process_notify_file() seq is error: " 
            << NOTIFY_FILE_TAG << " " << hex << uppercase << setw(8) << setfill('0') << seq 
            << " " << pclc->file.filename << endl;
        return false;
    }

    char *cmd = new char[1024];
    do{
        ifs.getline(cmd, 1024);
        if(ifs.gcount() && strlen(cmd))
        {
            istringstream linestrm(cmd);
            string type;
            linestrm >> type;
            if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_INSTANCE))
                cmd_instance(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_PATIENT))
                cmd_patient(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_STUDY))
                cmd_study(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_SERIES))
                cmd_series(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_FILE_TAG))
                ; // ignore it
            else 
                time_header_out(flog) << "handle_dir::process_notify_file() can't recognize line: " << linestrm.str() << endl;
        }
    } while(ifs.good());
    if(strcmp(pclc->file.charset, "ISO_IR 192") == 0)
    {
        char pn[128];
        if(UTF8ToGBK(pclc->patient.patientsName, pn, sizeof(pn)))
            strcpy_s(pclc->patient.patientsName, pn);
        else
            time_header_out(flog) << __FUNCSIG__" convert from utf-8 fail: " << pclc->patient.patientsName << endl;
    }
    if(cmd) delete[] cmd;
    return true;
}

DWORD process_notify(const file_notify* pt, NOTIFY_FILE_CONTEXT *pnfc, std::ostream &flog)
{
    // todo: find association
    //if(get<4>(*pt)) get<4>(*pt)->fill_association(pnfc);

    DWORD gle = 0, tag;
    string cmd, filepath(GetPacsTemp());
    filepath.append("\\pacs\\").append(pt->get_path()).append("\\state\\").append(pt->get_notify_filename());
    if(opt_verbose) time_header_out(flog) << "process_notify(): " << filepath << endl;

    ifstream ifs(filepath, ios_base::in, _SH_DENYWR);
    if(ifs.fail())
    {
        gle = GetLastError();
        if(gle != ERROR_SHARING_VIOLATION || opt_verbose)
        {
            string msg("process_notify() open file failed: ");
            msg.append(filepath);
            return displayErrorToCerr(msg.c_str(), gle, &flog);
        }
        else return gle;
    }

    ifs >> cmd >> hex >> tag;
    
    if(cmd.compare(NOTIFY_FILE_TAG) == 0) // receive a file
    {
        if(process_notify_file(ifs, tag, pnfc, flog))
        {
            strcpy_s(pnfc->src_notify_filename, pt->get_notify_filename().c_str());
            NotifyFileContextStorePath(&pnfc->file);
            if(opt_verbose) time_header_out(flog) << "process_notify(" << pt->get_path() << "\\" << pt->get_notify_filename() << ") read OK." << endl;
#ifdef _DEBUG
            time_header_out(cerr) << "process_notify(" << pt->get_path() << "\\" << pt->get_notify_filename() << ") read OK." << endl;
#endif
        }
        else time_header_out(flog) << "process_notify() can't process " << pt->get_path() << "\\" << pt->get_notify_filename() << ", ignore." << endl;
    }
    else if(cmd.compare(NOTIFY_STORE_TAG) == 0)
    {
        // todo: find association
        //if(get<4>(*pt)) get<4>(*pt)->fill_association(pnfc);
    }
    else if(cmd.compare(NOTIFY_ACKN_ITEM) == 0)
    {
        if(tag == NOTIFY_COMPRESS_OK || tag == NOTIFY_COMPRESS_FAIL)
        {
            string src_notify_file;
            ifs >> src_notify_file;
            if(opt_verbose) time_header_out(flog) << "process_notify() recieve compress complete notify " << pt->get_notify_filename() << "(" << src_notify_file << ")." << endl;
        }
        else if(tag == NOTIFY_ALL_COMPRESS_OK)
        {
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify() recieve all compress complete notify " << pt->get_notify_filename() << endl;
        }
        else time_header_out(flog) << "handle_dir::process_notify() ignore ack file " << pt->get_notify_filename() << endl
            << cmd << " " << hex << uppercase << tag << " ..." << endl;
    }
    else time_header_out(flog) << "handle_dir::process_notify() ignore " << pt->get_notify_filename() << endl;

    if(ifs.is_open()) ifs.close();

    return gle;
}
