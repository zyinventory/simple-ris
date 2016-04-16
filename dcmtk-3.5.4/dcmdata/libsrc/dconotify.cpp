#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofstack.h"
#include "dcmtk/ofstd/ofstd.h"

#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcdeftag.h"

static OFList<OFString>  patients, studies, series;
static size_t            instances = NOTIFY_FILE_SEQ_START;

namespace
{
    class numpunct_no_gouping : public std::numpunct_byname<char>
    {
    public:
        numpunct_no_gouping(const char* name) : std::numpunct_byname<char>(name){ };
    protected:
        virtual std::string do_grouping() const { return ""; } // no grouping
    };
}

void datasetToNotify(const char* instanceFileName, const char *notifyFileName, DcmDataset **imageDataSet, bool isFull)
{
    OFString patientID, studyUID, seriesUID;
    std::ostringstream strmbuf;
    std::locale loc_nnp(std::locale::locale(""), ::new numpunct_no_gouping("")); // force ::new, otherwise will encounter DEBUG_NEW bug
    strmbuf.imbue(loc_nnp);

    strmbuf << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << instances << " " << instanceFileName << endl;
    if(isFull)
        (*imageDataSet)->briefToStream(strmbuf, NOTIFY_LEVEL_FULL);
    else
    {
        (*imageDataSet)->briefToStream(strmbuf, NOTIFY_LEVEL_INSTANCE);

        (*imageDataSet)->findAndGetOFString(DCM_PatientID, patientID);
        if(patients.end() == find(patients.begin(), patients.end(), patientID))
        {
            (*imageDataSet)->briefToStream(strmbuf, NOTIFY_LEVEL_PATIENT);
            patients.push_back(patientID);
        }
        (*imageDataSet)->findAndGetOFString(DCM_StudyInstanceUID, studyUID);
        if(studies.end() == find(studies.begin(), studies.end(), studyUID))
        {
            (*imageDataSet)->briefToStream(strmbuf, NOTIFY_LEVEL_STUDY);
            studies.push_back(studyUID);
        }
        (*imageDataSet)->findAndGetOFString(DCM_SeriesInstanceUID, seriesUID);
        if(series.end() == find(series.begin(), series.end(), seriesUID))
        {
            (*imageDataSet)->briefToStream(strmbuf, NOTIFY_LEVEL_SERIES);
            series.push_back(seriesUID);
        }
    }
    strmbuf << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << instances << endl;
    ++instances;
    OFString sw = strmbuf.str();

    FILE *fplog = fopen(notifyFileName, "w");
    if(fplog != NULL)
    {
        fwrite(sw.c_str(), 1, sw.length(), fplog); fflush(fplog);
        fclose(fplog);
    }
    else
    {
        cerr << "can't create notify file " << notifyFileName << ", missing command:" << endl << sw.c_str() << endl;
    }

    sw.clear();
    strmbuf.str(sw);
}