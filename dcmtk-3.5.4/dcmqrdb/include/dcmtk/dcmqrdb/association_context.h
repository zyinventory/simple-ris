#ifndef DCMQR_ASSOC_CONTEXT
#define DCMQR_ASSOC_CONTEXT

class DcmQueryRetrieveStoreContext;
class DcmQueryRetrieveConfig;

typedef OFCondition(*IndexCallback)(const char*, const char*, DcmDataset*);

typedef struct {
    const DcmQueryRetrieveConfig *pConfig;
    IndexCallback cbToDcmQueryRetrieveStoreContext;
    char storageArea[DUL_LEN_NODE + 1];
    char associationId[40];
    char callingAPTitle[DUL_LEN_TITLE + 1];
    char calledAPTitle[DUL_LEN_TITLE + 1];
    DIC_NODENAME remoteHostName, localHostName;
    unsigned short port;
} ASSOCIATION_CONTEXT;

#endif //DCMQR_ASSOC_CONTEXT
