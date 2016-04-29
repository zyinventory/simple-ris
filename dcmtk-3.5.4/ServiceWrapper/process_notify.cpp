#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"
#include "../ofstd/include/dcmtk/ofstd/x_www_form_codec.h"

using namespace std;

static char linebuff[1024];

void process_file_notify_file(std::ifstream &ifs, unsigned int file_tag, const string &transfer_base)
{
    DWORD flag = 0;
    char *filename;
    ifs.getline(linebuff, sizeof(linebuff));
    filename = trim(linebuff, ifs.gcount());
    
    handle_context::CMOVE_FILE_SECTION cfs;
    memset(&cfs, 0, sizeof(cfs));
    cfs.tag = file_tag;
    sprintf_s(cfs.filename, "%s\\%s", transfer_base.c_str(), filename);

    do{
        ifs.getline(linebuff, 1024);
        if(ifs.gcount())
        {
            istringstream cmdstrm(linebuff);
            unsigned int tag = 0;
            string type;
            cmdstrm >> type;
            if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_INSTANCE))
            {
                cmdstrm >> hex >> cfs.tag;
                switch(tag)
                {
                case 0x0020000D:
                    cmdstrm >> cfs.studyUID;
                    flag |= 0x1;
                    break;
                case 0x0020000E:
                    cmdstrm >> cfs.seriesUID;
                    flag |= 0x2;
                    break;
                case 0x00080018:
                    cmdstrm >> cfs.instanceUID;
                    flag |= 0x4;
                    break;
                default:
                    break;
                }
            }
            // else ignore
            if(flag == 0x7) break;
        }
    } while(ifs.good());
    
    if(strlen(cfs.studyUID) == 0)
    {
        time_header_out(cerr) << "Unexpected empty studyUID " << cfs.studyUID << endl;
        return;
    }
    if(strlen(cfs.seriesUID) == 0)
    {
        time_header_out(cerr) << "Unexpected empty seriesUID " << cfs.seriesUID << endl;
        return;
    }
    if(strlen(cfs.instanceUID) == 0)
    {
        time_header_out(cerr) << "Unexpected empty instanceUID " << cfs.instanceUID << endl;
        return;
    }
    cfs.StorePath('\\');
    
}
