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
        MSXML2::IXMLDOMNodePtr assoc = pXMLDom->selectSingleNode(L"/study/association");
        if(assoc == NULL) return;
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
    catch(_com_error &ex) 
	{
        cerr << "add_association() failed: " << ex.ErrorMessage() << ", " <<ex.Description() << endl;
	}
	catch(char * message)
	{
		cerr << "add_association() failed: " << message << endl;
	}
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
                MSXML2::IXMLDOMNodePtr hash_prefix = pXMLDom->selectSingleNode(L"/study/@hash_prefix");
                if(hash_prefix)
                    sprintf_s(xmlpath, "indexdir/000d0020/%s/", (LPCSTR)hash_prefix->text);
                else
                {
                    uidHash(it->first.c_str(), studyHash, sizeof(studyHash));
                    sprintf_s(xmlpath, "indexdir/000d0020/%c%c/%c%c/%c%c/%c%c/", studyHash[0], studyHash[1],
                        studyHash[2], studyHash[3], studyHash[4], studyHash[5], studyHash[6], studyHash[7]);
                }
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
}

static void add_instance(MSXML2::IXMLDOMDocument *pXMLDom, const CMOVE_LOG_CONTEXT &clc)
{
    try
    {
        char filter[128];
        MSXML2::IXMLDOMNodePtr root = pXMLDom->lastChild;
        if(root == NULL)
        {
            cerr << "XMLDOM can't find root element." << endl;
            return;
        }
        if(strlen(clc.assoc.id) > 0)
        {
            sprintf_s(filter, "association[@id='%s']", clc.assoc.id);
            MSXML2::IXMLDOMElementPtr asso = root->selectSingleNode(filter);
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
                    root->appendChild(asso);
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
                if(strcmp(clc.file.patientID, clc.patient.patientID) == 0)
                {
                    pat->setAttribute(L"name", clc.patient.patientsName);
                    pat->setAttribute(L"sex", clc.patient.sex);
                    pat->setAttribute(L"birthday", clc.patient.birthday);
                    pat->setAttribute(L"height", clc.patient.height);
                    pat->setAttribute(L"weight", clc.patient.weight);
                }
                root->appendChild(pat);
            }
        }

        bool new_series = false;
        char modality[17] = "OT";
        sprintf_s(filter, "series[@id='%s']", clc.file.seriesUID);
        MSXML2::IXMLDOMElementPtr series = root->selectSingleNode(filter);
        if(series == NULL)
        {
            series = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"series", L"http://www.kurumi.com.cn/xsd/study");
            if(series)
            {
                new_series = true;
                series->setAttribute(L"id", clc.file.seriesUID);
                if(strcmp(clc.file.seriesUID, clc.series.seriesUID) == 0 && strlen(clc.series.modality) > 0)
                {
                    strcpy_s(modality, clc.series.modality);
                    series->setAttribute(L"modality", modality);
                }
                else
                {   // get first token of file name as modality
                    size_t cnt = strchr(clc.file.filename, '.') - clc.file.filename;
                    if(cnt < sizeof(modality) && cnt > 0)
                        strncpy_s(modality, clc.file.filename, cnt);
                    series->setAttribute(L"modality", modality);
                }
            }
            else
            {
                cerr << "ERROR: create series node " << clc.file.seriesUID << " failed." << endl;
            }
        }

        if(series)
        {
            sprintf_s(filter, "instance[@id='%s']", clc.file.instanceUID);
            MSXML2::IXMLDOMElementPtr instance = series->selectSingleNode(filter);
            if(instance == NULL)
            {
                instance = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"instance", L"http://www.kurumi.com.cn/xsd/study");
                if(instance)
                {
                    instance->setAttribute(L"id", clc.file.instanceUID);
                    instance->setAttribute(L"xfer", clc.file.xfer);
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
            if(new_series)
            {
                root->appendChild(series);
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
