#include <io.h>
#include <time.h>
#include <sys/stat.h>
#include <string>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <fstream>

#include <windows.h>
#include <atlbase.h>
#include "commonlib.h"
#include "../include/dcmtk/dcmdata/xml_index.h"

extern bool opt_verbose;

/*
instance state : [backup state]_[include state]
backup state:
    new : new instance, remote does not exist, local exist
    old : old instance, remote does not exist, local does not exist
    on  : online, remote exist, local exist
    off : offline, remote exist, local does not exist
include state:
    in : include
    ex : exclude
*/
#define XML_HEADER "<?xml version=\"1.0\" encoding=\"GBK\"?>"

using namespace std;
using namespace handle_context;

xml_index* xml_index::singleton_ptr = NULL;

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

xml_index& xml_index::operator=(const xml_index &r)
{
    base_path::operator=(r);
    copy(r.map_xml_study.cbegin(), r.map_xml_study.cend(), inserter(map_xml_study, map_xml_study.end()));
    return *this;
}

bool xml_index::create_study_dom(const NOTIFY_FILE_CONTEXT &clc, MSXML2::IXMLDOMDocument2Ptr &pXMLDom)
{
    try
    {
        pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
	    pXMLDom->async = VARIANT_FALSE;
        pXMLDom->appendChild(pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"study", L"http://www.kurumi.com.cn/xsd/study"));
        MSXML2::IXMLDOMElementPtr pRoot = pXMLDom->documentElement;
        if(pRoot)
        {
            HRESULT hr = pRoot->setAttribute(L"id", clc.file.studyUID);
            if(clc.file.PathSeparator() == '/')
                hr = pRoot->setAttribute(L"hash_prefix", clc.file.hash);
            else
            {
                char buff[sizeof(clc.file.hash)];
                strcpy_s(buff, clc.file.hash);
                replace(buff, buff + sizeof(clc.file.hash), '\\', '/');
                hr = pRoot->setAttribute(L"hash_prefix", buff);
            }
            hr = pRoot->setAttribute(L"state", L"new_in");
            if(strcmp(clc.file.studyUID, clc.study.studyUID) == 0)
            {
                hr = pRoot->setAttribute(L"accession_number", clc.study.accessionNumber);
                hr = pRoot->setAttribute(L"date", clc.study.studyDate);
                hr = pRoot->setAttribute(L"time", clc.study.studyTime);
                if(strlen(clc.study.studyID)) hr = pRoot->setAttribute(L"study_id", clc.study.studyID);
            }
                
            MSXML2::IXMLDOMNodePtr pAssociations = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"associations", L"http://www.kurumi.com.cn/xsd/study");
            pXMLDom->documentElement->appendChild(pAssociations);
            return true;
        }
        else time_header_out(*pflog) << __FUNCSIG__" study dom's documentElement is NULL." << endl;
    }
    CATCH_COM_ERROR("xml_index::create_study_dom()", *pflog)
    return false;
}

bool xml_index::create_assoc_dom(const NOTIFY_FILE_CONTEXT &nfc, MSXML2::IXMLDOMDocument2Ptr &pXMLDom)
{
    try
    {
        pXMLDom->preserveWhiteSpace = VARIANT_FALSE;
	    pXMLDom->async = VARIANT_FALSE;
        pXMLDom->appendChild(pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"receive_association", L"http://www.kurumi.com.cn/xsd/study"));
        MSXML2::IXMLDOMElementPtr asso = pXMLDom->documentElement;
        if(asso)
        {
            asso->setAttribute(L"id", nfc.assoc.id);
            asso->setAttribute(L"calling_ae", nfc.assoc.callingAE);
            asso->setAttribute(L"calling_address", nfc.assoc.callingAddr);
            asso->setAttribute(L"called_ae", nfc.assoc.calledAE);
            asso->setAttribute(L"called_address", nfc.assoc.calledAddr);
            asso->setAttribute(L"port", nfc.assoc.port);
        }
        return true;
    }
    CATCH_COM_ERROR("xml_index::create_assoc_dom()", *pflog)
    return false;
}

void xml_index::add_instance(MSXML2::IXMLDOMDocument2Ptr &pXMLDom, const NOTIFY_FILE_CONTEXT &nfc)
{
    try
    {
        char filter[128];
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL)
            throw runtime_error("xml_index::add_instance() XMLDOM can't find document element.");

        if(strcmp(nfc.file.studyUID, nfc.study.studyUID) == 0)
        {
            root->setAttribute(L"accession_number", nfc.study.accessionNumber);
            root->setAttribute(L"date", nfc.study.studyDate);
            root->setAttribute(L"time", nfc.study.studyTime);
            if(strlen(nfc.study.studyID)) root->setAttribute(L"study_id", nfc.study.studyID);
        }

        if(strlen(nfc.assoc.id) > 0)
        {
            MSXML2::IXMLDOMNodePtr assocCollection = root->selectSingleNode(L"associations");
            if(assocCollection)
            {
                sprintf_s(filter, "association[@id='%s']", nfc.assoc.id);
                MSXML2::IXMLDOMElementPtr asso = assocCollection->selectSingleNode(filter);
                if(asso == NULL)
                {
                    asso = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"association", L"http://www.kurumi.com.cn/xsd/study");
                    if(asso)
                    {
                        asso->setAttribute(L"id", nfc.assoc.id);
                        asso->setAttribute(L"calling_ae", nfc.assoc.callingAE);
                        asso->setAttribute(L"calling_address", nfc.assoc.callingAddr);
                        asso->setAttribute(L"called_ae", nfc.assoc.calledAE);
                        asso->setAttribute(L"called_address", nfc.assoc.calledAddr);
                        asso->setAttribute(L"port", nfc.assoc.port);
                        assocCollection->appendChild(asso);
                    }
                }
            }
        }

        sprintf_s(filter, "patient[@id='%s']", nfc.file.patientID);
        MSXML2::IXMLDOMElementPtr pat = root->selectSingleNode(filter);
        if(pat == NULL)
        {
            pat = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"patient", L"http://www.kurumi.com.cn/xsd/study");
            if(pat)
            {
                pat->setAttribute(L"id", nfc.file.patientID);
                char hash[16] = "", prefix[16] = "", enc[128];
                HashStr(nfc.file.patientID, hash, sizeof(hash));
                sprintf_s(prefix, "%c%c/%c%c/%c%c/%c%c", hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7]);
                pat->setAttribute(L"hash_prefix", prefix);
                EncodeBase32(nfc.file.patientID, enc, sizeof(enc));
                pat->setAttribute(L"encoded", enc);
                root->appendChild(pat);
            }
            else
                *pflog << "add_instance() create patient node " << nfc.file.patientID << " failed." << endl;
        }
        if(pat && strcmp(nfc.patient.patientID, nfc.file.patientID) == 0)
        {
            pat->setAttribute(L"name", nfc.patient.patientsName);
            pat->setAttribute(L"sex", nfc.patient.sex);
            pat->setAttribute(L"birthday", nfc.patient.birthday);
            pat->setAttribute(L"height", nfc.patient.height);
            pat->setAttribute(L"weight", nfc.patient.weight);
        }

        char modality[17] = "";
        sprintf_s(filter, "series[@id='%s']", nfc.file.seriesUID);
        MSXML2::IXMLDOMElementPtr series = root->selectSingleNode(filter);
        if(series == NULL)
        {
            series = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"series", L"http://www.kurumi.com.cn/xsd/study");
            if(series)
            {
                series->setAttribute(L"id", nfc.file.seriesUID);
                root->appendChild(series);
            }
            else
                *pflog << "add_instance() create series node " << nfc.file.seriesUID << " failed." << endl;
        }
        if(series && strcmp(nfc.file.seriesUID, nfc.series.seriesUID) == 0)
        {
            if(strlen(nfc.series.modality) > 0)
            {
                strcpy_s(modality, nfc.series.modality);
            }
            else
            {   // get first token of file name as modality
                size_t cnt = strchr(nfc.file.filename, '.') - nfc.file.filename;
                if(cnt < sizeof(modality) && cnt > 0)
                    strncpy_s(modality, nfc.file.filename, cnt);
            }
            series->setAttribute(L"modality", modality);
            series->setAttribute(L"number", nfc.series.number);

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
            long long saved_size = 0;
            sprintf_s(filter, "instance[@id='%s']", nfc.file.instanceUID);
            MSXML2::IXMLDOMElementPtr instance = series->selectSingleNode(filter);
            if(instance == NULL)
            {
                instance = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"instance", L"http://www.kurumi.com.cn/xsd/study");
                instance->setAttribute(L"id", nfc.file.instanceUID);
                series->appendChild(instance);
            }
            if(instance)
            {
                instance->setAttribute(L"xfer", nfc.file.xfer_new);
                instance->setAttribute(L"charset", nfc.file.charset);
                instance->setAttribute(L"number", nfc.file.number);
                if(strlen(nfc.file.sopClassUID)) instance->setAttribute(L"sop_class_uid", nfc.file.sopClassUID);
                char instance_path[MAX_PATH];
                int path_len = sprintf_s(instance_path, "%s\\pacs\\archdir\\v0000000\\%s\\%s\\%s", GetPacsBase(), nfc.file.hash, nfc.file.studyUID, nfc.file.unique_filename);
                if(nfc.file.PathSeparator() == '/')
                    replace(instance_path, instance_path + path_len, '/', '\\');
                if(_stat64(instance_path, &fs))
                {
                    char msg[256];
                    strerror_s(msg, errno);
                    time_header_out(*pflog) << __FUNCSIG__" _stat64(" << instance_path << ") fail: " << msg << endl;
                    fs.st_size = 0;
                }
                saved_size = fs.st_size;
                instance->setAttribute(L"file_size", saved_size);
                instance->setAttribute(L"state", fs.st_size ? L"new_in" : L"new_ex");
                if(nfc.file.PathSeparator() == '/')
                    instance->setAttribute(L"url", nfc.file.unique_filename);
                else
                {
                    char buff[sizeof(nfc.file.unique_filename)];
                    strcpy_s(buff, nfc.file.unique_filename);
                    replace(buff, buff + sizeof(buff), '\\', '/');
                    instance->setAttribute(L"url", buff);
                }
            }
            else
            {
                *pflog << "add_instance() create instance node " << nfc.file.instanceUID << " failed." << endl;
            }

            if(strlen(nfc.assoc.id) > 0)
            {
                sprintf_s(filter, "receive_from[@id='%s']", nfc.assoc.id);
                MSXML2::IXMLDOMElementPtr receive_from = instance->selectSingleNode(filter);
                if(receive_from == NULL)
                {
                    receive_from = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"receive_from", L"http://www.kurumi.com.cn/xsd/study");
                    receive_from->setAttribute(L"id", nfc.assoc.id);
                    instance->appendChild(receive_from);
                }
                if(receive_from)
                {
                    receive_from->setAttribute(L"xfer_receive", nfc.file.xfer);
                    receive_from->setAttribute(L"xfer_save", nfc.file.xfer_new);

                    char ui64buf[32];
                    if(_ui64toa_s(saved_size, ui64buf, sizeof(ui64buf), 10))
                        sprintf_s(ui64buf, "0");
                    receive_from->setAttribute(L"file_size_save", ui64buf);

                    if(_ui64toa_s(nfc.file.file_size_receive, ui64buf, sizeof(ui64buf), 10))
                        sprintf_s(ui64buf, "0");
                    receive_from->setAttribute(L"file_size_receive", ui64buf);
                }
            }
        }
    }
    CATCH_COM_ERROR("xml_index::add_instance()", *pflog)
}

static void add_study_to_assoc_dom(MSXML2::IXMLDOMDocument2Ptr &pAssocDom, const string &study_uid, const string &assoc_id) throw(...) 
{
    if(pAssocDom) // append study node(only id attr) to assoc dom
    {
        if(opt_verbose) time_header_out(cerr) << __FUNCSIG__" start." << endl;
        _bstr_t filter(L"study[@id='");
        filter += study_uid.c_str();
        filter += L"']";
        MSXML2::IXMLDOMNodePtr ps = pAssocDom->documentElement->selectSingleNode(filter);
        if(ps == NULL)
        {
            ps = pAssocDom->createNode(MSXML2::NODE_ELEMENT, L"study", L"http://www.kurumi.com.cn/xsd/study");
            if(ps)
            {
                MSXML2::IXMLDOMNodePtr attr_id = pAssocDom->createAttribute(L"id");
                attr_id->text = study_uid.c_str();
                ps->attributes->setNamedItem(attr_id);
                pAssocDom->documentElement->appendChild(ps);
            }
            else
            {
                string msg("can't create study node ");
                throw runtime_error(msg.append(study_uid).append(" in assoc dom ").append(assoc_id));
            }
        }
    }
    else
    {
        string msg("can't find or create association dom ");
        throw runtime_error(msg.append(assoc_id).append(", pAssocDom is NULL."));
    }
}

void xml_index::make_index(const NOTIFY_FILE_CONTEXT &nfc)
{
    string study_uid(nfc.file.studyUID), assoc_id(nfc.assoc.id);
    try
    {
        MSXML2::IXMLDOMDocument2Ptr pStudyDom, pAssocDom;
        HRESULT hr = pStudyDom.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
        if(FAILED(hr)) throw _com_error(hr);
        hr = pAssocDom.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
        if(FAILED(hr)) throw _com_error(hr);

        XML_MAP::iterator it = map_xml_study.find(study_uid);
        if(it != map_xml_study.end())
        {
            pStudyDom.Attach(it->second, true); // addref
            if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" study dom is found: " << study_uid << endl;
        }
        else
        {
            string xml_path(GetPacsBase());
            xml_path.append("\\pacs\\indexdir\\0020000d\\").append(nfc.file.hash).append(1, '\\').append(study_uid).append(".xml");
            if(pStudyDom->load(xml_path.c_str()) == VARIANT_FALSE) create_study_dom(nfc, pStudyDom);
            else
            {
                if(pStudyDom && pStudyDom->documentElement)
                {
                    hr = pStudyDom->documentElement->setAttribute(L"state", L"new_in");
                    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" study dom load OK: " << xml_path << endl;
                }
                else time_header_out(*pflog) << __FUNCSIG__" study dom load failed, documentElement is NULL: " << xml_path << endl;
            }
            if(pStudyDom)
            {
                pStudyDom.AddRef();
                map_xml_study[study_uid] = pStudyDom;
            }
        }

        // find or create assoc dom
        it = map_xml_assoc.find(assoc_id);
        if(it == map_xml_assoc.end())
        {
            if(opt_verbose) time_header_out(*pflog) << "association " << assoc_id << " is not found, create_assoc_dom()." << endl;
            if(create_assoc_dom(nfc, pAssocDom))
            {
                if(opt_verbose) time_header_out(*pflog) << "association " << assoc_id << " create OK." << endl;
                pAssocDom.AddRef();
                map_xml_assoc[assoc_id] = pAssocDom;
            }
            else if(opt_verbose) time_header_out(*pflog) << "association " << assoc_id << " create failed." << endl;
        }
        else pAssocDom.Attach(it->second, true); // add ref
        
        if(pStudyDom) add_instance(pStudyDom, nfc);
        else
        {
            string msg("can't find or create study dom ");
            throw runtime_error(msg.append(study_uid));
        }

        add_study_to_assoc_dom(pAssocDom, study_uid, assoc_id);
    }
    CATCH_COM_ERROR("xml_index::make_index()", *pflog);
}

void xml_index::generate_replace_fields(const string &replace_fields_path, MSXML2::IXMLDOMDocument2 *pXMLDom)
{
    ostringstream outbuff;
    _bstr_t patientId;
    char ris_path[MAX_PATH];
    
    try
    {
        if(pXMLDom == NULL) throw runtime_error("XMLDOM is NULL.");
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL) throw runtime_error("XMLDOM can't find root study element.");

        _bstr_t studyUid(root->getAttribute(L"id").bstrVal);
        if(studyUid.length() == 0) studyUid = L"";
        outbuff << "study_uid=" << (LPCSTR)studyUid << endl;

        MSXML2::IXMLDOMElementPtr pat = root->selectSingleNode(L"patient");
        if(pat == NULL) throw runtime_error("XMLDOM can't find patient element.");

        patientId = pat->getAttribute(L"id").bstrVal;
        if(patientId.length() == 0) patientId = L"";
        outbuff << "patient_id=" << (LPCSTR)patientId << endl;

        ris_path[0] = '\0';
        size_t ris_prog = GetSetting("RisIntegration", ris_path, sizeof(ris_path));

        if(ris_prog)
        {
            char hash[9];
            HashStr((LPCSTR)patientId, hash, sizeof(hash));
            sprintf_s(ris_path, "%s\\pacs\\indexdir\\00100020\\%c%c\\%c%c\\%c%c\\%c%c\\%s_ris.txt", GetPacsBase(),
                hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], (LPCSTR)patientId);
            ifstream ris_patient(ris_path);
            if(ris_patient.good())
            {
                char line[1024] = "";
                ris_patient.getline(line, sizeof(line));
                while(ris_patient.good())
                {
                    if(strlen(line) <= 0) goto ris_patient_next_line;
                    char *p = strchr(line, '=');
                    if(p)
                    {
                        *p++ = '\0';
                        if(strlen(p))
                        {
                            pat->setAttribute(line, p);
                            if(opt_verbose) time_header_out(*pflog) << "xml_index::generate_replace_fields() set " << line << " = " << p << endl;
                        }
                    }
                    else time_header_out(*pflog) << "xml_index::generate_replace_fields(): " << ris_path << ", bad line: " << line << endl;
ris_patient_next_line:
                    line[0] = '\0';
                    ris_patient.getline(line, sizeof(line));
                }
                ris_patient.close();
            }
            else time_header_out(*pflog) << "xml_index::generate_replace_fields() open ris info failed: " << ris_path << endl;

            HashStr((LPCSTR)studyUid, hash, sizeof(hash));
            sprintf_s(ris_path, "%s\\pacs\\indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s_ris.txt", GetPacsBase(),
                hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], (LPCSTR)studyUid);
            ifstream ris_study(ris_path);
            if(ris_study.good())
            {
                char line[1024] = "";
                ris_study.getline(line, sizeof(line));
                while(ris_study.good())
                {
                    if(strlen(line) <= 0) goto ris_study_next_line;
                    char *p = strchr(line, '=');
                    if(p)
                    {
                        *p++ = '\0';
                        if(strlen(p))
                        {
                            root->setAttribute(line, p);
                            if(opt_verbose) time_header_out(*pflog) << "xml_index::generate_replace_fields() set " << line << " = " << p << endl;
                        }
                    }
                    else time_header_out(*pflog) << "xml_index::generate_replace_fields(): " << ris_path << ", bad line: " << line << endl;
ris_study_next_line:
                    line[0] = '\0';
                    ris_study.getline(line, sizeof(line));
                }
                ris_study.close();
            }
            else time_header_out(*pflog) << "xml_index::generate_replace_fields() open ris info failed: " << ris_path << endl;
        }

        MSXML2::IXMLDOMNamedNodeMapPtr attrs = pat->Getattributes();
        MSXML2::IXMLDOMNodePtr attr;
        while(attr = attrs->nextNode())
        {
            if(wcscmp(attr->nodeName, L"id") && wcscmp(attr->nodeName, L"hash_prefix")
                && wcscmp(attr->nodeName, L"encoded") && wcscmp(attr->nodeName, L"xmlns"))
            {
                if(wcscmp(attr->nodeName, L"sex") == 0)
                {
                    _bstr_t sex(attr->nodeValue.bstrVal);
                    if(sex.length() == 0) sex = L"";
                    if(sex == _bstr_t(L"M")) sex = L"ÄÐ";
                    else if(sex == _bstr_t(L"F")) sex = L"Å®";
                    else if(sex == _bstr_t(L"O")) sex = L"ÆäËû";
                    else sex = L"";
                    outbuff << "sex=" << (LPCSTR)sex << endl;
                }
                else if(wcscmp(attr->nodeName, L"birthday") == 0)
                {
                    _bstr_t birthday(attr->nodeValue.bstrVal);
                    if(birthday.length() == 0) birthday = L"";
                    string bd((LPCSTR)birthday);
                    if(bd.length() >= 8)
                    {
                        bd.insert(6, 1, '/');
                        bd.insert(4, 1, '/');
                    }
                    outbuff << "birthday=" << bd << endl;

                    time_t now = 0;
                    struct tm tm_now;
                    time(&now);
                    int age = 0;
                    if(0 == localtime_s(&tm_now, &now))
                    {
                        age = atoi(bd.c_str());
                        if(age) age = 1900 + tm_now.tm_year - age;
                    }
                    outbuff << "age=" << age << endl;
                }
                else
                    outbuff << (LPCSTR)(attr->nodeName) << "=" << (LPCSTR)_bstr_t(attr->nodeValue.bstrVal) << endl;
            }
        }

        attrs = root->Getattributes();
        while(attr = attrs->nextNode())
        {
            if(wcscmp(attr->nodeName, L"id") && wcscmp(attr->nodeName, L"hash_prefix")
                && wcscmp(attr->nodeName, L"encoded") && wcscmp(attr->nodeName, L"xmlns"))
                outbuff << (LPCSTR)(attr->nodeName) << "=" << (LPCSTR)_bstr_t(attr->nodeValue.bstrVal) << endl;
        }
    }
    CATCH_COM_ERROR("xml_index::generate_replace_fields()", *pflog);

    string fields_content(outbuff.str());
    if(fields_content.length() == 0)
    {
        time_header_out(*pflog) << "xml_index::generate_replace_fields() failed, content is empty." << endl;
        return;
    }

    ofstream fxml(replace_fields_path, ios_base::trunc | ios_base::out, _SH_DENYNO);
    if(fxml.fail())
    {
        time_header_out(*pflog) << "xml_index::generate_replace_fields() open " << replace_fields_path << " failed." << endl;
        return;
    }
    fxml << fields_content;
    fxml.flush();
    fxml.close();
    //cout << "trigger index_study_uid_fields " << replace_fields_path << endl;
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

bool xml_index::save_receive(MSXML2::IXMLDOMDocument2 *pAssocDom)
{
    if(pAssocDom == NULL) return true;
    try
    {
        string xmlpath((LPCSTR)pAssocDom->documentElement->attributes->getNamedItem(L"id")->text);
        xmlpath.insert(8, 1, '\\').insert(6, 1, '\\').insert(4, 1, '\\')
            .insert(0, "\\pacs\\indexdir\\receive\\").insert(0, GetPacsBase()).append(".xml");
        if(PrepareFileDir(xmlpath.c_str()))
        {
            ofstream fxml(xmlpath.c_str(), ios_base::trunc | ios_base::out, _SH_DENYNO);
            if(fxml.good())
            {
                fxml << XML_HEADER << (LPCSTR)pAssocDom->xml << endl;
                fxml.close();
            }
        }
        else if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" PrepareFileDir(" << xmlpath << ") failed." << endl;
        return true;
    }
    CATCH_COM_ERROR("xml_index::save_receive()", *pflog);
    return false;
}

bool xml_index::save_study(const string &study_uid, MSXML2::IXMLDOMDocument2 *pStudyDom)
{
    if(pStudyDom == NULL) return true;
    try
    {
        char xml_path[MAX_PATH], hash[9];
        HashStr(study_uid.c_str(), hash, sizeof(hash));
        int pos = sprintf_s(xml_path, "%s\\pacs\\indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c", GetPacsBase(),
            hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7]);
        if(!MkdirRecursive(xml_path))
        {
            sprintf_s(xml_path + pos, sizeof(xml_path) - pos, " mkdir error", study_uid.c_str());
            throw runtime_error(xml_path);
        }
        sprintf_s(xml_path + pos, sizeof(xml_path) - pos, "\\%s.txt", study_uid.c_str());

        if(opt_verbose) time_header_out(*pflog) << "xml_index::save_study() ready to generate_replace_fields()" << endl;
        generate_replace_fields(xml_path, pStudyDom);

        char *p = strrchr(xml_path, '.');
        if(p) // replace .txt to .xml
        {
            *p = '\0';
            strcat_s(xml_path, ".xml");
        }
        ofstream fxml(xml_path, ios_base::trunc | ios_base::out, _SH_DENYNO);
        if(fxml.good())
        {
            fxml << XML_HEADER << (LPCSTR)pStudyDom->documentElement->xml << endl;
            fxml.close();
            //cout << "trigger index_study_uid_xml " << xml_path << endl;
            return true;
        }
        else
        {
            string msg("save ");
            throw runtime_error(msg.append(xml_path).append(" error"));
        }
    }
    CATCH_COM_ERROR("xml_index::save_study()", *pflog);
    return false;
}

bool xml_index::save_index_study_date(MSXML2::IXMLDOMDocument2Ptr &pDomStudy)
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
        xml_path.insert(6, 1, '\\').insert(4, 1, '\\').insert(0, "\\pacs\\indexdir\\00080020\\").insert(0, GetPacsBase()).append(".xml");
        if(!PrepareFileDir(xml_path.c_str())) throw logic_error(xml_path.insert(0, "PrepareFileDir ").append(" failed"));
        MSXML2::IXMLDOMDocument2Ptr pDom;
        pDom.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
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
            //cout << "trigger index_study_date " << xml_path << endl;
        }
        else throw runtime_error(xml_path.insert(0, "save ").append(" error"));
        return true;
    }
    CATCH_COM_ERROR("save_index_study_date()", *pflog)
    return false;
}

bool xml_index::save_index_patient(MSXML2::IXMLDOMDocument2Ptr &pDomStudy)
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

        string xml_path(GetPacsBase());
        xml_path.append("\\pacs\\indexdir\\00100020\\").append(hash_prefix_str)
            .append(1, '\\').append((LPCSTR)encoded).append(".xml");
        if(!PrepareFileDir(xml_path.c_str())) throw logic_error(xml_path.insert(0, "PrepareFileDir ").append(" failed"));

        MSXML2::IXMLDOMDocument2Ptr pDom;
        pDom.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
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
            //cout << "trigger index_patient " << xml_path << endl;
        }
        else throw runtime_error(xml_path.insert(0, "save ").append(" error"));
        return true;
    }
    CATCH_COM_ERROR("save_index_patient()", *pflog)
    return false;
}

void xml_index::find_all_study_uid(std::list<std::string> &uids) const
{
    std::transform(map_xml_study.begin(), map_xml_study.end(), back_inserter(uids),
        [](const XML_PAIR &p) { return p.first; });
}

bool xml_index::unload_and_sync_study(const std::string &study_uid)
{
    XML_MAP::iterator its = map_xml_study.find(study_uid);
    if(its == map_xml_study.end() || its->second == NULL) return false;
    try
    {
        MSXML2::IXMLDOMDocument2 *pdom = its->second;
        calculate_size_cluster_aligned(pdom);

        if(save_study(study_uid, pdom)) map_xml_study.erase(its);
        else throw runtime_error("can't save study");

        // try complete association
        _bstr_t filter_not_complete(L"study[not(@hash_prefix)]"), filter(L"study[@id='");
        filter += study_uid.c_str();
        filter += L"']";
        XML_MAP::iterator ita = map_xml_assoc.begin();
        while(ita != map_xml_assoc.end())
        {
            //replace study node in receive_association.
            MSXML2::IXMLDOMNodePtr ps = ita->second->documentElement->selectSingleNode(filter);
            if(ps)
            {
                ita->second->documentElement->removeChild(ps);
                // shallow copy study node
                MSXML2::IXMLDOMNodePtr study = shallow_copy_study(pdom->documentElement);
                if(study) ita->second->documentElement->appendChild(study);
            }
            
            // if all study nodes are complete, association is complete.
            MSXML2::IXMLDOMNodeListPtr nodes = ita->second->documentElement->selectNodes(filter_not_complete);
            if(nodes->length == 0) // association is complete
            {
                if(save_receive(ita->second))
                {
                    if(ita->second) ita->second->Release();
                    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" map_xml_assoc.erase(" << ita->first << ")." << endl;
                    ita = map_xml_assoc.erase(ita);
                }
                else if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" map_xml_assoc.erase(" << ita->first << ") failed." << endl;
            }
            else ++ita;
        }

        MSXML2::IXMLDOMDocument2Ptr pStudyDom;
        HRESULT hr = pStudyDom.CreateInstance(__uuidof(MSXML2::DOMDocument30), NULL, CLSCTX_INPROC_SERVER);
        if(FAILED(hr)) throw runtime_error("can't create IXMLDOMDocument2Ptr");

        pStudyDom.Attach(pdom, false); // don't add ref

        if(opt_verbose) time_header_out(*pflog) << "xml_index::unload_and_sync_study() ready to save study date and patient." << endl;
        save_index_study_date(pStudyDom);
        save_index_patient(pStudyDom);
        return true;
    }
    CATCH_COM_ERROR("xml_index::unload_and_sync_study()", *pflog);
    return false;
}

xml_index::~xml_index()
{
    XML_MAP::iterator it = map_xml_study.begin();
    while(it != map_xml_study.end())
    {
        time_header_out(*pflog) << "xml_index::~xml_index() remain study " << it->first << endl;
        save_study(it->first, it->second);
        if(it->second) it->second->Release();
        it = map_xml_study.erase(it);
    }

    it = map_xml_assoc.begin();
    while(it != map_xml_assoc.end())
    {
        time_header_out(*pflog) << "xml_index::~xml_index() remain association " << it->first << endl;
        save_receive(it->second);
        if(it->second) it->second->Release();
        it = map_xml_assoc.erase(it);
    }
}
