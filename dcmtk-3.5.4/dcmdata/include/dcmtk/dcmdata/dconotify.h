#ifndef DCONOTIFY_H
#define DCONOTIFY_H

#include <string>
#include <list>
#include "notify_context.h"

class DcmDataset;

unsigned int in_process_sequence(char *buff, size_t buff_size, const char *prefix);
bool mkdir_recursive_dcm(const char *subdir);

class DatasetNotifyWriter
{
private:
    std::list<std::string>  patients, studies, series;
    std::string currentStudyUID;
    size_t            instances;

public:
    DatasetNotifyWriter() : instances(NOTIFY_FILE_SEQ_START) {};
    void datasetToNotify(const char* instanceFileName, const char *notifyFileName, DcmDataset **imageDataSet,
        handle_context::NOTIFY_FILE_CONTEXT_FILE_SECTION *pnfc, bool isFull);
};

#endif // DCONOTIFY_H