#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"
#import <msxml3.dll>
/*
instance state : [backup state]_[include state]
backup state:
    on  : online, remote exist, local exist
    off : offline, remote exist, local does not exist
    new : new instance, remote does not exist, local exist
    old : old instance, remote does not exist, local does not exist
include state:
    in : include
    ex : exclude
*/
#define XML_HEADER "<?xml version=\"1.0\" encoding=\"GBK\"?>"

using namespace std;

static map<string, MSXML2::IXMLDOMDocument2*> study_map;

static MSXML2::IXMLDOMDocument2* create_xmldom(const CMOVE_LOG_CONTEXT &clc)
{
    try
    {
        MSXML2::IXMLDOMDocument2Ptr pXMLDom;
        HRESULT hr = pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
        if(SUCCEEDED(hr))
        {
            pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
	        pXMLDom->async = VARIANT_FALSE;
            pXMLDom->appendChild(pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"study", L"http://www.kurumi.com.cn/xsd/study"));
            MSXML2::IXMLDOMElementPtr pRoot = pXMLDom->documentElement;
            if(pRoot)
            {
                hr = pRoot->setAttribute(L"id", clc.file.studyUID);
                if(clc.file.PathSeparator() == '/')
                    hr = pRoot->setAttribute(L"hash_prefix", clc.file.hash);
                else
                {
                    char buff[sizeof(clc.file.hash)];
                    strcpy_s(buff, clc.file.hash);
                    replace(buff, buff + sizeof(clc.file.hash), '\\', '/');
                    hr = pRoot->setAttribute(L"hash_prefix", buff);
                }
                if(strcmp(clc.file.studyUID, clc.study.studyUID) == 0)
                {
                    hr = pRoot->setAttribute(L"accession_number", clc.study.accessionNumber);
                    hr = pRoot->setAttribute(L"date", clc.study.studyDate);
                    hr = pRoot->setAttribute(L"time", clc.study.studyTime);
                }
                
                MSXML2::IXMLDOMNodePtr pAssociations = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"associations", L"http://www.kurumi.com.cn/xsd/study");
                pXMLDom->documentElement->appendChild(pAssociations);
            }
            MSXML2::IXMLDOMDocument2 *pdom = pXMLDom.Detach();
            return pdom;
        }
    }
    CATCH_COM_ERROR("create_xmldom()")
    return NULL;
}

static MSXML2::IXMLDOMNode *shallow_copy_study(MSXML2::IXMLDOMNode *root)
{
    MSXML2::IXMLDOMNodePtr study = root->cloneNode(VARIANT_FALSE);
    if(study)
    {
        MSXML2::IXMLDOMNodePtr patient = root->selectSingleNode(L"patient");
        if(patient) study->appendChild(patient->cloneNode(VARIANT_TRUE));
        return study.Detach();
    }
    else
        return NULL;
}

static bool save_index_study_date(MSXML2::IXMLDOMDocument2 *pDomStudy)
{
    try
    {
        MSXML2::IXMLDOMNodePtr study = shallow_copy_study(pDomStudy->documentElement);
        if(study == NULL) throw logic_error("shallow_copy_study() return NULL");
        MSXML2::IXMLDOMNodePtr dateNode = study->attributes->getNamedItem(L"date");
        _bstr_t attr_value;
        if(dateNode && dateNode->text.length()) attr_value = dateNode->text;
        else attr_value = L"19700102";
        string xml_path((LPCSTR)attr_value);
        xml_path.insert(6, 1, '\\').insert(4, 1, '\\')
            .insert(0, "\\pacs\\indexdir\\00080020\\")
            .insert(0, COMMONLIB_PACS_BASE).append(".xml");
        if(!PrepareFileDir(xml_path.c_str())) throw logic_error(xml_path.insert(0, "PrepareFileDir ").append(" failed"));
        MSXML2::IXMLDOMDocument2Ptr pDom;
        pDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
        errno_t en = _access_s(xml_path.c_str(), 6);
        if(en == ENOENT)
        {   // not exist
            pDom->appendChild(pDom->createNode(MSXML2::NODE_ELEMENT, L"study_date", L"http://www.kurumi.com.cn/xsd/study"));
        }
        else if(en == 0)
        {   // exist, rw OK
            if(VARIANT_FALSE == pDom->load(xml_path.c_str()))
                throw logic_error(xml_path.insert(0, "load exist xml ").append(" failed"));
        }
        else
        {   // other error: EACCES EINVAL
            char msg[1024] = " ";
            strerror_s(msg + 1, sizeof(msg) - 1, en);
            throw logic_error(xml_path.insert(0, "open xml ").append(msg));
        }
        if(pDom == NULL || pDom->documentElement == NULL)
            throw logic_error(xml_path.insert(0, "open xml ").append(" failed"));
        
        _bstr_t query_exist_study(L"study[@id='");
        query_exist_study += pDomStudy->documentElement->getAttribute(L"id").bstrVal;
        query_exist_study += "']";
        if(MSXML2::IXMLDOMNodePtr exist_study = pDom->documentElement->selectSingleNode(query_exist_study))
            pDom->documentElement->removeChild(exist_study);
        pDom->documentElement->appendChild(study);

        ofstream fxml(xml_path.c_str(), ios_base::trunc | ios_base::out, _SH_DENYNO);
        if(fxml.good())
        {
            fxml << XML_HEADER << (LPCSTR)pDom->documentElement->xml << endl;
            fxml.close();
            cout << "trigger index_study_date " << xml_path << endl;
        }
        else throw runtime_error(xml_path.insert(0, "save ").append(" error"));
        return true;
    }
    CATCH_COM_ERROR("save_index_study_date()")
    return false;
}

static bool save_index_patient(MSXML2::IXMLDOMDocument2 *pDomStudy)
{
    try
    {
        MSXML2::IXMLDOMNodePtr study = shallow_copy_study(pDomStudy->documentElement);
        if(study == NULL) throw logic_error("shallow_copy_study() return NULL");
        MSXML2::IXMLDOMNodePtr patient = study->selectSingleNode(L"patient");
        if(patient == NULL) throw logic_error("shallow_copy_study()'s patient node is NULL");

        _bstr_t hash_prefix, encoded;
        MSXML2::IXMLDOMNodePtr hash_prefix_attr = patient->attributes->getNamedItem(L"hash_prefix");
        if(hash_prefix_attr == NULL) throw logic_error("shallow_copy_study()'s patient[prefix] attr is NULL");
        hash_prefix = hash_prefix_attr->text;
        if(hash_prefix.length() == 0) throw logic_error("shallow_copy_study()'s patient[prefix] value is NULL");
        string hash_prefix_str((LPCSTR)hash_prefix);
        replace(hash_prefix_str.begin(), hash_prefix_str.end(), '/', '\\');

        MSXML2::IXMLDOMNodePtr encoded_attr = patient->attributes->getNamedItem(L"encoded");
        if(encoded_attr == NULL) throw logic_error("shallow_copy_study()'s patient[encoded] attr is NULL");
        encoded = encoded_attr->text;
        if(encoded.length() == 0) throw logic_error("shallow_copy_study()'s patient[encoded] value is NULL");

        string xml_path(COMMONLIB_PACS_BASE);
        xml_path.append("\\pacs\\indexdir\\00100020\\").append(hash_prefix_str)
            .append(1, '\\').append((LPCSTR)encoded).append(".xml");
        if(!PrepareFileDir(xml_path.c_str())) throw logic_error(xml_path.insert(0, "PrepareFileDir ").append(" failed"));

        MSXML2::IXMLDOMDocument2Ptr pDom;
        pDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
        errno_t en = _access_s(xml_path.c_str(), 6);
        if(en == ENOENT)
        {   // not exist
            pDom->appendChild(pDom->createNode(MSXML2::NODE_ELEMENT, L"patient_all_study", L"http://www.kurumi.com.cn/xsd/study"));
        }
        else if(en == 0)
        {   // exist, rw OK
            if(VARIANT_FALSE == pDom->load(xml_path.c_str()))
                throw logic_error(xml_path.insert(0, "load exist xml ").append(" failed"));
        }
        else
        {   // other error: EACCES EINVAL
            char msg[1024] = " ";
            strerror_s(msg + 1, sizeof(msg) - 1, en);
            throw logic_error(xml_path.insert(0, "open xml ").append(msg));
        }
        if(pDom == NULL || pDom->documentElement == NULL)
            throw logic_error(xml_path.insert(0, "open xml ").append(" failed"));
        
        _bstr_t query_exist_study(L"study[@id='");
        query_exist_study += pDomStudy->documentElement->getAttribute(L"id").bstrVal;
        query_exist_study += "']";
        if(MSXML2::IXMLDOMNodePtr exist_study = pDom->documentElement->selectSingleNode(query_exist_study))
            pDom->documentElement->removeChild(exist_study);
        pDom->documentElement->appendChild(study);

        ofstream fxml(xml_path.c_str(), ios_base::trunc | ios_base::out, _SH_DENYNO);
        if(fxml.good())
        {
            fxml << XML_HEADER << (LPCSTR)pDom->documentElement->xml << endl;
            fxml.close();
            cout << "trigger index_patient " << xml_path << endl;
        }
        else throw runtime_error(xml_path.insert(0, "save ").append(" error"));
        return true;
    }
    CATCH_COM_ERROR("save_index_patient()")
    return false;
}

static void add_association(map<string, MSXML2::IXMLDOMDocument2*> &association_map, MSXML2::IXMLDOMDocument2 *pXMLDom)
{
    if(pXMLDom == NULL) return;
    try
    {
        MSXML2::IXMLDOMNodeListPtr pAssociations = pXMLDom->selectNodes(L"/study/associations/association");
        if(pAssociations == NULL) return;
        while(MSXML2::IXMLDOMNodePtr assoc = pAssociations->nextNode())
        {
            string assoc_id((LPCSTR)assoc->attributes->getNamedItem(L"id")->text);
            MSXML2::IXMLDOMDocument2 *pa = association_map[assoc_id];
            MSXML2::IXMLDOMDocument2Ptr pDomAssoc;
            MSXML2::IXMLDOMNodePtr rec_assoc;
            bool insert_new = false;
            if(pa == NULL)
            {
                pDomAssoc.CreateInstance(__uuidof(MSXML2::DOMDocument30));
                pDomAssoc->preserveWhiteSpace = VARIANT_FALSE;
	            pDomAssoc->async = VARIANT_FALSE;
                rec_assoc = pDomAssoc->createNode(MSXML2::NODE_ELEMENT, L"receive_association", L"http://www.kurumi.com.cn/xsd/study");
                pDomAssoc->appendChild(rec_assoc);
                for(long i = 0; i < assoc->attributes->Getlength(); ++i)
                {
                    MSXML2::IXMLDOMNodePtr attr = assoc->attributes->Getitem(i);
                    pDomAssoc->documentElement->setAttribute(attr->nodeName, attr->text);
                }
                insert_new = true;
            }
            else
            {
                pDomAssoc.Attach(pa);
                rec_assoc = pDomAssoc->documentElement; //receive_association
            }

            if(rec_assoc)
            {
                MSXML2::IXMLDOMNodePtr study = shallow_copy_study(pXMLDom->documentElement);
                if(study)
                    rec_assoc->appendChild(study);
                else
                    cerr << "add_association() shallow_copy_study() failed, return NULL" << endl;
                pa = pDomAssoc.Detach();
                if(insert_new) association_map[assoc_id] = pa;
            }
        }
    }
    CATCH_COM_ERROR("add_association()")
}

#define CLUSTER_SIZE 4096LL
#define ALIGNED_SIZE(x) (((x) & ~(CLUSTER_SIZE - 1LL)) + CLUSTER_SIZE)
static void calculate_size_cluster_aligned(MSXML2::IXMLDOMDocument2 *pXMLDom)
{
    size_t series_count = 0, instance_count = 0;
    __int64 size_aligned = 0;
    MSXML2::IXMLDOMNodeListPtr seriesNodes = pXMLDom->documentElement->selectNodes(L"series");
    while(MSXML2::IXMLDOMNodePtr s = seriesNodes->nextNode())
    {
        size_t current_instance_count = 0;
        MSXML2::IXMLDOMNodeListPtr instances = s->selectNodes(L"instance[@state='new_in' or @state='on_in']");
        while(MSXML2::IXMLDOMNodePtr n = instances->nextNode())
        {
            __int64 fs = _atoi64((LPCSTR)n->attributes->getNamedItem(L"file_size")->text);
            size_aligned += ALIGNED_SIZE(fs);
            ++current_instance_count;
        }
        MSXML2::IXMLDOMAttributePtr attr = pXMLDom->createAttribute(L"collection_state");
        if(current_instance_count)
        {
            ++series_count;
            instance_count += current_instance_count;
            attr->PutnodeValue(L"include");
        }
        else
            attr->PutnodeValue(L"exclude");
        s->attributes->setNamedItem(attr);
    }
    pXMLDom->documentElement->setAttribute(L"study_size_cluster_aligned", size_aligned);
    pXMLDom->documentElement->setAttribute(L"series_count", series_count);
    pXMLDom->documentElement->setAttribute(L"instance_count", instance_count);
}

// ref static: study_map
void save_index_study_receive_to_session()
{
    map<string, MSXML2::IXMLDOMDocument2*> association_map;
    for(map<string, MSXML2::IXMLDOMDocument2*>::iterator it = study_map.begin(); it != study_map.end(); ++it)
    {
        if(it->second == NULL)
        {
            cerr << "save_index_study_receive_to_session() study phase" << " failed at " << it->first << "'s DOM pointer is NULL" << endl;
            continue;
        }
        try
        {
            char studyHash[9], xmlpath[MAX_PATH];
            MSXML2::IXMLDOMDocument2Ptr pXMLDom;
            pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
            pXMLDom.Attach(it->second);
            _bstr_t hash_prefix(pXMLDom->documentElement->getAttribute(L"hash_prefix").bstrVal);
            if(hash_prefix.length() > 0)
                sprintf_s(xmlpath, "indexdir/0020000d/%s/", (LPCSTR)hash_prefix);
            else
            {
                HashStr(it->first.c_str(), studyHash, sizeof(studyHash));
                sprintf_s(xmlpath, "indexdir/0020000d/%c%c/%c%c/%c%c/%c%c/", studyHash[0], studyHash[1],
                    studyHash[2], studyHash[3], studyHash[4], studyHash[5], studyHash[6], studyHash[7]);
            }

            calculate_size_cluster_aligned(pXMLDom);

            if(MkdirRecursive(xmlpath))
            {
                strcat_s(xmlpath, it->first.c_str());
                strcat_s(xmlpath, ".xml");
                ofstream fxml(xmlpath, ios_base::trunc | ios_base::out, _SH_DENYNO);
                if(fxml.good())
                {
                    fxml << XML_HEADER << (LPCSTR)pXMLDom->xml << endl;
                    fxml.close();
                    add_association(association_map, pXMLDom);
                }
                else
                {
                    char msg[1024];
                    sprintf_s(msg, "save %s error", xmlpath);
                    throw runtime_error(msg);
                }
            }
            else
            {
                char msg[1024];
                sprintf_s(msg, "can't MkdirRecursive(%s)", xmlpath);
                throw logic_error(msg);
            }
        }
        CATCH_COM_ERROR("save_index_study_receive_to_session() study phase")
    }
    study_map.clear();

    for(map<string, MSXML2::IXMLDOMDocument2*>::iterator it = association_map.begin(); it != association_map.end(); ++it)
    {
        MSXML2::IXMLDOMDocument2 *pDomAssoc = it->second;
        if(pDomAssoc == NULL) continue;
        try
        {
            MSXML2::IXMLDOMNodePtr assoc = pDomAssoc->documentElement; //receive_association
            string xmlpath((LPCSTR)assoc->attributes->getNamedItem(L"id")->text);
            xmlpath.insert(8, 1, '/');
            xmlpath.insert(6, 1, '/');
            xmlpath.insert(4, 1, '/');
            xmlpath.insert(0, "indexdir/receive/");
            xmlpath.append(".xml");
            if(PrepareFileDir(xmlpath.c_str()))
            {
                ofstream fxml(xmlpath.c_str(), ios_base::trunc | ios_base::out, _SH_DENYNO);
                if(fxml.good())
                {
                    fxml << XML_HEADER << (LPCSTR)pDomAssoc->xml << endl;
                    fxml.close();
                }
            }
            pDomAssoc->Release();
        }
        CATCH_COM_ERROR("save_index_study_receive_to_session() association phase")
    }
    association_map.clear();
}

static void add_instance(MSXML2::IXMLDOMDocument2 *pXMLDom, const CMOVE_LOG_CONTEXT &clc)
{
    try
    {
        char filter[128];
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL)
        {
            cerr << "add_instance() XMLDOM can't find associations element." << endl;
            return;
        }
        if(strcmp(clc.file.studyUID, clc.study.studyUID) == 0)
        {
            root->setAttribute(L"accession_number", clc.study.accessionNumber);
            root->setAttribute(L"date", clc.study.studyDate);
            root->setAttribute(L"time", clc.study.studyTime);
        }

        if(strlen(clc.assoc.id) > 0)
        {
            MSXML2::IXMLDOMNodePtr assocCollection = root->selectSingleNode(L"associations");
            if(assocCollection)
            {
                sprintf_s(filter, "association[@id='%s']", clc.assoc.id);
                MSXML2::IXMLDOMElementPtr asso = assocCollection->selectSingleNode(filter);
                if(asso == NULL)
                {
                    asso = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"association", L"http://www.kurumi.com.cn/xsd/study");
                    if(asso)
                    {
                        asso->setAttribute(L"id", clc.assoc.id);
                        asso->setAttribute(L"calling_ae", clc.assoc.callingAE);
                        asso->setAttribute(L"calling_address", clc.assoc.callingAddr);
                        asso->setAttribute(L"called_ae", clc.assoc.calledAE);
                        asso->setAttribute(L"called_address", clc.assoc.calledAddr);
                        asso->setAttribute(L"port", clc.assoc.port);
                        assocCollection->appendChild(asso);
                    }
                }
            }
        }

        sprintf_s(filter, "patient[@id='%s']", clc.file.patientID);
        MSXML2::IXMLDOMElementPtr pat = root->selectSingleNode(filter);
        if(pat == NULL)
        {
            pat = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"patient", L"http://www.kurumi.com.cn/xsd/study");
            if(pat)
            {
                pat->setAttribute(L"id", clc.file.patientID);
                char hash[16] = "", prefix[16] = "", enc[128];
                HashStr(clc.file.patientID, hash, sizeof(hash));
                sprintf_s(prefix, "%c%c/%c%c/%c%c/%c%c", hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7]);
                pat->setAttribute(L"hash_prefix", prefix);
                EncodeBase32(clc.file.patientID, enc, sizeof(enc));
                pat->setAttribute(L"encoded", enc);
                root->appendChild(pat);
            }
            else
                cerr << "add_instance() create patient node " << clc.file.patientID << " failed." << endl;
        }
        if(pat && strcmp(clc.patient.patientID, clc.file.patientID) == 0)
        {
            pat->setAttribute(L"name", clc.patient.patientsName);
            pat->setAttribute(L"sex", clc.patient.sex);
            pat->setAttribute(L"birthday", clc.patient.birthday);
            pat->setAttribute(L"height", clc.patient.height);
            pat->setAttribute(L"weight", clc.patient.weight);
        }

        char modality[17] = "";
        sprintf_s(filter, "series[@id='%s']", clc.file.seriesUID);
        MSXML2::IXMLDOMElementPtr series = root->selectSingleNode(filter);
        if(series == NULL)
        {
            series = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"series", L"http://www.kurumi.com.cn/xsd/study");
            if(series)
            {
                series->setAttribute(L"id", clc.file.seriesUID);
                root->appendChild(series);
            }
            else
                cerr << "add_instance() create series node " << clc.file.seriesUID << " failed." << endl;
        }
        if(series && strcmp(clc.file.seriesUID, clc.series.seriesUID) == 0)
        {
            if(strlen(clc.series.modality) > 0)
            {
                strcpy_s(modality, clc.series.modality);
            }
            else
            {   // get first token of file name as modality
                size_t cnt = strchr(clc.file.filename, '.') - clc.file.filename;
                if(cnt < sizeof(modality) && cnt > 0)
                    strncpy_s(modality, clc.file.filename, cnt);
            }
            series->setAttribute(L"modality", modality);

            // update study's modality list
            MSXML2::IXMLDOMNodePtr attr = root->attributes->getNamedItem(L"modality");
            if(attr)
            {
                if(NULL == strstr((LPCSTR)attr->text, modality))
                {
                    attr->text += L",";
                    attr->text += modality;
                }
            }
            else
            {
                MSXML2::IXMLDOMNodePtr attr = pXMLDom->createAttribute(L"modality");
                attr->text = modality;
                root->attributes->setNamedItem(attr);
            }
        }

        if(series)
        {
            struct _stat64 fs;
            fs.st_size = 0;
            sprintf_s(filter, "instance[@id='%s']", clc.file.instanceUID);
            MSXML2::IXMLDOMElementPtr instance = series->selectSingleNode(filter);
            if(instance == NULL)
            {
                instance = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"instance", L"http://www.kurumi.com.cn/xsd/study");
                if(instance)
                {
                    instance->setAttribute(L"id", clc.file.instanceUID);
                    instance->setAttribute(L"xfer", clc.file.xfer_new);
                    char instance_path[MAX_PATH];
                    int path_len = sprintf_s(instance_path, "archdir\\%s\\%s", clc.file.studyUID, clc.file.unique_filename);
                    if(clc.file.PathSeparator() == '/')
                        replace(instance_path, instance_path + path_len, '/', '\\');
                    if(_stat64(instance_path, &fs))
                    {
                        perror(instance_path);
                        fs.st_size = 0;
                    }
                    instance->setAttribute(L"file_size", fs.st_size);
                    instance->setAttribute(L"state", fs.st_size ? L"new_in" : L"new_ex");
                    if(clc.file.PathSeparator() == '/')
                        instance->setAttribute(L"url", clc.file.unique_filename);
                    else
                    {
                        char buff[sizeof(clc.file.unique_filename)];
                        strcpy_s(buff, clc.file.unique_filename);
                        replace(buff, buff + sizeof(buff), '\\', '/');
                        instance->setAttribute(L"url", buff);
                    }
                }
                else
                {
                    cerr << "add_instance() create instance node " << clc.file.instanceUID << " failed." << endl;
                }
                series->appendChild(instance);
            }
            if(strlen(clc.assoc.id) > 0)
            {
                MSXML2::IXMLDOMNodePtr receive_from = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"receive_from", L"http://www.kurumi.com.cn/xsd/study");
                if(receive_from)
                {
                    MSXML2::IXMLDOMNodePtr attr = pXMLDom->createAttribute(L"id");
                    attr->text = clc.assoc.id;
                    receive_from->attributes->setNamedItem(attr);
                    attr = pXMLDom->createAttribute(L"xfer_receive");
                    attr->text = clc.file.xfer;
                    receive_from->attributes->setNamedItem(attr);
                    attr = pXMLDom->createAttribute(L"xfer_save");
                    attr->text = clc.file.xfer_new;
                    receive_from->attributes->setNamedItem(attr);
                    attr = pXMLDom->createAttribute(L"file_size_save");
                    char ui64buf[32];
                    if(_ui64toa_s(fs.st_size, ui64buf, sizeof(ui64buf), 10))
                        attr->text = L"0";
                    else
                        attr->text = ui64buf;
                    receive_from->attributes->setNamedItem(attr);
                    if(_stat64(clc.file.filename, &fs))
                    {
                        perror(clc.file.filename);
                        fs.st_size = 0;
                    }
                    attr = pXMLDom->createAttribute(L"file_size_receive");
                    if(_ui64toa_s(fs.st_size, ui64buf, sizeof(ui64buf), 10))
                        attr->text = L"0";
                    else
                        attr->text = ui64buf;
                    receive_from->attributes->setNamedItem(attr);
                    instance->appendChild(receive_from);
                }
            }
        }
    }
    CATCH_COM_ERROR("add_instance()")
}

static int line_num = 0;
errno_t make_index(const CMOVE_LOG_CONTEXT &clc)
{
    const string study_uid(clc.file.studyUID);
    MSXML2::IXMLDOMDocument2 *pXMLDom = study_map[study_uid];
    if(pXMLDom == NULL)
        pXMLDom = create_xmldom(clc);
    if(pXMLDom)
    {
        study_map[study_uid] = pXMLDom;
        add_instance(pXMLDom, clc);
        //cout << (LPCSTR)pXMLDom->xml << endl;
        return 0;
    }
    else
        return ENOENT;
}

static bool merge_node(const _bstr_t &xpath, MSXML2::IXMLDOMNode *nodeSrc, MSXML2::IXMLDOMNode *nodeDest)
{
    if(opt_verbose) cerr << (LPCSTR)xpath << endl;

    if(nodeSrc->nodeName != nodeDest->nodeName)
    {
        cerr << "merge_node(" << (LPCSTR)nodeSrc->nodeName << ", " << (LPCSTR)nodeDest->nodeName << "): node name mismatch." << endl;
        return false;
    }

    MSXML2::IXMLDOMNamedNodeMapPtr attrs = nodeSrc->attributes;
    while(MSXML2::IXMLDOMNodePtr attr = attrs->nextNode())
    {
        if(attr->nodeName == _bstr_t(L"xmlns")) continue; // xmlns is read-only

        _bstr_t nodeName("@");
        nodeName += attr->nodeName;
        try
        {
            if(attr->text.length())
            {
                MSXML2::IXMLDOMNodePtr attr_dest = nodeDest->attributes->getNamedItem(attr->nodeName);
                if(attr_dest == NULL)
                {
                    attr_dest = nodeDest->ownerDocument->createAttribute(attr->nodeName);
                    nodeDest->appendChild(attr_dest);
                }
                attr_dest->Puttext(attr->text);
            }
            else
            {
                nodeDest->attributes->removeNamedItem(attr->nodeName);
            }
            /*
            cerr << (LPCSTR)nodeName << " : ";
            if(attr->text.length())
                cerr << (LPCSTR)attr->text;
            cerr << endl;
            */
        }
        CATCH_COM_ERROR("merge_node() node " << (LPCSTR)xpath << (LPCSTR)nodeName)
    }

    MSXML2::IXMLDOMNodeListPtr nodes = nodeSrc->childNodes;
    MSXML2::IXMLDOMNodePtr textNodeSrc;
    while(MSXML2::IXMLDOMNodePtr child = nodes->nextNode())
    {
        _bstr_t filter(L""), currentNodeXPath(xpath + _bstr_t(L"/") + child->nodeName);
        try
        {
            if(child->nodeType == MSXML2::DOMNodeType::NODE_TEXT)
            {
                textNodeSrc = child;
                continue;
            }
            else if(child->nodeType != MSXML2::DOMNodeType::NODE_ELEMENT)
            {
                if(opt_verbose) cerr << "ignore " << (LPCSTR)currentNodeXPath << " : " << child->nodeTypeString << endl;
                continue;
            }
            //else ergodic attributes and child nodes

            MSXML2::IXMLDOMNodePtr attr_id = child->attributes->getNamedItem(L"id");
            _bstr_t id_value;
            if(attr_id) id_value = attr_id->text;
            if(id_value.length())
                filter = _bstr_t(L"[@id=\"") + id_value + _bstr_t(L"\"]");
            currentNodeXPath += filter;
            MSXML2::IXMLDOMNodePtr child_dest = nodeDest->selectSingleNode(child->nodeName + filter);
            if(child_dest)
                merge_node(currentNodeXPath, child, child_dest);
            else
            {   
                nodeDest->appendChild(child->cloneNode(VARIANT_TRUE));
            }
        }
        CATCH_COM_ERROR("merge_node() node " << (LPCSTR)currentNodeXPath)
    }
    // process text node
    try
    {
        MSXML2::IXMLDOMNodeListPtr list_dest = nodeDest->childNodes;
        MSXML2::IXMLDOMNodePtr textNodeDest;
        while(MSXML2::IXMLDOMNodePtr it = list_dest->nextNode())
        {
            if(it->nodeType == MSXML2::DOMNodeType::NODE_TEXT)
            {
                textNodeDest = it;
                break;
            }
        }
        if(textNodeSrc)
        {
            _bstr_t node_value(textNodeSrc->nodeValue.bstrVal);
            if(opt_verbose) cerr << (LPCSTR)xpath << "/#text" << " : " << (LPCSTR)node_value << endl;
            if(textNodeDest == NULL)
                nodeDest->appendChild(nodeDest->ownerDocument->createTextNode(node_value));
            else
                textNodeDest->PutnodeValue(node_value);
        }
        else if(textNodeDest)
        {
            nodeDest->removeChild(textNodeDest);
        }
    }
    CATCH_COM_ERROR("merge_node() text node " << (LPCSTR)xpath << "/#text")
    return true;
}

void merge_index_study_patient_date(bool overwrite, std::map<std::string, LARGE_INTEGER> &map_move_study_status)
{
    for(map<string, LARGE_INTEGER>::iterator it = map_move_study_status.begin(); it != map_move_study_status.end(); ++it)
    {
        try
        {
            if(it->second.LowPart == ERROR_PATH_NOT_FOUND) continue;
            char dest_path[MAX_PATH];
            int offset = sprintf_s(dest_path, "%s\\pacs\\", COMMONLIB_PACS_BASE);
            char *src_path = dest_path + offset;
            sprintf_s(src_path, sizeof(dest_path) - offset, "indexdir\\0020000d\\%s.xml", it->first.c_str());
            MSXML2::IXMLDOMDocument2Ptr pSrc;
            HRESULT hr = pSrc.CreateInstance(__uuidof(MSXML2::DOMDocument30));
            if(VARIANT_FALSE == pSrc->load(src_path))
            {
                cerr << "merge_index_study_patient_date() can't load src xml " << src_path << endl;
                it->second.LowPart = ERROR_FILE_NOT_FOUND;
                continue;
            }
            pSrc->preserveWhiteSpace = VARIANT_FALSE;
	        pSrc->async = VARIANT_FALSE;

            char vol_id[16];
            sprintf_s(vol_id, "v%07d", it->second.HighPart);
            pSrc->documentElement->setAttribute(L"volume", vol_id);

            if(!PrepareFileDir(dest_path))
            {
                cerr << "merge_index_study_patient_date() can't PrepareFileDir(" << dest_path << ")" << endl;
                it->second.LowPart = ERROR_PATH_NOT_FOUND;
                continue;
            }
            if(_access_s(dest_path, 0) == 0)
            {
                ofstream merged_xml;
                MSXML2::IXMLDOMDocument2Ptr pDest;
                hr = pDest.CreateInstance(__uuidof(MSXML2::DOMDocument30));
                if(VARIANT_FALSE == pDest->load(dest_path))
                {
                    it->second.LowPart = ERROR_FILE_NOT_FOUND;
                    cerr << "merge_index_study_patient_date() can't load dest xml " << src_path << endl;
                    continue;
                }
                pDest->preserveWhiteSpace = VARIANT_FALSE;
	            pDest->async = VARIANT_FALSE;
                //hr = pDest->setProperty(L"SelectionLanguage", L"XPath");
                //hr = pDest->setProperty(L"SelectionNamespaces", "xmlns:p='http://www.kurumi.com.cn/xsd/study'");
                if(overwrite)
                {
                    MSXML2::IXMLDOMNodeListPtr instances = pDest->documentElement->selectNodes(L"//instance");
                    while(MSXML2::IXMLDOMNodePtr n = instances->nextNode())
                    {
                        MSXML2::IXMLDOMNodePtr attr = n->attributes->getNamedItem(L"state");
                        if(attr == NULL)
                        {
                            attr = pDest->createAttribute(L"state");
                            attr->nodeValue = L"new_ex";
                            n->appendChild(attr);
                        }
                        else
                            attr->nodeValue = L"new_ex";
                    }
                }

                if(merge_node(L"/study", pSrc->documentElement, pDest->documentElement))
                    calculate_size_cluster_aligned(pDest);
                else
                {
                    it->second.LowPart = ERROR_FILE_NOT_FOUND;
                    cerr << "merge_index_study_patient_date() merge study node failed" << endl;
                    continue;
                }
                
                merged_xml.open(dest_path, ios_base::out | ios_base::trunc, _SH_DENYNO);
                if(merged_xml.good())
                {
                    merged_xml << XML_HEADER << (LPCSTR)pDest->documentElement->xml << endl;
                    merged_xml.close();
                    cout << "trigger index_study " << dest_path << endl;
                    save_index_study_date(pDest);
                    save_index_patient(pDest);
                }
                else
                {
                    char msg[1024];
                    DWORD gle = GetLastError();
                    errno_t en = errno;
                    it->second.LowPart = ERROR_FILE_NOT_FOUND;
                    sprintf_s(msg, "merge_index_study_patient_date() save %s failed", dest_path);
                    displayErrorToCerr(msg, gle);
                    strerror_s(msg, en);
                    cerr << "merge_index_study_patient_date() save " << dest_path << " failed: " << msg << endl;
                }
                it->second.LowPart = 0;
            }
            else
            {   // not exist, save dom to dest xml
                FILE *dest_fp = NULL;
                errno_t en = 0;
                if(0 == (en = fopen_s(&dest_fp, dest_path, "w+")))
                {
                    fwrite(XML_HEADER, 1, strlen(XML_HEADER), dest_fp);
                    size_t xml_len = strlen((LPCSTR)pSrc->documentElement->xml);
                    char *buff = new char[xml_len + 2];  // 2 is \n + \0
                    if(0 == (en = strcpy_s(buff, xml_len + 2, (LPCSTR)pSrc->documentElement->xml)))
                    {
                        if(strlen(buff) == xml_len)
                        {
                            buff[xml_len] = '\n';
                            buff[xml_len + 1] = '\0';
                            fwrite(buff, 1, xml_len + 1, dest_fp);
                            fclose(dest_fp);
                            cout << "trigger index_study " << dest_path << endl;
                            save_index_study_date(pSrc);
                            save_index_patient(pSrc);
                        }
                        else
                        {
                            fclose(dest_fp);
                            it->second.LowPart = ERROR_FILE_NOT_FOUND;
                            cerr << "merge_index_study_patient_date() strcpy_s(xml) from " << dest_path << ": buffer size error." << endl;
                        }
                    }
                    else
                    {
                        char msg[1024];
                        fclose(dest_fp);
                        it->second.LowPart = ERROR_FILE_NOT_FOUND;
                        strerror_s(msg, en);
                        cerr << "merge_index_study_patient_date() strcpy_s(xml) from " << dest_path << " failed: " << msg << endl;
                    }
                }
                else
                {
                    char msg[1024];
                    strerror_s(msg, en);
                    cerr << "merge_index_study_patient_date() can't fopen_s(" << dest_path << ", w+): " << msg << endl;
                    it->second.LowPart = ERROR_FILE_NOT_FOUND;
                }
            }
        }
        CATCH_COM_ERROR("merge_index_study_patient_date()")
    }
}

#ifdef _DEBUG

COMMONLIB_API int test_for_make_index(bool verbose)
{
    opt_verbose = verbose;

    map<string, LARGE_INTEGER> map_move_study_status;
    LARGE_INTEGER state = {0, 0};
    map_move_study_status["CL\\6F\\47\\0L\\1.2.840.113619.2.55.3.2831208458.63.1326435165.930"] = state;
    map_move_study_status["J9\\DD\\O9\\GS\\1.2.840.113619.2.55.3.2831208458.315.1336457410.39"] = state;
    map_move_study_status["N3\\LE\\BX\\J5\\1.2.840.113619.2.55.3.2831208458.335.1327645840.955"] = state;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

    for(map<string, LARGE_INTEGER>::iterator it = map_move_study_status.begin(); it != map_move_study_status.end(); ++it)
    {
        string study_path;
        study_path.reserve(255);
        study_path.append(COMMONLIB_PACS_BASE).append("\\pacs\\indexdir\\000d0020\\").append(it->first).append(".xml");
        MSXML2::IXMLDOMDocument2Ptr pXMLDom;
        pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
        pXMLDom->load(study_path.c_str());
        save_index_patient(pXMLDom);
    }

    CoUninitialize();
    return 0;
}

#endif //_DEBUG
