#include <string>
#include <windows.h>
#include "commonlib.h"
#include "../include/dcmtk/dcmdata/notify_context.h"

using namespace std;
using namespace handle_context;

const char* _tag_NOTIFY_FILE_CONTEXT_FILE_SECTION::StorePath(char sp)
{
    HashStr(studyUID, unique_filename, sizeof(unique_filename));
    unique_filename[8] = sp;
    SeriesInstancePath(seriesUID, instanceUID, unique_filename + 9, sizeof(unique_filename) - 9, sp);
    sprintf_s(hash, "%c%c%c%c%c%c%c%c%c%c%c",
        unique_filename[0], unique_filename[1], sp, unique_filename[2], unique_filename[3], sp, 
        unique_filename[4], unique_filename[5], sp, unique_filename[6], unique_filename[7]);
    return unique_filename;
}

bool _tag_NOTIFY_FILE_CONTEXT::operator<(const struct _tag_NOTIFY_FILE_CONTEXT &r) const
{
    int cmp = strcmp(src_notify_filename, r.src_notify_filename);
    if(cmp < 0) return true;
    else if(cmp > 0) return false;
    else
    {
        cmp = strcmp(file.unique_filename, r.file.unique_filename);
        if(cmp < 0) return true;
        else if(cmp > 0) return false;
        else
        {
            cmp = strcmp(file.filename, r.file.filename);
            if(cmp < 0) return true;
            else return false;
        }
    }
}
