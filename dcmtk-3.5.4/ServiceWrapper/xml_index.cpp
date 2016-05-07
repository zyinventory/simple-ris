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

xml_index& xml_index::operator=(const xml_index &r)
{
    base_path::operator=(r);
    copy(r.map_xml.cbegin(), r.map_xml.cend(), inserter(map_xml, map_xml.end()));
    return *this;
}

MSXML2::IXMLDOMDocument2* xml_index::create_xmldom(const NOTIFY_FILE_CONTEXT &clc)
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
    CATCH_COM_ERROR("xml_index::create_xmldom()", *pflog)
    return NULL;
}

void xml_index::add_instance(MSXML2::IXMLDOMDocument2 *pXMLDom, const NOTIFY_FILE_CONTEXT &nfc)
{
    try
    {
        char filter[128];
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL)
        {
            cerr << "xml_index::add_instance() XMLDOM can't find associations element." << endl;
            return;
        }
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
                cerr << "add_instance() create patient node " << nfc.file.patientID << " failed." << endl;
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
                cerr << "add_instance() create series node " << nfc.file.seriesUID << " failed." << endl;
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
                cerr << "add_instance() create instance node " << nfc.file.instanceUID << " failed." << endl;
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

void xml_index::make_index(const NOTIFY_FILE_CONTEXT &nfc)
{
    string study_uid(nfc.file.studyUID);
    MSXML2::IXMLDOMDocument2* pXMLDom = NULL;
    try
    {
        XML_MAP::iterator it = map_xml.find(study_uid);
        if(it != map_xml.end()) pXMLDom = it->second;
        if(pXMLDom == NULL)
        {
            string xml_path(GetPacsBase());
            xml_path.append("\\pacs\\indexdir\\0020000d\\").append(nfc.file.hash).append(1, '\\').append(study_uid).append(".xml");
            MSXML2::IXMLDOMDocument2Ptr pdom;
            HRESULT hr = pdom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
            if(SUCCEEDED(hr))
            {
                if(pdom->load(xml_path.c_str()) == VARIANT_TRUE)
                    pXMLDom = pdom.Detach();
                else
                    pXMLDom = create_xmldom(nfc);

                if(pXMLDom) map_xml[study_uid] = pXMLDom;
            }
        }

        if(pXMLDom) add_instance(pXMLDom, nfc);
    }
    CATCH_COM_ERROR("xml_index::make_index()", *pflog)
}

bool xml_index::unload_and_sync_study(const std::string &study_uid)
{
    XML_MAP::iterator it = map_xml.find(study_uid);
    if(it == map_xml.end() || it->second == NULL) return false;
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
        sprintf_s(xml_path + pos, sizeof(xml_path) - pos, "\\%s.xml", study_uid.c_str());

        ofstream fxml(xml_path, ios_base::trunc | ios_base::out, _SH_DENYNO);
        if(fxml.good())
        {
            fxml << XML_HEADER << (LPCSTR)it->second->documentElement->xml << endl;
            fxml.close();

            cout << "trigger index_study_uid " << xml_path << endl;

            it->second->Release();
            map_xml.erase(study_uid);
            return true;
        }
        else
        {
            string msg("save ");
            throw runtime_error(msg.append(xml_path).append(" error"));
        }
    }
    CATCH_COM_ERROR("xml_index::unload_and_sync_study()", *pflog)
    return false;
}
