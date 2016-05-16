#pragma once

#ifndef _XML_INDEX_CLASS_
#define _XML_INDEX_CLASS_

#include "notify_context.h"
#include <list>
#include <map>
#include <iterator>
#include <algorithm>
#import <msxml3.dll>

namespace handle_context
{
    typedef std::map<std::string, MSXML2::IXMLDOMDocument2*> XML_MAP;
    typedef std::pair<std::string, MSXML2::IXMLDOMDocument2*> XML_PAIR;

    class xml_index : public base_path
    {
    private:
        XML_MAP map_xml_study, map_xml_assoc;

        bool create_study_dom(const NOTIFY_FILE_CONTEXT &nfc, MSXML2::IXMLDOMDocument2Ptr &pXMLDom);
        bool create_assoc_dom(const NOTIFY_FILE_CONTEXT &nfc, MSXML2::IXMLDOMDocument2Ptr &pXMLDom);
        void add_instance(MSXML2::IXMLDOMDocument2Ptr &pXMLDom, const NOTIFY_FILE_CONTEXT &nfc);
        void generate_replace_fields(const std::string &replace_fields_path, MSXML2::IXMLDOMDocument2 *pXMLDom);
        bool save_index_study_date(MSXML2::IXMLDOMDocument2Ptr &pDomStudy);
        bool save_index_patient(MSXML2::IXMLDOMDocument2Ptr &pDomStudy);
        bool save_receive(MSXML2::IXMLDOMDocument2 *pAssocDom);
        bool save_study(const std::string &study_uid, MSXML2::IXMLDOMDocument2 *pStudyDom);

    public:
        static xml_index *singleton_ptr;

        xml_index(std::ostream *plog) : base_path("", plog) { if(plog == NULL) plog = &std::cerr; };
        xml_index(const xml_index &r) : base_path(r), map_xml_study(r.map_xml_study) {};
        virtual ~xml_index();
        xml_index& operator=(const xml_index &r);
        void make_index(const NOTIFY_FILE_CONTEXT &nfc);
        bool unload_and_sync_study(const std::string &study_uid);
        void find_all_study_uid(std::list<std::string> &uids) const;
    };
}

#endif
