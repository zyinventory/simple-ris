#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"
#include <sys/timeb.h>
#include <process.h>

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofstack.h"
#include "dcmtk/ofstd/ofstd.h"

#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcdeftag.h"

static _timeb current_sequence_val = {0LL, 0, 0, 0};
static __time64_t current_sequence_val_diff;

size_t in_process_sequence(char *buff, size_t buff_size, const char *prefix)
{
    if(strcpy_s(buff, buff_size, prefix)) return 0;
    size_t buff_used = strlen(buff);

    struct _timeb storeTimeThis;
    struct tm localtime;
    _ftime_s(&storeTimeThis);
    if(storeTimeThis.time < current_sequence_val.time || (storeTimeThis.time == current_sequence_val.time && storeTimeThis.millitm <= current_sequence_val.millitm))
    {
        if(current_sequence_val.millitm == 999)
        {
            ++current_sequence_val.time;
            current_sequence_val.millitm = 0;
        }
        else
            ++current_sequence_val.millitm;

        current_sequence_val_diff = (current_sequence_val.time - storeTimeThis.time) * 1000 + current_sequence_val.millitm - storeTimeThis.millitm;
        storeTimeThis = current_sequence_val;
    }
    else
    {
        current_sequence_val = storeTimeThis;
        current_sequence_val_diff = 0;
    }

    localtime_s(&localtime, &storeTimeThis.time);
    size_t sf_size = strftime(buff + buff_used, buff_size - buff_used, "%Y%m%d%H%M%S", &localtime);
    if(sf_size == 0) return 0;
    buff_used += sf_size;
    int sp_size = sprintf_s(buff + buff_used, buff_size - buff_used, ".%03hd-%lld-%x", 
        storeTimeThis.millitm, current_sequence_val_diff, _getpid());
    if(sp_size == -1) return 0;
    buff_used += sp_size;
    return buff_used;
}

#define MAX_PATH    260

static OFList<OFString>  patients, studies, series;
static size_t            instances = 0x00011000;

void datasetToNotify(const char* fileName, DcmDataset **imageDataSet)
{
    OFString patientID, studyUID, seriesUID;
    std::stringstream strmbuf;
    strmbuf << "F " << hex << setw(8) << setfill('0') << uppercase << instances << " " << fileName << endl;
    (*imageDataSet)->briefToStream(strmbuf, 'I');

    (*imageDataSet)->findAndGetOFString(DCM_PatientID, patientID);
    if(patients.end() == find(patients.begin(), patients.end(), patientID))
    {
        (*imageDataSet)->briefToStream(strmbuf, 'P');
        patients.push_back(patientID);
    }
    (*imageDataSet)->findAndGetOFString(DCM_StudyInstanceUID, studyUID);
    if(studies.end() == find(studies.begin(), studies.end(), studyUID))
    {
        (*imageDataSet)->briefToStream(strmbuf, 'S');
        studies.push_back(studyUID);
    }
    (*imageDataSet)->findAndGetOFString(DCM_SeriesInstanceUID, seriesUID);
    if(series.end() == find(series.begin(), series.end(), seriesUID))
    {
        (*imageDataSet)->briefToStream(strmbuf, 'E');
        series.push_back(seriesUID);
    }
    strmbuf << "F " << hex << setw(8) << setfill('0') << uppercase << instances << endl;
    ++instances;
    OFString sw = strmbuf.str();

    char filename[MAX_PATH];
    size_t used = in_process_sequence(filename, sizeof(filename), STATE_DIR);
    if(used > 0 && 0 == strcpy_s(filename + used, sizeof(filename) - used, "_F.dfc"))
    {
        FILE *fplog = fopen(filename, "a");
        if(fplog != NULL)
        {
            fwrite(sw.c_str(), 1, sw.length(), fplog); fflush(fplog);
            fclose(fplog);
        }
    }
    else
    {
        cerr << "can't create sequence file name, missing command: " << sw.c_str() << endl;
    }

    sw.clear();
    strmbuf.str(sw);
}
