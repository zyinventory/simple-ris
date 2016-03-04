#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"
#import <msxml3.dll>

using namespace std;

static map<string, MSXML2::IXMLDOMDocument*> study_map;
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
            MSXML2::IXMLDOMElementPtr pRoot = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"study", L"http://www.kurumi.com.cn/xsd/study");
            if(pRoot)
            {
                hr = pRoot->setAttribute(L"id", clc.file.studyUID);
                if(strcmp(clc.file.studyUID, clc.study.studyUID) == 0)
                {
                    hr = pRoot->setAttribute(L"accession_number", clc.study.accessionNumber);
                    hr = pRoot->setAttribute(L"date", clc.study.studyDate);
                    hr = pRoot->setAttribute(L"time", clc.study.studyTime);
                }
                pXMLDom->appendChild(pRoot);
            }
            MSXML2::IXMLDOMDocument *pdom = pXMLDom.Detach();
            return pdom;
        }
        return NULL;
    }
	catch(_com_error &ex) 
	{
		cerr << ex.ErrorMessage() << endl;
		return NULL;
	}
	catch(char * message)
	{
		cerr << message << endl;
		return NULL;
	}
}

void clear_study_map()
{
    for(map<string, MSXML2::IXMLDOMDocument*>::iterator it = study_map.begin(); it != study_map.end(); ++it)
    {
        try
        {
            char studyHash[9], xmlpath[MAX_PATH];
            MSXML2::IXMLDOMDocument *pXMLDom = it->second;
            if(pXMLDom)
            {
                uidHash(it->first.c_str(), studyHash, sizeof(studyHash));
                sprintf_s(xmlpath, "indexdir\\000d0020\\%c%c\\%c%c\\%c%c\\%c%c\\", studyHash[0], studyHash[1],
                    studyHash[2], studyHash[3], studyHash[4], studyHash[5], studyHash[6], studyHash[7]);
                if(MkdirRecursive(xmlpath))
                {
                    strcat_s(xmlpath, it->first.c_str());
                    strcat_s(xmlpath, ".xml");
                    ofstream fxml(xmlpath, ios_base::trunc | ios_base::out, _SH_DENYNO);
                    if(fxml.good())
                    {
                        fxml << "<?xml version=\"1.0\" encoding=\"GBK\"?>" << (LPCSTR)pXMLDom->xml << endl;
                        fxml.close();
                    }
                }
                else
                {
                    cerr << "can't create path " << xmlpath << endl;
                }
                pXMLDom->Release();
            }
        }
	    catch(_com_error &ex) 
	    {
		    cerr << ex.ErrorMessage() << endl;
	    }
    }
    study_map.clear();
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
        sprintf_s(filter, "series[@id='%s']", clc.file.seriesUID);
        MSXML2::IXMLDOMElementPtr series = root->selectSingleNode(filter);
        if(series == NULL)
        {
            series = pXMLDom->createNode(MSXML2::NODE_ELEMENT, L"series", L"http://www.kurumi.com.cn/xsd/study");
            if(series)
            {
                new_series = true;
                series->setAttribute(L"id", clc.file.seriesUID);
                if(strcmp(clc.file.seriesUID, clc.series.seriesUID) == 0)
                    series->setAttribute(L"modality", clc.series.modality);
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
                    if(NULL == strstr((LPCSTR)attr->text, clc.series.modality))
                    {
                        attr->text += L",";
                        attr->text += clc.series.modality;
                    }
                }
                else
                {
                    MSXML2::IXMLDOMNodePtr attr = pXMLDom->createAttribute(L"modality");
                    attr->text = clc.series.modality;
                    root->attributes->setNamedItem(attr);
                }
            }
        }
    }
	catch(_com_error &ex) 
	{
		cerr << ex.ErrorMessage() << endl;
	}
	catch(char * message)
	{
		cerr << message << endl;
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
