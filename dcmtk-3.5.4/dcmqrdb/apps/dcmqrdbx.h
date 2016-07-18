#pragma once
#ifndef DCMQRDBX_H
#define DCMQRDBX_H

#include <string>
#include <set>
#include <atlbase.h>
#import  <msxml3.dll>

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmqrdb/dcmqrdba.h"    /* for class DcmQueryRetrieveDatabaseHandle */
#include "dcmtk/dcmqrdb/dcmqrcnf.h"
#include "dcmtk/dcmqrdb/dcmqridx.h"

#include "dcmtk/dcmnet/dicom.h"
#include "dcmtk/dcmnet/dimse.h"
#include "commonlib.h"

const OFConditionConst DcmQRXmlDatabaseErrorC(OFM_imagectn, 0x002, OF_error, "DcmQR Xml Database Error");

class StudyDataFilter {
public:
    char low[16], high[16];
    const char *src;
    StudyDataFilter()
    {
        src = NULL;
        strcpy_s(low, "");
        strcpy_s(high, "99999999");
    };
    StudyDataFilter(const char *date);
    StudyDataFilter(const StudyDataFilter &r)
    {
        *this = r;
    };
    StudyDataFilter &operator=(const StudyDataFilter &r)
    {
        src = r.src;
        strcpy_s(low, r.low);
        strcpy_s(high, r.high);
        return *this;
    };
    bool inRange(const char *date) const
    {
        if(date == NULL || strlen(date) == 0) return true;
        if(strcmp(low, date) > 0) return false;
        if(strcmp(date, high) > 0) return false;
        return true;
    };
    void testRange(const char *t) const
    {
        if(inRange(t))
            CERR << (t ? t : "(NULL)") << " in range" << (src ? src : "(NULL)") << endl;
        else
            CERR << (t ? t : "(NULL)") << " NOT in range" << (src ? src : "(NULL)") << endl;
    };
};

class DcmQueryRetrieveXmlDatabaseHandle: public DcmQueryRetrieveDatabaseHandle
{
private:
    int debugLevel;
    OFBool doCheckFindIdentifier, doCheckMoveIdentifier;
    char storageArea[DUL_LEN_NODE+1];
    DB_LEVEL rootLevel, lowestLevel; // highest and lowest legal level for a query in the current model
    DB_LEVEL queryLevel;             // queryLevel belongs to [rootLevel, lowestLevel]
    
    MSXML2::IXMLDOMDocument2Ptr pXmlDom;
    MSXML2::IXMLDOMNodeListPtr ptl, stl, sel, inl;
    MSXML2::IXMLDOMElementPtr pt, st, se;
    DcmDataset ds, *req;

    bool fillNextElementToDcmdataset();

    size_t findRequestFilter(DcmDataset *findRequestIdentifiers);
    bool add_study(MSXML2::IXMLDOMElementPtr &pStudy, const set<string> &pids, const StudyDataFilter &date_filter);
    size_t findByStudyUIDs(const set<string> &uids, const set<string> &pids, const StudyDataFilter &date_filter);
    size_t findByPatientIDs(const set<string> &pids, const StudyDataFilter &date_filter);
    size_t findByStudyDate(const StudyDataFilter &date_filter);

    static size_t extractMultiValues(char *ids, set<string> &id_set, char *seps = "\\");
    static OFCondition testFindRequestList(DcmDataset *findRequestIdentifiers,
        DB_LEVEL queryLevel, DB_LEVEL infLevel, DB_LEVEL lowestLevel);
public:
    DcmQueryRetrieveXmlDatabaseHandle(const char *storage, long maxStudiesPerStorageArea,
        long maxBytesPerStudy, OFCondition& result);
    
    const char *getStorageArea() const { return storageArea; };

    OFCondition storeRequest(const char *SOPClassUID, const char *SOPInstanceUID, const char *imageFileName,
        DcmQueryRetrieveDatabaseStatus  *status, OFBool isNew = OFTrue, DcmQueryRetrieveStoreContext *psc = NULL);
    OFCondition startFindRequest(const char *SOPClassUID, DcmDataset *findRequestIdentifiers,
        DcmQueryRetrieveDatabaseStatus *status);
    OFCondition nextFindResponse(DcmDataset **findResponseIdentifiers,
        DcmQueryRetrieveDatabaseStatus *status);
    OFCondition cancelFindRequest(DcmQueryRetrieveDatabaseStatus *status);
    OFCondition startMoveRequest(const char *SOPClassUID, DcmDataset *moveRequestIdentifiers,
        DcmQueryRetrieveDatabaseStatus *status);
    OFCondition nextMoveResponse(char *SOPClassUID, char *SOPInstanceUID, char *imageFileName,
        unsigned short *numberOfRemainingSubOperations, DcmQueryRetrieveDatabaseStatus *status);
    OFCondition cancelMoveRequest(DcmQueryRetrieveDatabaseStatus *status);
    OFCondition pruneInvalidRecords();
    void setDebugLevel(int dbgLv) { debugLevel = dbgLv; };
    void setIdentifierChecking(OFBool checkFind, OFBool checkMove) { doCheckFindIdentifier = checkFind; doCheckMoveIdentifier = checkMove; };
};

class DcmQueryRetrieveXmlDatabaseHandleFactory: public DcmQueryRetrieveDatabaseHandleFactory
{
private:
    /// pointer to system configuration
    const DcmQueryRetrieveConfig *config_;

public:
    /** constructor
    *  @param config system configuration object, must not be NULL.
    */
    DcmQueryRetrieveXmlDatabaseHandleFactory(const DcmQueryRetrieveConfig *config)
        : config_(config) { };

    /// destructor
    virtual ~DcmQueryRetrieveXmlDatabaseHandleFactory() { };

    /** this method creates a new database handle instance on the heap and returns
    *  a pointer to it, along with a result that indicates if the instance was
    *  successfully initialized, i.e. connected to the database
    *  @param callingAETitle calling aetitle
    *  @param calledAETitle called aetitle
    *  @param result result returned in this variable
    *  @return pointer to database object, must not be NULL if result is EC_Normal.
    */
    virtual DcmQueryRetrieveDatabaseHandle *createDBHandle(
        const char *callingAETitle, 
        const char *calledAETitle,
        OFCondition& result) const
    {
        return new DcmQueryRetrieveXmlDatabaseHandle(
            config_->getStorageArea(calledAETitle),
            config_->getMaxStudies(calledAETitle),
            config_->getMaxBytesPerStudy(calledAETitle), result);
    };
};

#endif
