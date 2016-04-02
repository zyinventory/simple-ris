#ifndef DCONOTIFY_H
#define DCONOTIFY_H

#define NOTIFY_ASSOC_ESTA       0x00010010
#define NOTIFY_ASSOC_RELEASE    0xFFFFFFFF
#define NOTIFY_ASSOC_ABORT      0xFFFFFFFD
#define NOTIFY_ACKI_MV_STAT     0xFFFFFFFB
#define NOTIFY_OPER_CANCEL      0xFFFFFFFC
#define NOTIFY_MV_STAT_REMAIN   0x00001020
#define NOTIFY_MV_STAT_COMPLETE 0x00001021
#define NOTIFY_MV_STAT_FAILED   0x00001022
#define NOTIFY_MV_STAT_WARNING  0x00001023

#define INT_TO_CHAR4(pi) reinterpret_cast<const char*>(pi)
#define CHAR4_TO_INT(pc) (*reinterpret_cast<const int*>(pc))
#define STRING_PRE4_TO_INT(pc) (*reinterpret_cast<const int*>(pc.c_str()))

#define NOTIFY_STORE_TAG        "STOR"
#define NOTIFY_MOVE_TAG         "MOVE"
#define NOTIFY_FILE_TAG         "FILE"
#define NOTIFY_FILE_SEQ_START   0x00011000
#define NOTIFY_ACKN_TAG         "ACKN"
#define NOTIFY_ACKN_ITEM        "ACKI"

#define NOTIFY_LEVEL_FULL       "FULL"
#define NOTIFY_LEVEL_INSTANCE   "INST"
#define NOTIFY_LEVEL_PATIENT    "PATI"
#define NOTIFY_LEVEL_STUDY      "STUD"
#define NOTIFY_LEVEL_SERIES     "SERI"

#define NOTIFY_ALL_COMPRESS_OK  0xFFFFFFFA
#define NOTIFY_ARCHIVE_STUDY    0xFFFFFFF9
#define NOTIFY_ARCHIVE_DICOMDIR 0xFFFFFFF8
#define NOTIFY_XML_OK           0xFFFFFFF0

#define STATE_DIR_NO_SP "state"
#define STATE_DIR       "state\\"

class DcmDataset;

unsigned int in_process_sequence(char *buff, size_t buff_size, const char *prefix);
void datasetToNotify(const char* fileName, DcmDataset **imageDataSet, bool isFull = false);

#endif // DCONOTIFY_H