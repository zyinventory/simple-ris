#include "stdafx.h"
#include "commonlib.h"
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

MSXML2::IXMLDOMDocument2* xml_index::create_study_dom(const NOTIFY_FILE_CONTEXT &clc)
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
    CATCH_COM_ERROR("xml_index::create_study_dom()", *pflog)
    return NULL;
}

MSXML2::IXMLDOMDocument2* xml_index::create_assoc_dom(const NOTIFY_FILE_CONTEXT &nfc)
{
    try
    {
        MSXML2::IXMLDOMDocument2Ptr pXMLDom;
        HRESULT hr = pXMLDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
        if(SUCCEEDED(hr))
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
            MSXML2::IXMLDOMDocument2 *pdom = pXMLDom.Detach();
            return pdom;
        }
    }
    CATCH_COM_ERROR("xml_index::create_assoc_dom()", *pflog)
    return NULL;
}

void xml_index::add_instance(MSXML2::IXMLDOMDocument2 *pXMLDom, const NOTIFY_FILE_CONTEXT &nfc)
{
    try
    {
        char filter[128];
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL)
            throw runtime_error("xml_index::add_instance() XMLDOM can't find associations element.");

        if(strcmp(nfc.file.studyUID, nfc.study.studyUID) == 0)
        {
            root->setAttribute(L"accession_number", nfc.study.accessionNumber);
            root->setAttribute(L"date", nfc.study.studyDate);
            root->setAttribute(L"time", nfc.study.studyTime);
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
                char instance_path[MAX_PATH];
                int path_len = sprintf_s(instance_path, "archdir\\v0000000\\%s\\%s\\%s", nfc.file.hash, nfc.file.studyUID, nfc.file.unique_filename);
                if(nfc.file.PathSeparator() == '/')
                    replace(instance_path, instance_path + path_len, '/', '\\');
                if(_stat64(instance_path, &fs))
                {
                    perror(instance_path);
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

                    string origin_file_path(nfc.assoc.path);
                    origin_file_path.append(1, '\\').append(nfc.file.filename);
                    if(_stat64(origin_file_path.c_str(), &fs))
                    {
                        perror(nfc.file.filename);
                        fs.st_size = 0;
                    }
                    if(_ui64toa_s(fs.st_size, ui64buf, sizeof(ui64buf), 10))
                        sprintf_s(ui64buf, "0");
                    receive_from->setAttribute(L"file_size_receive", ui64buf);
                }
            }
        }
    }
    CATCH_COM_ERROR("xml_index::add_instance()", *pflog)
}

static void add_study_to_assoc_dom(MSXML2::IXMLDOMDocument2 *pAssocDom, const string &study_uid, const string &assoc_id) throw(...) 
{
    if(pAssocDom) // append study node(only id attr) to assoc dom
    {
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
        throw runtime_error(msg.append(assoc_id));
    }
}

void xml_index::make_index(const NOTIFY_FILE_CONTEXT &nfc)
{
    string study_uid(nfc.file.studyUID), assoc_id(nfc.assoc.id);
    MSXML2::IXMLDOMDocument2* pStudyDom = NULL, *pAssocDom = NULL;
    try
    {
        XML_MAP::iterator it = map_xml_study.find(study_uid);
        if(it != map_xml_study.end()) pStudyDom = it->second;
        if(pStudyDom == NULL)
        {
            string xml_path(GetPacsBase());
            xml_path.append("\\pacs\\indexdir\\0020000d\\").append(nfc.file.hash).append(1, '\\').append(study_uid).append(".xml");
            MSXML2::IXMLDOMDocument2Ptr pdom;
            HRESULT hr = pdom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
            if(SUCCEEDED(hr))
            {
                if(pdom->load(xml_path.c_str()) == VARIANT_TRUE)
                    pStudyDom = pdom.Detach();
                else
                    pStudyDom = create_study_dom(nfc);

                if(pStudyDom) map_xml_study[study_uid] = pStudyDom;
            }
        }

        // find or create assoc dom
        it = map_xml_assoc.find(assoc_id);
        if(it == map_xml_assoc.end())
        {
            pAssocDom = create_assoc_dom(nfc);
            if(pAssocDom) map_xml_assoc[assoc_id] = pAssocDom;
        }
        else pAssocDom = it->second;
        
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
    string patientNameChs;

    try
    {
        if(pXMLDom == NULL) throw runtime_error("XMLDOM is NULL.");
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL) throw runtime_error("XMLDOM can't find root study element.");

        _bstr_t studyUid(root->getAttribute(L"id").bstrVal);
        if(studyUid.length() == 0) studyUid = L"";
        outbuff << "StudyUID=" << (LPCSTR)studyUid << endl;

        _bstr_t accessionNumber(root->getAttribute(L"accession_number").bstrVal);
        if(accessionNumber.length() == 0) accessionNumber = L"";
        outbuff << "AccessionNumber=" << (LPCSTR)accessionNumber << endl;

        _bstr_t studyDate(root->getAttribute(L"date").bstrVal);
        if(studyDate.length() == 0) studyDate = L"";
        outbuff << "StudyDate=" << (LPCSTR)studyDate << endl;

        _bstr_t studyTime(root->getAttribute(L"time").bstrVal);
        if(studyTime.length() == 0) studyTime = L"";
        outbuff << "StudyTime=" << (LPCSTR)studyTime << endl;

        _bstr_t modality(root->getAttribute(L"modality").bstrVal);
        if(modality.length() == 0) modality = L"";
        outbuff << "Modality=" << (LPCSTR)modality << endl;

        MSXML2::IXMLDOMElementPtr pat = root->selectSingleNode(L"patient");
        if(pat == NULL) throw runtime_error("XMLDOM can't find patient element.");

        patientId = pat->getAttribute(L"id").bstrVal;
        if(patientId.length() == 0) patientId = L"";
        outbuff << "PatientID=" << (LPCSTR)patientId << endl;

        if(GetSetting("RisIntegration", ris_path, sizeof(ris_path)))
        {
            char hash[9];
            HashStr((LPCSTR)patientId, hash, sizeof(hash));
            sprintf_s(ris_path, "%s\\pacs\\indexdir\\00100020\\%c%c\\%c%c\\%c%c\\%c%c\\%s_ris.txt", GetPacsBase(),
                hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], (LPCSTR)patientId);
            ifstream ris(ris_path);
            if(ris.good())
            {
                char line[1024];
                ris.getline(line, sizeof(line));
                while(ris.good())
                {
                    if(strlen(line) <= 0) goto ris_next_line;
                    outbuff << line << endl;
                    if(strstr(line, "PatientNameChs="))
                    {
                        char *p = strchr(line, '=');
                        if(p) ++p;
                        patientNameChs = p;
                        pat->setAttribute(L"PatientNameChs", patientNameChs.c_str());
                    }
ris_next_line:
                    ris.getline(line, sizeof(line));
                }
                ris.close();
            }
            else time_header_out(*pflog) << "xml_index::generate_replace_fields() open ris info failed: " << ris_path << endl;
        }

        _bstr_t patientName(pat->getAttribute(L"name").bstrVal);
        if(patientName.length() == 0) patientName = L"";
        outbuff << "PatientName=" << (LPCSTR)patientName << endl;

        _bstr_t sex(pat->getAttribute(L"sex").bstrVal);
        if(sex.length() == 0) sex = L"";
        if(sex == _bstr_t(L"M")) sex = L"ÄÐ";
        else if(sex == _bstr_t(L"F")) sex = L"Å®";
        else if(sex == _bstr_t(L"O")) sex = L"ÆäËû";
        else sex = L"";
        outbuff << "Gender=" << (LPCSTR)sex << endl;

        _bstr_t birthday(pat->getAttribute(L"birthday").bstrVal);
        if(birthday.length() == 0) birthday = L"";
        string bd((LPCSTR)birthday);
        if(bd.length() >= 8)
        {
            bd.insert(6, 1, '/');
            bd.insert(4, 1, '/');
        }
        outbuff << "Birthday=" << bd << endl;

        time_t now = 0;
        struct tm tm_now;
        time(&now);
        int age = 0;
        if(0 == localtime_s(&tm_now, &now))
        {
            age = atoi(bd.c_str());
            if(age) age = 1900 + tm_now.tm_year - age;
        }
        outbuff << "Age=" << age << endl;
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
                fxml << XML_HEADER << (LPCSTR)pAssocDom->xml << endl;
                fxml.close();
            }
        }
        pAssocDom->Release();
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

bool xml_index::save_index_study_date(MSXML2::IXMLDOMDocument2 *pDomStudy)
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
            //cout << "trigger index_study_date " << xml_path << endl;
        }
        else throw runtime_error(xml_path.insert(0, "save ").append(" error"));
        return true;
    }
    CATCH_COM_ERROR("save_index_study_date()", cerr)
    return false;
}

bool xml_index::save_index_patient(MSXML2::IXMLDOMDocument2 *pDomStudy)
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
            //cout << "trigger index_patient " << xml_path << endl;
        }
        else throw runtime_error(xml_path.insert(0, "save ").append(" error"));
        return true;
    }
    CATCH_COM_ERROR("save_index_patient()", cerr)
    return false;
}

bool xml_index::unload_and_sync_study(const std::string &study_uid)
{
    XML_MAP::iterator its = map_xml_study.find(study_uid);
    if(its == map_xml_study.end() || its->second == NULL) return false;
    try
    {
        char xml_path[MAX_PATH];
        MSXML2::IXMLDOMDocument2Ptr pStudyDom;
        HRESULT hr = pStudyDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
        if(FAILED(hr)) throw runtime_error("can't create IXMLDOMDocument2Ptr");

        calculate_size_cluster_aligned(its->second);

        if(save_study(study_uid, its->second))
        {
            pStudyDom.Attach(its->second); // ensure its->second shall release ptr
            map_xml_study.erase(its);
        }
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
                MSXML2::IXMLDOMNodePtr study = shallow_copy_study(pStudyDom->documentElement);
                if(study) ita->second->documentElement->appendChild(study);
            }
            
            // if all study nodes are complete, association is complete.
            MSXML2::IXMLDOMNodeListPtr nodes = ita->second->documentElement->selectNodes(filter_not_complete);
            if(nodes->length == 0) // association is complete
            {
                if(save_receive(ita->second)) ita = map_xml_assoc.erase(ita);
            }
            else ++ita;
        }

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
        save_study(it->first, it->second);
        if(it->second) it->second->Release();
        time_header_out(*pflog) << "xml_index::~xml_index() remain study " << it->first << endl;
        it = map_xml_study.erase(it);
    }

    it = map_xml_assoc.begin();
    while(it != map_xml_assoc.end())
    {
        save_receive(it->second);
        if(it->second) it->second->Release();
        time_header_out(*pflog) << "xml_index::~xml_index() remain association " << it->first << endl;
        it = map_xml_assoc.erase(it);
    }
}
