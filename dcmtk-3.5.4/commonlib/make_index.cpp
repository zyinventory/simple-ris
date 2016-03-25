#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"
#import <msxml3.dll>

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"GBK\"?>"

using namespace std;

static map<string, MSXML2::IXMLDOMDocument*> study_map, association_map;

static MSXML2::IXMLDOMDocument* create_xmldom(const CMOVE_LOG_CONTEXT &clc)
{
    try
    {
        MSXML2::IXMLDOMDocumentPtr pXMLDom;
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
            MSXML2::IXMLDOMDocument *pdom = pXMLDom.Detach();
            return pdom;
        }
        return NULL;
    }
	catch(_com_error &ex) 
	{
		cerr << "create_xmldom() failed: " << ex.ErrorMessage() << ", " <<ex.Description() << endl;
		return NULL;
	}
	catch(char * message)
	{
		cerr << "create_xmldom() failed: " << message << endl;
		return NULL;
	}
}

static void add_association(MSXML2::IXMLDOMDocument *pXMLDom)
{
    if(pXMLDom == NULL) return;
    try
    {
        MSXML2::IXMLDOMNodeListPtr pAssociations = pXMLDom->selectNodes(L"/study/associations/association");
        if(pAssociations == NULL) return;
        while(MSXML2::IXMLDOMNodePtr assoc = pAssociations->nextNode())
        {
            string assoc_id((LPCSTR)assoc->attributes->getNamedItem(L"id")->text);
            MSXML2::IXMLDOMDocument *pa = association_map[assoc_id];
            MSXML2::IXMLDOMDocumentPtr pDomAssoc;
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
                rec_assoc = pDomAssoc->selectSingleNode(L"/receive_association");
            }

            if(rec_assoc)
            {
                MSXML2::IXMLDOMNodePtr study = pXMLDom->selectSingleNode(L"/study");
                if(study) rec_assoc->appendChild(study->cloneNode(VARIANT_FALSE));
                pa = pDomAssoc.Detach();
                if(insert_new) association_map[assoc_id] = pa;
            }
        }
    }
    catch(_com_error &ex) 
	{
        cerr << "add_association() failed: " << ex.ErrorMessage() << ", " <<ex.Description() << endl;
	}
	catch(char * message)
	{
		cerr << "add_association() failed: " << message << endl;
	}
}

#define CLUSTER_SIZE 4096LL
#define ALIGNED_SIZE(x) (((x) & ~(CLUSTER_SIZE - 1LL)) + CLUSTER_SIZE)
void calculate_size_cluster_aligned(MSXML2::IXMLDOMDocument *pXMLDom)
{
    MSXML2::IXMLDOMNodeListPtr instances = pXMLDom->documentElement->selectNodes(L"//instance");
    __int64 size_aligned = 0;
    while(MSXML2::IXMLDOMNodePtr n = instances->nextNode())
    {
        __int64 fs = _atoi64((LPCSTR)n->attributes->getNamedItem(L"file_size")->text);
        size_aligned += ALIGNED_SIZE(fs);
    }
    pXMLDom->documentElement->setAttribute(L"study_size_cluster_aligned", size_aligned);
}

void clear_map()
{
    for(map<string, MSXML2::IXMLDOMDocument*>::iterator it = study_map.begin(); it != study_map.end(); ++it)
    {
        try
        {
            char studyHash[9], xmlpath[MAX_PATH];
            MSXML2::IXMLDOMDocument *pXMLDom = it->second;
            if(pXMLDom)
            {
                _bstr_t hash_prefix(pXMLDom->documentElement->getAttribute(L"hash_prefix").bstrVal);
                if(hash_prefix.length() > 0)
                    sprintf_s(xmlpath, "indexdir/000d0020/%s/", (LPCSTR)hash_prefix);
                else
                {
                    HashStr(it->first.c_str(), studyHash, sizeof(studyHash));
                    sprintf_s(xmlpath, "indexdir/000d0020/%c%c/%c%c/%c%c/%c%c/", studyHash[0], studyHash[1],
                        studyHash[2], studyHash[3], studyHash[4], studyHash[5], studyHash[6], studyHash[7]);
                }

                //calculate_size_cluster_aligned(pXMLDom);

                if(MkdirRecursive(xmlpath))
                {
                    strcat_s(xmlpath, it->first.c_str());
                    strcat_s(xmlpath, ".xml");
                    ofstream fxml(xmlpath, ios_base::trunc | ios_base::out, _SH_DENYNO);
                    if(fxml.good())
                    {
                        fxml << XML_HEADER << (LPCSTR)pXMLDom->xml << endl;
                        fxml.close();
                        add_association(pXMLDom);
                    }
                }
                else
                {
                    cerr << "can't save " << xmlpath << endl;
                }
                pXMLDom->Release();
            }
        }
	    catch(_com_error &ex) 
	    {
		    cerr << "clear_map() clear study failed: " << ex.ErrorMessage() << ", " <<ex.Description() << endl;
	    }
    }
    study_map.clear();

    for(map<string, MSXML2::IXMLDOMDocument*>::iterator it = association_map.begin(); it != association_map.end(); ++it)
    {
        MSXML2::IXMLDOMDocument *pDomAssoc = it->second;
        if(pDomAssoc == NULL) continue;
        try
        {
            MSXML2::IXMLDOMNodePtr assoc = pDomAssoc->selectSingleNode(L"/receive_association");
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
        catch(_com_error &ex) 
	    {
		    cerr << "clear_map() clear association failed: " << ex.ErrorMessage() << ", " <<ex.Description() << endl;
	    }
    }
    association_map.clear();

    char notify_file_name[MAX_PATH];
    GetNextUniqueNo("state\\", notify_file_name, sizeof(notify_file_name));
    strcat_s(notify_file_name, "_N.dfc");
    ofstream ntf(notify_file_name, ios_base::app | ios_base::out);
    if(ntf.good())
    {
        ntf << "N FFFFFFF0" << endl;
        ntf.close();
    }
    else
        cerr << "N FFFFFFF0" << endl;
}

static void add_instance(MSXML2::IXMLDOMDocument *pXMLDom, const CMOVE_LOG_CONTEXT &clc)
{
    try
    {
        char filter[128];
        MSXML2::IXMLDOMElementPtr root = pXMLDom->documentElement;
        if(root == NULL)
        {
            cerr << "XMLDOM can't find associations element." << endl;
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
                cerr << "ERROR: create patient node " << clc.file.patientID << " failed." << endl;
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
                cerr << "ERROR: create series node " << clc.file.seriesUID << " failed." << endl;
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
                    cerr << "ERROR: create instance node " << clc.file.instanceUID << " failed." << endl;
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
	catch(_com_error &ex) 
	{
		cerr << "add_instance() failed: " << ex.ErrorMessage() << ", " <<ex.Description() << endl;
	}
	catch(char * message)
	{
		cerr << "add_instance() failed: " << message << endl;
	}
}

static int line_num = 0;
errno_t make_index(const CMOVE_LOG_CONTEXT &clc)
{
    const string study_uid(clc.file.studyUID);
    MSXML2::IXMLDOMDocument *pXMLDom = study_map[study_uid];
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
