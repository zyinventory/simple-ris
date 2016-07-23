#include <map>
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CCTYPE
#define INCLUDE_CSTDARG
#include "dcmtk/ofstd/ofstdinc.h"

#include "dcmtk/dcmqrdb/dcmqrdbs.h"
#include "dcmtk/dcmqrdb/dcmqrcbs.h"
#include "dcmqrdbx.h"

#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/ofstd/ofstd.h"

/* ========================= static data ========================= */

/**** The TbFindAttr table contains the description of tags (keys) supported
 **** by the DB Module.
 **** Tags described here have to be present in the Index Record file.
 **** The order is unsignificant.
 ****
 **** Each element of this table is described by
 ****           The tag value
 ****           The level of this tag (from patient to image)
 ****           The Key Type (only UNIQUE_KEY values is used)
 ****           The key matching type, specifiing which type of
 ****                   matching should be performed. The OTHER_CLASS
 ****                   value specifies that only strict comparison is applied.
 ****
 **** This table and the IndexRecord structure should contain at least
 **** all Unique and Required keys.
 ***/

static const std::pair<wstring, DB_FindAttr> patientRootFindAttr[] = {
    std::make_pair(L"id",               DB_FindAttr( DCM_PatientID,                             PATIENT_LEVEL,  UNIQUE_KEY,     UID_CLASS       )),
    std::make_pair(L"study_count",      DB_FindAttr( DCM_NumberOfPatientRelatedStudies,         PATIENT_LEVEL,  OPTIONAL_KEY,   OTHER_CLASS     )),
    std::make_pair(L"series_count",     DB_FindAttr( DCM_NumberOfPatientRelatedSeries,          PATIENT_LEVEL,  OPTIONAL_KEY,   OTHER_CLASS     )),
    std::make_pair(L"instance_count",   DB_FindAttr( DCM_NumberOfPatientRelatedInstances,       PATIENT_LEVEL,  OPTIONAL_KEY,   OTHER_CLASS     ))
};
static const int patientRootFindAttrNum = sizeof(patientRootFindAttr) / sizeof(std::pair<OFString, DB_FindAttr>);
static const std::pair<wstring, DB_FindAttr> patientFindAttr[] = {
    std::make_pair(L"name",             DB_FindAttr( DCM_PatientsName,                          PATIENT_LEVEL,  REQUIRED_KEY,   STRING_CLASS    )),
    std::make_pair(L"birthday",         DB_FindAttr( DCM_PatientsBirthDate,                     PATIENT_LEVEL,  OPTIONAL_KEY,   DATE_CLASS      )),
    std::make_pair(L"sex",              DB_FindAttr( DCM_PatientsSex,                           PATIENT_LEVEL,  OPTIONAL_KEY,   STRING_CLASS    )),
    std::make_pair(L"height",           DB_FindAttr( DCM_PatientsSize,                          PATIENT_LEVEL,  OPTIONAL_KEY,   OTHER_CLASS     )),
    std::make_pair(L"weight",           DB_FindAttr( DCM_PatientsWeight,                        PATIENT_LEVEL,  OPTIONAL_KEY,   OTHER_CLASS     )),
};
static const int patientFindAttrNum = sizeof(patientFindAttr) / sizeof(std::pair<OFString, DB_FindAttr>);
static const std::pair<wstring, DB_FindAttr> studyFindAttr[] = {
    std::make_pair(L"id",               DB_FindAttr( DCM_StudyInstanceUID,                      STUDY_LEVEL,    UNIQUE_KEY,     UID_CLASS       )),
    std::make_pair(L"date",             DB_FindAttr( DCM_StudyDate,                             STUDY_LEVEL,    REQUIRED_KEY,   DATE_CLASS      )),
    std::make_pair(L"time",             DB_FindAttr( DCM_StudyTime,                             STUDY_LEVEL,    REQUIRED_KEY,   TIME_CLASS      )),
    std::make_pair(L"study_id",         DB_FindAttr( DCM_StudyID,                               STUDY_LEVEL,    REQUIRED_KEY,   STRING_CLASS    )),
    std::make_pair(L"accession_number", DB_FindAttr( DCM_AccessionNumber,                       STUDY_LEVEL,    REQUIRED_KEY,   STRING_CLASS    )),
    std::make_pair(L"modality",         DB_FindAttr( DCM_ModalitiesInStudy,                     STUDY_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    )),
    std::make_pair(L"series_count",     DB_FindAttr( DCM_NumberOfStudyRelatedSeries,            STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     )),
    std::make_pair(L"instance_count",   DB_FindAttr( DCM_NumberOfStudyRelatedInstances,         STUDY_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ))
};
static const int studyFindAttrNum = sizeof(studyFindAttr) / sizeof(std::pair<OFString, DB_FindAttr>);
static const std::pair<wstring, DB_FindAttr> seriesFindAttr[] = {
    std::make_pair(L"id",               DB_FindAttr( DCM_SeriesInstanceUID,                     SERIE_LEVEL,    UNIQUE_KEY,     UID_CLASS       )),
    std::make_pair(L"number",           DB_FindAttr( DCM_SeriesNumber,                          SERIE_LEVEL,    REQUIRED_KEY,   OTHER_CLASS     )),
    std::make_pair(L"modality",         DB_FindAttr( DCM_Modality,                              SERIE_LEVEL,    OPTIONAL_KEY,   STRING_CLASS    )),
    std::make_pair(L"instance_count",   DB_FindAttr( DCM_NumberOfSeriesRelatedInstances,        SERIE_LEVEL,    OPTIONAL_KEY,   OTHER_CLASS     ))
};
static const int seriesFindAttrNum = sizeof(seriesFindAttr) / sizeof(std::pair<OFString, DB_FindAttr>);
static const std::pair<wstring, DB_FindAttr> instanceFindAttr[] = {
    std::make_pair(L"id",               DB_FindAttr( DCM_SOPInstanceUID,                        IMAGE_LEVEL,    UNIQUE_KEY,     UID_CLASS       )),
    std::make_pair(L"number",           DB_FindAttr( DCM_InstanceNumber,                        IMAGE_LEVEL,    REQUIRED_KEY,   OTHER_CLASS     )),
    std::make_pair(L"sop_class_uid",    DB_FindAttr( DCM_SOPClassUID,                           IMAGE_LEVEL,    OPTIONAL_KEY,   UID_CLASS       ))
};
static const int instanceFindAttrNum = sizeof(instanceFindAttr) / sizeof(std::pair<OFString, DB_FindAttr>);

/* ========================= static functions ========================= */
static void fill_dcmdataset(_variant_t &vt, DcmDataset &ds, const DB_FindAttr &dbf)
{
    if(vt.vt == VT_NULL)
    {
        if(dbf.keyAttr == REQUIRED_KEY || dbf.keyAttr == UNIQUE_KEY)
        {
            if(dbf.keyClass == UID_CLASS)
                ds.putAndInsertString(dbf.tag, "1.2.3.4.5.6.7.8.9.0");
            else if(dbf.keyClass == STRING_CLASS)
                ds.putAndInsertString(dbf.tag, "FIND_MOCK_VALUE");
            else // OTHER_CLASS
                ds.putAndInsertString(dbf.tag, "0");
        }
    }
    else
    {
        if(vt.vt != VT_BSTR) vt.ChangeType(VT_BSTR);
        ds.putAndInsertString(dbf.tag, (_bstr_t)vt);
    }
}

/* ========================= method functions ========================= */
OFCondition DcmQueryRetrieveXmlDatabaseHandle::testFindRequestList(DcmDataset *findRequestIdentifiers,
    DB_LEVEL queryLevel, DB_LEVEL infLevel, DB_LEVEL lowestLevel)
{
    int level = 0;
    // Query level must be at least the infLevel
    if (queryLevel < infLevel) {
        CERR << "Level Error: incompatible with Information Model (level " << queryLevel << ")" << endl;
        return DcmQRXmlDatabaseErrorC;
    }

    if (queryLevel > lowestLevel) {
        CERR << "Level Error: incompatible with Information Model (level " << queryLevel << ")" << endl;
        return DcmQRXmlDatabaseErrorC;
    }

    DcmElement *elem = NULL;
    findRequestIdentifiers->findAndGetElement(DCM_StudyInstanceUID, elem);
    if(elem) return EC_Normal;
    findRequestIdentifiers->findAndGetElement(DCM_PatientID, elem);
    if(elem) return EC_Normal;
    findRequestIdentifiers->findAndGetElement(DCM_StudyDate, elem);
    if(elem) return EC_Normal;

    CERR << "Unique Key Error: At least one of Patient ID, Study Instance UID or Study Date shall not be empty." << endl;
    return DcmQRXmlDatabaseErrorC;
}

size_t DcmQueryRetrieveXmlDatabaseHandle::extractMultiValues(char *ids, set<string> &id_set, char *seps)
{
    char *token = NULL, *next_token = NULL;
    size_t num = 0;
    token = strtok_s(ids, seps, &next_token);
    while(token)
    {
        if(strlen(token) && strcmp("*", token))
        {
            string str(token);
            // trim '*'
            str.erase(find_if(str.rbegin(), str.rend(), [](const char c){ return c != '*'; }).base(), str.end());
            str.erase(str.begin(), find_if(str.begin(), str.end(), [](const char c){ return c != '*'; }));
            if(str.length()) id_set.insert(str);
            ++num;
        }
        token = strtok_s(NULL, seps, &next_token);
    }
    return num;
}

StudyDataFilter::StudyDataFilter(const char *date)
{
    src = date;
    if(date == NULL || strlen(date) == 0 || strcmp("*", date) == 0 || strcmp("-", date) == 0)
    {
        src = NULL;
        strcpy_s(low, "");
        strcpy_s(high, "99999999");
    }
    else
    {
        const char *p = strchr (date, '-');
        if (p == NULL)
        {
            strcpy_s(low, date);
            strcpy_s(high, date);
        }
        else if(p == date)
        {
            strcpy_s(low, "");
            strcpy_s(high, date + 1);
        }
        else if(p == date + strlen(date) - 1)
        {
            strncpy_s(low, date, strlen(date) - 1);
            strcpy_s(high, "99999999");
        }
        else
        {
            strncpy_s(low, date, p - date);
            strcpy_s(high, p + 1);
        }
    }
}

DcmQueryRetrieveXmlDatabaseHandle::DcmQueryRetrieveXmlDatabaseHandle(const char *storage, long maxStudiesPerStorageArea,
        long maxBytesPerStudy, OFCondition& result) : DcmQueryRetrieveDatabaseHandle(), req(NULL),
        doCheckFindIdentifier(OFFalse), doCheckMoveIdentifier(OFFalse), debugLevel(0),
        rootLevel(PATIENT_LEVEL), lowestLevel(IMAGE_LEVEL), queryLevel(STUDY_LEVEL)
{
    strcpy_s(storageArea, storage);
    HRESULT hr = pXmlDom.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
    if(SUCCEEDED(hr))
    {
        try {
            pXmlDom->preserveWhiteSpace = VARIANT_FALSE;
	        pXmlDom->async = VARIANT_FALSE;
            pXmlDom->appendChild(pXmlDom->createNode(MSXML2::NODE_ELEMENT, L"result_set", L"http://www.kurumi.com.cn/xsd/study"));
            //hr = pXmlDom->setProperty("SelectionLanguage", "XPath");
            //hr = pXmlDom->setProperty("SelectionNamespaces", "xmlns:fn='http://www.w3.org/2005/xpath-functions'");
            result = EC_Normal;
            return;
        }
        CATCH_COM_ERROR(__FUNCSIG__, CERR);
    }
    result = DcmQRXmlDatabaseErrorC;
}

bool DcmQueryRetrieveXmlDatabaseHandle::add_study(MSXML2::IXMLDOMElementPtr &pStudy,
    const set<string> &pids, const set<string> &ser_uids, const set<string> &inst_uids,
    const StudyDataFilter &date_filter, bool study_only)
{
    char buff[MAX_PATH];
    if(pStudy == NULL) return false;
    MSXML2::IXMLDOMNodePtr patient_id_attr = pStudy->selectSingleNode(L"patient/@id");
    if(patient_id_attr == NULL) THROW_HR_TO_COM_ERROR(ERROR_XML_PARSE_ERROR, OLESTR("xml node /study/patient has not @id"));
    if(patient_id_attr->text.length() == 0)
        THROW_HR_TO_COM_ERROR(ERROR_XML_PARSE_ERROR, OLESTR("xml node /study/patient has not @id"));
    if(pids.size() && pids.cend() == pids.find((LPCSTR)patient_id_attr->text)) return false;
    _bstr_t study_date(pStudy->getAttribute(L"date"));
    if(study_date.length() && !date_filter.inRange((LPCSTR)study_date)) return false;

    sprintf_s(buff, "patient_root[@id='%s']", (LPCSTR)patient_id_attr->text);
    MSXML2::IXMLDOMElementPtr patient_node = pXmlDom->documentElement->selectSingleNode(buff);
    if(patient_node == NULL)
    {
        patient_node = pXmlDom->createNode(MSXML2::NODE_ELEMENT, L"patient_root", L"http://www.kurumi.com.cn/xsd/study");
        patient_node->setAttribute(L"id", patient_id_attr->text);
        pXmlDom->documentElement->appendChild(patient_node);
    }
    _bstr_t study_uid(pStudy->getAttribute(L"id"));
    if(study_uid.length() == 0)
        THROW_HR_TO_COM_ERROR(ERROR_XML_PARSE_ERROR, OLESTR("study xml dom has not @id"));
    sprintf_s(buff, "study[@id='%s']", (LPCSTR)study_uid);
    MSXML2::IXMLDOMElementPtr exist_study_node = patient_node->selectSingleNode(buff);
    if(exist_study_node == NULL)
    {
        long instance_count = 0, series_count = 0;
        if(study_only)
        {
            exist_study_node = pStudy->cloneNode(VARIANT_FALSE);
            MSXML2::IXMLDOMNodePtr patient = pStudy->selectSingleNode(L"patient");
            if(patient) exist_study_node->appendChild(patient->cloneNode(VARIANT_FALSE));

            _variant_t sc = exist_study_node->getAttribute(L"series_count");
            if(sc.vt == VT_NULL) series_count = 0;
            else
            {
                sc.ChangeType(VT_I4);
                series_count = sc.lVal;
            }
            _variant_t ic = exist_study_node->getAttribute(L"instance_count");
            if(ic.vt == VT_NULL) instance_count = 0;
            else
            {
                ic.ChangeType(VT_I4);
                instance_count = ic.lVal;
            }
        }
        else
        {
            exist_study_node = pStudy->cloneNode(VARIANT_TRUE);

            MSXML2::IXMLDOMNodeListPtr series_list = exist_study_node->selectNodes(L"series[@id]");
            for(long i = series_list->length - 1; i >= 0; --i)
            {
                MSXML2::IXMLDOMElementPtr series = series_list->Getitem(i);
                _bstr_t se_id(series ? series->getAttribute(L"id") : L"");

                bool ser_uid_match = (se_id.length() > 0);
                if(ser_uid_match && ser_uids.size())
                    ser_uid_match = (ser_uids.end() != ser_uids.find((LPCSTR)se_id));

                if(ser_uid_match)
                {
                    long inst_per_series_count = 0;
                    MSXML2::IXMLDOMNodeListPtr inst_list = series->selectNodes(L"instance[@id]");
                    for(long j = inst_list->length - 1; j >= 0; --j)
                    {
                        MSXML2::IXMLDOMElementPtr inst = inst_list->Getitem(j);
                        _bstr_t inst_id(inst ? inst->getAttribute(L"id") : L"");

                        bool inst_uid_match = (inst_id.length() > 0);
                        if(inst_uid_match && inst_uids.size())
                            inst_uid_match = (inst_uids.end() != inst_uids.find((LPCSTR)inst_id));

                        if(inst_uid_match) ++inst_per_series_count;
                        else series->removeChild(inst);
                    }

                    if(inst_list->length)
                    {
                        series->setAttribute(L"instance_count", inst_per_series_count);
                        instance_count += inst_per_series_count;
                        ++series_count;
                    }
                    else exist_study_node->removeChild(series);
                }
                else exist_study_node->removeChild(series);
            }
            exist_study_node->setAttribute(L"instance_count", instance_count);
            exist_study_node->setAttribute(L"series_count", series_count);
        }

        if(instance_count > 0 && series_count > 0)
        {
            patient_node->appendChild(exist_study_node);

            _variant_t stc = patient_node->getAttribute(L"study_count");
            if(stc.vt == VT_NULL)
            {
                stc.vt = VT_I4;
                stc.lVal = 0;
            }
            else stc.ChangeType(VT_I4);
            stc.lVal += 1;
            patient_node->setAttribute(L"study_count", stc);

            _variant_t ic = patient_node->getAttribute(L"instance_count");
            if(ic.vt == VT_NULL)
            {
                ic.vt = VT_I4;
                ic.lVal = 0;
            }
            else ic.ChangeType(VT_I4);
            ic.lVal += instance_count;
            patient_node->setAttribute(L"instance_count", ic);

            _variant_t sc = patient_node->getAttribute(L"series_count");
            if(sc.vt == VT_NULL)
            {
                sc.vt = VT_I4;
                sc.lVal = 0;
            }
            else sc.ChangeType(VT_I4);
            sc.lVal += series_count;
            patient_node->setAttribute(L"series_count", sc);
            return true;
        }
    }
    return false;
}

size_t DcmQueryRetrieveXmlDatabaseHandle::findByStudyDate(const set<string> &ser_uids,
    const set<string> &inst_uids, const StudyDataFilter &date_filter)
{
    if(debugLevel)
    {
        CERR << __FUNCSIG__" get study date:" << (date_filter.src ? date_filter.src : "") << endl;
    }
    if(date_filter.src == NULL) return 0;

    char date[9] = "";
    if(strlen(date_filter.low) == 8) strcpy_s(date, date_filter.low);
    else if (strlen(date_filter.high) == 8) strcpy_s(date, date_filter.high);
    else return 0;

    size_t num = 0;
    set<string> empty_set;
    if(strlen(date) == 8 && strcmp(date, "99999999"))
    {
        char buff[260], hash[9];
        sprintf_s(buff, "%s\\pacs\\indexdir\\00080020\\%c%c%c%c\\%c%c\\%c%c.xml", getPacsBase(),
            date[0], date[1], date[2], date[3], date[4], date[5], date[6], date[7]);
        HRESULT hr = 0;
        try {
            MSXML2::IXMLDOMDocument2Ptr pDomDate;
            hr = pDomDate.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
            if(FAILED(hr)) THROW_HR_TO_COM_ERROR(hr, OLESTR("pDomDate.CreateInstance()"));
            if(pDomDate->load(_variant_t(buff)) == VARIANT_FALSE)
                THROW_HR_TO_COM_ERROR(STG_E_ACCESSDENIED, OLESTR("pDomDate->load()"));

            bool study_only = (queryLevel == PATIENT_LEVEL || queryLevel == STUDY_LEVEL);
            if(study_only)
            {
                MSXML2::IXMLDOMNodeListPtr studies = pDomDate->documentElement->selectNodes(L"study");
                while(MSXML2::IXMLDOMElementPtr pStudy = studies->nextNode())
                {
                    try {
                        if(add_study(pStudy, empty_set, ser_uids, inst_uids, date_filter, study_only)) ++num;
                    }
                    CATCH_COM_ERROR(__FUNCSIG__, CERR);
                }
            }
            else
            {
                MSXML2::IXMLDOMNodeListPtr study_uids = pDomDate->documentElement->selectNodes(L"study/@id");
                while(MSXML2::IXMLDOMNodePtr pStudyUID = study_uids->nextNode())
                {
                    HashStrW((LPCWSTR)pStudyUID->text, hash, sizeof(hash));
                    sprintf_s(buff, "%s\\pacs\\indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml", getPacsBase(),
                        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], (LPCSTR)pStudyUID->text);
                    try {
                        MSXML2::IXMLDOMDocument2Ptr pDomStudy;
                        hr = pDomStudy.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
                        if(FAILED(hr)) THROW_HR_TO_COM_ERROR(hr, OLESTR("pDomStudy.CreateInstance()"));
                        if(pDomStudy->load(_variant_t(buff)) == VARIANT_FALSE || pDomStudy->documentElement == NULL)
                            THROW_HR_TO_COM_ERROR(STG_E_ACCESSDENIED, OLESTR("pDomStudy->load()"));
                        if(add_study(pDomStudy->documentElement, empty_set, ser_uids, inst_uids, date_filter, study_only)) ++num;
                    }
                    CATCH_COM_ERROR(__FUNCSIG__, CERR);
                }
            }
        }
        CATCH_COM_ERROR(__FUNCSIG__, CERR);
    }
    return num;
}

size_t DcmQueryRetrieveXmlDatabaseHandle::findByStudyUIDs(const set<string> &study_uids,
    const set<string> &pids, const set<string> &ser_uids, const set<string> &inst_uids,
    const StudyDataFilter &date_filter)
{
    if(debugLevel)
    {
        CERR << __FUNCSIG__" get study uids:" << endl;
        for_each(study_uids.begin(), study_uids.end(), [](const string &uid) { CERR << uid << endl; });
    }
    size_t num = 0;
    for(set<OFString>::const_iterator it = study_uids.cbegin(); it != study_uids.cend(); ++it)
    {
        char buff[260], hash[9];
        HashStr(it->c_str(), hash, sizeof(hash));
        sprintf_s(buff, "%s\\pacs\\indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml", getPacsBase(),
            hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], it->c_str());
        HRESULT hr = 0;
        try {
            MSXML2::IXMLDOMDocument2Ptr pDomStudy;
            hr = pDomStudy.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
            if(FAILED(hr)) THROW_HR_TO_COM_ERROR(hr, OLESTR("pDomStudy.CreateInstance()"));
            if(pDomStudy->load(_variant_t(buff)) == VARIANT_FALSE || pDomStudy->documentElement == NULL)
                THROW_HR_TO_COM_ERROR(STG_E_ACCESSDENIED, OLESTR("pDomStudy->load()"));
            if(add_study(pDomStudy->documentElement, pids, ser_uids, inst_uids, date_filter)) ++num;
        }
        CATCH_COM_ERROR(__FUNCSIG__, CERR);
    }
    return num;
}

size_t DcmQueryRetrieveXmlDatabaseHandle::findByPatientIDs(const set<string> &pids,
    const set<string> &ser_uids, const set<string> &inst_uids, const StudyDataFilter &date_filter)
{
    if(debugLevel)
    {
        CERR << __FUNCSIG__" get patient ids:" << endl;
        for_each(pids.begin(), pids.end(), [](const string &uid) { CERR << uid << endl; });
    }
    size_t num = 0;
    for(set<OFString>::const_iterator it = pids.cbegin(); it != pids.cend(); ++it)
    {
        char pid_b32[128], buff[260], hash[9];
        HashStr(it->c_str(), hash, sizeof(hash));
        EncodeBase32(it->c_str(), pid_b32, sizeof(pid_b32));
        sprintf_s(buff, "%s\\pacs\\indexdir\\00100020\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml", getPacsBase(),
            hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], pid_b32);
        HRESULT hr = 0;
        try {
            MSXML2::IXMLDOMDocument2Ptr pDomPatient;
            hr = pDomPatient.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
            if(FAILED(hr)) THROW_HR_TO_COM_ERROR(hr, OLESTR("pDomPatient.CreateInstance()"));
            if(pDomPatient->load(_variant_t(buff)) == VARIANT_FALSE)
                THROW_HR_TO_COM_ERROR(STG_E_ACCESSDENIED, OLESTR("pDomPatient->load()"));

            bool study_only = (queryLevel == PATIENT_LEVEL || queryLevel == STUDY_LEVEL);
            if(study_only)
            {
                MSXML2::IXMLDOMNodeListPtr studies = pDomPatient->documentElement->selectNodes(L"study");
                while(MSXML2::IXMLDOMElementPtr pStudy = studies->nextNode())
                {
                    try {
                        if(add_study(pStudy, pids, ser_uids, inst_uids, date_filter, study_only)) ++num;
                    }
                    CATCH_COM_ERROR(__FUNCSIG__, CERR);
                }
            }
            else
            {
                MSXML2::IXMLDOMNodeListPtr study_uids = pDomPatient->documentElement->selectNodes(L"study/@id");
                while(MSXML2::IXMLDOMNodePtr pStudyUID = study_uids->nextNode())
                {
                    HashStrW((LPCWSTR)pStudyUID->text, hash, sizeof(hash));
                    sprintf_s(buff, "%s\\pacs\\indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml", getPacsBase(),
                        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], (LPCSTR)pStudyUID->text);
                    try {
                        MSXML2::IXMLDOMDocument2Ptr pDomStudy;
                        hr = pDomStudy.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
                        if(FAILED(hr)) THROW_HR_TO_COM_ERROR(hr, OLESTR("pDomStudy.CreateInstance()"));
                        if(pDomStudy->load(_variant_t(buff)) == VARIANT_FALSE || pDomStudy->documentElement == NULL)
                            THROW_HR_TO_COM_ERROR(STG_E_ACCESSDENIED, OLESTR("pDomStudy->load()"));
                        if(add_study(pDomStudy->documentElement, pids, ser_uids, inst_uids, date_filter, study_only)) ++num;
                    }
                    CATCH_COM_ERROR(__FUNCSIG__, CERR);
                }
            }
        }
        CATCH_COM_ERROR(__FUNCSIG__, CERR);
    }
    return num;
}

size_t DcmQueryRetrieveXmlDatabaseHandle::findRequestFilter(DcmDataset *findRequestIdentifiers)
{
    set<string> patient_ids, study_uids, series_uids, instance_uids;
    StudyDataFilter study_dates;

    DcmElement *pDateIDElem = NULL;
    findRequestIdentifiers->findAndGetElement(DCM_StudyDate, pDateIDElem);
    if(pDateIDElem)
    {
        char *pDates = NULL;
        pDateIDElem->getString(pDates);
        study_dates = StudyDataFilter(pDates);
    }
    
    DcmElement *pPatientIDElem = NULL;
    findRequestIdentifiers->findAndGetElement(DCM_PatientID, pPatientIDElem);
    if(pPatientIDElem)
    {
        char *pIDs = NULL;
        pPatientIDElem->getString(pIDs);
        if(pIDs) extractMultiValues(pIDs, patient_ids);
    }

    DcmElement *pStudyUIDElem = NULL;
    findRequestIdentifiers->findAndGetElement(DCM_StudyInstanceUID, pStudyUIDElem);
    if(pStudyUIDElem)
    {
        char *pStudyUIDs = NULL;
        pStudyUIDElem->getString(pStudyUIDs);
        if(pStudyUIDs) extractMultiValues(pStudyUIDs, study_uids);
    }

    DcmElement *pSeriesUIDElem = NULL;
    findRequestIdentifiers->findAndGetElement(DCM_SeriesInstanceUID, pSeriesUIDElem);
    if(pSeriesUIDElem)
    {
        char *pSeriesUIDs = NULL;
        pSeriesUIDElem->getString(pSeriesUIDs);
        if(pSeriesUIDs) extractMultiValues(pSeriesUIDs, series_uids);
    }

    DcmElement *pInstanceUIDElem = NULL;
    findRequestIdentifiers->findAndGetElement(DCM_SOPInstanceUID, pInstanceUIDElem);
    if(pInstanceUIDElem)
    {
        char *pInstUIDs = NULL;
        pInstanceUIDElem->getString(pInstUIDs);
        if(pInstUIDs) extractMultiValues(pInstUIDs, instance_uids);
    }

    if(study_uids.size()) findByStudyUIDs(study_uids, patient_ids, series_uids, instance_uids, study_dates);
    else if(patient_ids.size()) findByPatientIDs(patient_ids, series_uids, instance_uids, study_dates);
    else findByStudyDate(series_uids, instance_uids, study_dates);

    size_t num = 0;
    MSXML2::IXMLDOMNodeListPtr psl = pXmlDom->documentElement->selectNodes(L"patient_root");
    for(int i = psl->length - 1; i >= 0; --i)
    {
        _variant_t stc, sc, ic;
        MSXML2::IXMLDOMElementPtr ps = psl->Getitem(i);

        stc = ps->getAttribute(L"study_count");
        if(stc.vt == VT_NULL) goto remove_patient_from_root;
        stc.ChangeType(VT_I4);
        if(stc.lVal == 0) goto remove_patient_from_root;

        sc = ps->getAttribute(L"series_count");
        if(sc.vt == VT_NULL) goto remove_patient_from_root;
        sc.ChangeType(VT_I4);
        if(sc.lVal == 0) goto remove_patient_from_root;

        ic = ps->getAttribute(L"instance_count");
        if(ic.vt == VT_NULL) goto remove_patient_from_root;
        ic.ChangeType(VT_I4);
        if(ic.lVal == 0) goto remove_patient_from_root;

        num += ic.lVal;
        continue;
remove_patient_from_root:
        pXmlDom->documentElement->removeChild(ps);
    }
    if(debugLevel == 3) CERR << (LPCSTR)pXmlDom->xml << endl;
    return num;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::storeRequest(const char *SOPClassUID,
    const char *SOPInstanceUID, const char *imageFileName,
    DcmQueryRetrieveDatabaseStatus *status, OFBool isNew, DcmQueryRetrieveStoreContext *psc)
{
    if(psc && psc->pac)
        return psc->pac->cbToDcmQueryRetrieveStoreContext(psc->getFileName(), psc->getDataset());
    else
        return EC_IllegalParameter;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::startFindRequest(const char *SOPClassUID,
    DcmDataset *findRequestIdentifiers, DcmQueryRetrieveDatabaseStatus *status)
{
    OFCondition cond = EC_Normal;
    OFBool qrLevelFound = OFFalse;
    
    // Is SOPClassUID supported ?
    if (strcmp( SOPClassUID, UID_FINDPatientRootQueryRetrieveInformationModel) == 0)
        rootLevel = PATIENT_LEVEL;
    else if (strcmp( SOPClassUID, UID_FINDStudyRootQueryRetrieveInformationModel) == 0)
        rootLevel = STUDY_LEVEL;
#ifndef NO_PATIENTSTUDYONLY_SUPPORT
    else if (strcmp( SOPClassUID, UID_FINDPatientStudyOnlyQueryRetrieveInformationModel) == 0)
    {
        rootLevel = PATIENT_LEVEL;
        lowestLevel = STUDY_LEVEL;
    }
#endif
    else {
        status->setStatus(STATUS_FIND_Refused_SOPClassNotSupported);
        return (DcmQRXmlDatabaseErrorC) ;
    }

    // Parse Identifiers in the Dicom Object
    // Find Query Level and contruct a list of query identifiers
    DcmElement* dcelem = NULL;
    cond = findRequestIdentifiers->findAndGetElement(DCM_QueryRetrieveLevel, dcelem);
    if(dcelem)
    {
        DcmTagKey xtag = dcelem->getTag().getXTag();
        char *s = NULL, level[DIC_CS_LEN + 1] = "";
        dcelem->getString(s);
        if(s && strcpy_s(level, s) == 0)
        {
            _strupr_s(level);
            if (strncmp (level, PATIENT_LEVEL_STRING, strlen(PATIENT_LEVEL_STRING)) == 0)
                queryLevel = PATIENT_LEVEL;
            else if (strncmp (level, STUDY_LEVEL_STRING, strlen(STUDY_LEVEL_STRING)) == 0)
                queryLevel = STUDY_LEVEL;
            else if (strncmp (level, SERIE_LEVEL_STRING, strlen(SERIE_LEVEL_STRING)) == 0)
                queryLevel = SERIE_LEVEL;
            else if (strncmp (level, IMAGE_LEVEL_STRING, strlen(IMAGE_LEVEL_STRING)) == 0)
                queryLevel = IMAGE_LEVEL;
            else CERR << "Error DcmQueryRetrieveXmlDatabaseHandle::startFindRequest(): Illegal query level (" << level << "), using default value STUDY_LEVEL" << endl;
        }
        else CERR << "Error DcmQueryRetrieveXmlDatabaseHandle::startFindRequest(): Illegal query level (" << level << "), using default value STUDY_LEVEL" << endl;
    }
    else CERR << "Error DcmQueryRetrieveXmlDatabaseHandle::startFindRequest(): Can't find element(" << DCM_QueryRetrieveLevel.toString() << "), using default value STUDY_LEVEL" << endl;

    cond = testFindRequestList(findRequestIdentifiers, queryLevel, rootLevel, lowestLevel);
    if(cond.bad())
    {
        status->setStatus(STATUS_FIND_Refused_OutOfResources);
        return cond;
    }
    
    if(0 == findRequestFilter(findRequestIdentifiers))
    {
        status->setStatus(STATUS_Success);
        return cond;
    }
    req = findRequestIdentifiers;
    status->setStatus(STATUS_Pending);
    return cond;
}

bool DcmQueryRetrieveXmlDatabaseHandle::fillNextElementToDcmdataset()
{
    MSXML2::IXMLDOMElementPtr inst;
    if(inl) inst = inl->nextNode();
    if(inst == NULL)
    {
        if(sel) se = sel->nextNode();
        if(se == NULL)
        {
            if(stl) st = stl->nextNode();
            if(st == NULL)
            {
                ds.clear();
                ds.putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 100");
                switch(queryLevel)
                {
                case PATIENT_LEVEL:
                    ds.putAndInsertString(DCM_QueryRetrieveLevel, PATIENT_LEVEL_STRING);
                    break;
                case STUDY_LEVEL:
                    ds.putAndInsertString(DCM_QueryRetrieveLevel, STUDY_LEVEL_STRING);
                    break;
                case SERIE_LEVEL:
                    ds.putAndInsertString(DCM_QueryRetrieveLevel, SERIE_LEVEL_STRING);
                    break;
                case IMAGE_LEVEL:
                    ds.putAndInsertString(DCM_QueryRetrieveLevel, IMAGE_LEVEL_STRING);
                    break;
                default:
                    ds.insertEmptyElement(DCM_QueryRetrieveLevel);
                }

                if(ptl == NULL) ptl = pXmlDom->documentElement->selectNodes(L"patient_root[@id]");
                pt = ptl->nextNode();
                if(pt)
                {
                    for(int i = 0; i < patientRootFindAttrNum; ++i)
                    {
                        _variant_t vt(pt->getAttribute(patientRootFindAttr[i].first.c_str()));
                        fill_dcmdataset(vt, ds, patientRootFindAttr[i].second);
                    }
                    stl = pt->selectNodes(L"study[@id]");
                    st = stl->nextNode();
                }
            }
            if(st)
            {
                for(int i = 0; i < studyFindAttrNum; ++i)
                {
                    _variant_t vt(st->getAttribute(studyFindAttr[i].first.c_str()));
                    fill_dcmdataset(vt, ds, studyFindAttr[i].second);
                }
                MSXML2::IXMLDOMElementPtr patient = st->selectSingleNode(L"patient");
                for(int i = 0; i < patientFindAttrNum; ++i)
                {
                    _variant_t vt(NULL);
                    if(patient) vt = patient->getAttribute(patientFindAttr[i].first.c_str());
                    fill_dcmdataset(vt, ds, patientFindAttr[i].second);
                }

                if(queryLevel == STUDY_LEVEL || queryLevel == PATIENT_LEVEL) return true;

                sel = st->selectNodes(L"series[@id]");
                se = sel->nextNode();
            }
        }
        if(se)
        {
            for(int i = 0; i < seriesFindAttrNum; ++i)
            {
                _variant_t vt(se->getAttribute(seriesFindAttr[i].first.c_str()));
                fill_dcmdataset(vt, ds, seriesFindAttr[i].second);
            }
            if(queryLevel == SERIE_LEVEL) return true;

            inl = se->selectNodes(L"instance[@id]");
            inst = inl->nextNode();
        }
    }
    if(inst)
    {
        for(int i = 0; i < instanceFindAttrNum; ++i)
        {
            _variant_t vt(inst->getAttribute(instanceFindAttr[i].first.c_str()));
            fill_dcmdataset(vt, ds, instanceFindAttr[i].second);
        }
        if(req)
        {
            DcmObject *dO = NULL;
            while(dO = req->nextInContainer(dO))
            {
                DcmElement *de = OFdynamic_cast(DcmElement*, dO);
                if(de && de->isLeaf()) ds.insertEmptyElement(de->getTag(), false);
            }
        }
    }
    return (inst != NULL);
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::nextFindResponse(
    DcmDataset **findResponseIdentifiers, DcmQueryRetrieveDatabaseStatus *status)
{
    if(fillNextElementToDcmdataset())
    {
        *findResponseIdentifiers = new DcmDataset(ds);
        status->setStatus(STATUS_Pending);
    }
    else
    {
        *findResponseIdentifiers = NULL;
        status->setStatus(STATUS_Success);
    }
    return EC_Normal;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::cancelFindRequest(
    DcmQueryRetrieveDatabaseStatus *status)
{
    status->setStatus(STATUS_FIND_Cancel_MatchingTerminatedDueToCancelRequest);
    return EC_Normal;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::startMoveRequest(const char *SOPClassUID,
    DcmDataset *moveRequestIdentifiers, DcmQueryRetrieveDatabaseStatus *status)
{
    OFCondition cond = EC_Normal;
    rootLevel = PATIENT_LEVEL;
    lowestLevel = IMAGE_LEVEL;
    queryLevel = IMAGE_LEVEL;

    if(0 == findRequestFilter(moveRequestIdentifiers))
    {
        status->setStatus(STATUS_Success);
        return cond;
    }
    req = moveRequestIdentifiers;
    status->setStatus(STATUS_Pending);

    return cond;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::nextMoveResponse(char *SOPClassUID,
    char *SOPInstanceUID, char *imageFileName, unsigned short *numberOfRemainingSubOperations,
    DcmQueryRetrieveDatabaseStatus *status)
{
    return EC_Normal;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::cancelMoveRequest(DcmQueryRetrieveDatabaseStatus *status)
{
    return EC_Normal;
}

OFCondition DcmQueryRetrieveXmlDatabaseHandle::pruneInvalidRecords()
{
    return EC_Normal;
}

