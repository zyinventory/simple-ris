#include "stdafx.h"
#import <msxml3.dll>

using namespace std;

static int start_write_log = 0;
static bool inFile = false;
#define XML_HEADER "<?xml version=\"1.0\" encoding=\"GBK\"?>"

static void close_cmdfile(ofstream &cmdfile)
{
    cmdfile.close();
    inFile = false;
    ++start_write_log;
    Sleep(10 + rand() % 50);
}

static void test_sim_slow_log_writer(void*)
{
    ifstream strmlog("cmove.txt", ios_base::in, _SH_DENYNO);
    if(strmlog.fail())
    {
        start_write_log = -1;
        displayErrorToCerr("test_sim_slow_log_writer()", GetLastError());
        return;
    }
    char buf[1024];
    int fnpos = 0;
    ofstream cmdfile;
    do {
        char fn[MAX_PATH];
        strmlog.getline(buf, sizeof(buf));
        if(strmlog.eof()) break;
        switch(buf[0])
        {
        case 'M':
        case 'T':
            fnpos = GetNextUniqueNo("state\\", fn, sizeof(fn));
            sprintf_s(fn + fnpos, sizeof(fn) - fnpos, "_%c.dfc", buf[0]);
            if(inFile) close_cmdfile(cmdfile);
            cmdfile.open(fn, ios_base::out | ios_base::trunc, _SH_DENYRW);
            inFile = !cmdfile.fail();
            if(inFile)
            {
                cmdfile << buf << endl;
                close_cmdfile(cmdfile);
            }
            else cerr << "can't understand how to write " << buf << " to file " << fn << endl;
            break;
        case 'F': // F tag must be coupled !!!
        case 'N': // N tag must be coupled !!!
            if(inFile)
            {
                cmdfile << buf << endl;
                close_cmdfile(cmdfile);
                inFile = false;
            }
            else
            {
                int fnpos = GetNextUniqueNo("state\\", fn, sizeof(fn));
                sprintf_s(fn + fnpos, sizeof(fn) - fnpos, "_%c.dfc", buf[0]);
                if(inFile) close_cmdfile(cmdfile);
                cmdfile.open(fn, ios_base::out | ios_base::trunc, _SH_DENYRW);
                inFile = !cmdfile.fail();
                if(inFile) cmdfile << buf << endl;
                else cerr << "can't understand how to write " << buf << " to file " << fn << endl;
            }
            break;
        default:
            if(inFile) cmdfile << buf << endl;
            else cerr << "can't understand how to write " << buf << " to file " << fn << endl;
        }
    } while(!strmlog.eof());
    if(inFile) cmdfile.close();
    strmlog.close();
}
/*
static void test_consume_log(const char *sid)
{
    char log_name[MAX_PATH];
    sprintf_s(log_name, "%s\\cmove.txt", sid);

    ifstream tail(log_name, ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << log_name << endl;
        return;
    }

    while(true)
    {
        if(const char *buff = try_read_line(tail))
        {
            cout << buff << endl;
            if(buff[0] == 'M') break;
        }
        else
            Sleep(1);
    }
    tail.close();
}
*/

void overwrite_study_archdir(const char *pacs_base, map<string, DWORD> &map_move_study_status)
{
    list<string> study_dirs, dir_files;
    struct _finddata_t wfd;
    intptr_t hSearch = _findfirst("archdir\\*", &wfd);
    if(hSearch == -1)
    {
        perror("move_study_dir() failed");
        fprintf_s(stderr, "\tfind %s failed\n", "archdir\\*");
        return;
    }
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0 || node.compare("DICOMDIR") == 0) 
			continue; // skip . .. DICOMDIR
        if(wfd.attrib & _A_SUBDIR)
            study_dirs.push_back(node);
        else
            dir_files.push_back(node);
	} while(_findnext(hSearch, &wfd) == 0);
	_findclose(hSearch);

    map<string, string> map_studies;
    for(list<string>::iterator it_study = study_dirs.begin(); it_study != study_dirs.end(); ++it_study)
    {
        string dir_filename(*it_study);
        dir_filename.append(".dir");
        list<string>::iterator it_dicomdir = find_if(dir_files.begin(), dir_files.end(),
            [&dir_filename](const string &fn) { return fn.compare(dir_filename) == 0; });
        if(it_dicomdir != dir_files.end())
        {
            map_studies[*it_study] = dir_filename;
            dir_files.erase(it_dicomdir);
        }
        else
            cerr << "study " << *it_study << " can't find matched DICOMDIR " << dir_filename << endl;
    }
    for_each(dir_files.begin(), dir_files.end(), [](const string &fn)
        { cerr << "dicomdir " << fn << " remain, there is no matched study UID." << endl; });

    for(map<string, string>::iterator it = map_studies.begin(); it != map_studies.end(); ++it)
    {
        char prefix[16], src_path[MAX_PATH], dest_path[MAX_PATH];
        HashStr(it->first.c_str(), prefix, sizeof(prefix));
        sprintf_s(src_path, "archdir\\%s", it->first.c_str());
        sprintf_s(dest_path, "%s\\pacs\\archdir\\%c%c\\%c%c\\%c%c\\%c%c\\%s", pacs_base, 
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->first.c_str());

        DWORD gle = 0;
        // move study dir
        bool study_moved = false;
        if(DeleteTree(dest_path, &cerr))
        {
            if(PrepareFileDir(dest_path))
            {
                if(rename(src_path, dest_path))
                {
                    perror("move_study_dir() rename study dir failed");
                    cerr << "\t" << src_path << " -> " << dest_path << endl;
                }
                else
                {
                    study_moved = true;
                    cout << "trigger archive " << it->first << " " << dest_path << endl;
                }
            }
            else
                cerr << "move_study_dir() can't PrepareFileDir(" << dest_path << ")" << endl;
        }
        else
            cerr << "move_study_dir() can't delete dir " << dest_path << endl;

        if(!study_moved)
        {
            gle = ERROR_PATH_NOT_FOUND;
            goto report_study_status;
        }

        // move dicomdir
        bool dicomdir_moved = false;
        strcat_s(src_path, ".dir");
        strcat_s(dest_path, ".dir");
        errno_t ec = _unlink(dest_path);
        if(ec == 0 || errno == ENOENT)
        {
            if(rename(src_path, dest_path))
            {
                perror("move_study_dir() rename dicomdir failed");
                cerr << "\t" << src_path << " -> " << dest_path << endl;
            }
            else
            {
                dicomdir_moved = true;
                cout << "trigger dicomdir " << it->first << ".dir " << dest_path << endl;
            }
        }
        else
            cerr << "move_study_dir() can't delete dicomdir " << dest_path << endl;

        if(!dicomdir_moved) gle = ERROR_FILE_NOT_FOUND;

report_study_status:
        sprintf_s(dest_path, "%c%c\\%c%c\\%c%c\\%c%c\\%s", 
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->first.c_str());
        map_move_study_status[dest_path] = gle;
    }
}

bool merge_node(const _bstr_t &xpath, MSXML2::IXMLDOMNode *nodeSrc, MSXML2::IXMLDOMNode *nodeDest)
{
    //cerr << (LPCSTR)xpath << endl;

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
        catch(_com_error & come)
        {
            cerr << "merge_node() node " << (LPCSTR)xpath <<(LPCSTR)nodeName << " COM error: " << come.ErrorMessage();
            _bstr_t desc = come.Description();
            if(desc.length())
                cerr << ", " << (LPCSTR)desc;
            cerr << endl;
        }
        catch(...)
        {
            cerr << "merge_node() node " << (LPCSTR)xpath << (LPCSTR)nodeName << " unknown error" << endl;
        }
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
                cerr << "ignore " << (LPCSTR)currentNodeXPath << " : " << child->nodeTypeString << endl;
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
        catch(_com_error & come)
        {
            cerr << "merge_node() node " << (LPCSTR)currentNodeXPath << " COM error: " << come.ErrorMessage();
            _bstr_t desc = come.Description();
            if(desc.length())
                cerr << ", " << (LPCSTR)desc;
            cerr << endl;
        }
        catch(...)
        {
            cerr << "merge_node() node " << (LPCSTR)currentNodeXPath << " unknown error" << endl;
        }
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
            //cerr << (LPCSTR)xpath << "/#text" << " : " << (LPCSTR)node_value << endl;
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
    catch(_com_error & come)
    {
        cerr << "merge_node() text node " << (LPCSTR)xpath << "/#text COM error: " << come.ErrorMessage();
        _bstr_t desc = come.Description();
        if(desc.length())
            cerr << ", " << (LPCSTR)desc;
        cerr << endl;
    }
    catch(...)
    {
        cerr << "merge_node() text node " << (LPCSTR)xpath << "/#text unknown error" << endl;
    }
    return true;
}

void merge_study_index(const char *pacs_base, bool overwrite, map<string, DWORD> &map_move_study_status)
{
    for(map<string, DWORD>::iterator it = map_move_study_status.begin(); it != map_move_study_status.end(); ++it)
    {
        try
        {
            if(it->second == ERROR_PATH_NOT_FOUND) continue;
            char dest_path[MAX_PATH];
            int offset = sprintf_s(dest_path, "%s\\pacs\\", pacs_base);
            char *src_path = dest_path + offset;
            sprintf_s(src_path, sizeof(dest_path) - offset, "indexdir\\000d0020\\%s.xml", it->first.c_str());
            if(!PrepareFileDir(dest_path))
            {
                cerr << "overwrite_index_study() can't PrepareFileDir(" << dest_path << ")" << endl;
                continue;
            }
            if(_access_s(dest_path, 0) == 0)
            {
                ofstream merged_xml;
                MSXML2::IXMLDOMDocument2Ptr pSrc, pDest;
                HRESULT hr = pSrc.CreateInstance(__uuidof(MSXML2::DOMDocument30));
                hr = pDest.CreateInstance(__uuidof(MSXML2::DOMDocument30));
                if(VARIANT_FALSE == pSrc->load(src_path)) goto study_xml_merge_over;
                if(VARIANT_FALSE == pDest->load(dest_path)) goto study_xml_merge_over;
                pSrc->preserveWhiteSpace = VARIANT_FALSE;
	            pSrc->async = VARIANT_FALSE;
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
                            attr->nodeValue = L"off_ex"; // remote is offline and local is exclusive
                            n->appendChild(attr);
                        }
                        else
                            attr->nodeValue = L"off_ex";
                    }
                }

                if(!merge_node(L"/study", pSrc->documentElement, pDest->documentElement))
                    cerr << "overwrite_index_study() merge study node failed" << endl;
                
                merged_xml.open(dest_path, ios_base::out | ios_base::trunc, _SH_DENYNO);
                if(merged_xml.good())
                {
                    merged_xml << XML_HEADER << (LPCSTR)pDest->documentElement->xml << endl;
                    merged_xml.close();
                }
                else
                {
                    DWORD gle = GetLastError();
                    errno_t en = errno;
                    char msg[1024];
                    sprintf_s(msg, "merge_study_index() save %s failed", dest_path);
                    displayErrorToCerr(msg, gle);
                    strerror_s(msg, en);
                    cerr << "merge_study_index() save " << dest_path << " failed: " << msg << endl;
                }
    study_xml_merge_over:
                if(pSrc == NULL) cerr << "overwrite_index_study() can't load src xml " << src_path << endl;
                if(pDest == NULL) cerr << "overwrite_index_study() can't load dest xml " << dest_path << endl;
            }
            else
            {   // not exist, copy src xml to dest
                FILE *dest_fp = NULL, *src_fp = NULL;
                errno_t en = fopen_s(&src_fp, src_path, "r");
                if(en)
                {
                    char msg[1024];
                    strerror_s(msg, en);
                    cerr << "overwrite_index_study() can't fopen_s(" << src_path << "): " << msg << endl;
                    continue;
                }
                if(fopen_s(&dest_fp, dest_path, "w+") == 0)
                {
                    char *buff = new char[4096];
                    size_t read_bytes = 0;
                    while(read_bytes = fread(buff, 1, 4096, src_fp))
                        fwrite(buff, 1, read_bytes, dest_fp);
                    delete[] buff;
                    fclose(dest_fp);
                    cout << "trigger index_study " << dest_path << endl;
                }
                else
                {
                    char msg[1024];
                    strerror_s(msg, en);
                    cerr << "overwrite_index_study() can't fopen_s(" << dest_path << ", w+): " << msg << endl;
                }
                fclose(src_fp);
            }
        }
        catch(_com_error & come)
        {
            cerr << "overwrite_index_study() COM error: " << come.ErrorMessage();
            _bstr_t desc = come.Description();
            if(desc.length())
                cerr << ", " << (LPCSTR)desc;
            cerr << endl;
        }
        catch(...)
        {
            cerr << "overwrite_index_study() unknown error" << endl;
        }
    }
}

template<class Pred> static void find_recursive(const string &input_path, list<string> &collector, Pred pred)
{
    string path(input_path);
    if(path.empty()) path = ".";
    string::size_type old_size = path.size();
    string::iterator it = path.end();
    --it;
    if(*it != '\\') path.append(1, '\\');
    path.append(1, '*');
    it = path.end();
    --it; // it -> '*'

    struct __finddata64_t wfd;
    intptr_t hSearch = _findfirst64(path.c_str(), &wfd);
    if(hSearch == -1)
    {
        perror("find_recursive() failed");
        fprintf_s(stderr, "\tfind %s failed\n", path.c_str());
        return;
    }
    path.erase(it);
    do {
        if(wfd.attrib & _A_SUBDIR)
        {
            if(strcmp(wfd.name, ".") == 0 || strcmp(wfd.name, "..") == 0) continue;
            size_t old_len = path.length();
            path.append(wfd.name);
            find_recursive(path, collector, pred);
            path.resize(old_len);
        }
        else  // file
        {
            if(pred(&wfd))
            {
                size_t old_len = path.length();
                path.append(wfd.name);
                collector.push_back(path);
                path.resize(old_len);
            }
        }
	} while(_findnext64(hSearch, &wfd) == 0);
	_findclose(hSearch);
}

static bool is_ext_file(const struct __finddata64_t *wfd, const char *ext)
{
    if((wfd->attrib & _A_SUBDIR) == 0)
    {
        const char * p = strstr(wfd->name, ext);
        if(p && strcmp(p, ext) == 0) return true;
    }
    return false;
}

static void find_recursive_xml_file(string &input_path, list<string> &collector)
{
    find_recursive(input_path, collector, bind2nd(ptr_fun(is_ext_file), ".xml"));
}

void move_index_receive(const char *pacs_base, map<string, bool> &map_receive_index)
{
    list<string> collector;
    string path("indexdir\\receive");
    //find_recursive(path, collector, [](const struct __finddata64_t *wfd) { return is_ext_file(wfd, ".xml"); });
    find_recursive_xml_file(path, collector);

    for(list<string>::iterator it = collector.begin(); it != collector.end(); ++it)
    {
        bool move_ok = false;
        char dest_path[MAX_PATH];
        sprintf_s(dest_path, "%s\\pacs\\%s", pacs_base, it->c_str());
        if(PrepareFileDir(dest_path))
        {
            if(_unlink(dest_path) == 0 || errno == ENOENT)
            {
                if(rename(it->c_str(), dest_path))
                {
                    char msg[1024];
                    strerror_s(msg, errno);
                    cerr << "move_index_receive() move " << *it << " to " << dest_path << " failed: " << msg << endl;
                }
                else
                {
                    move_ok = true;
                    cout << "trigger index_receive " << dest_path << endl;
                }
            }
            else
            {
                char msg[1024];
                strerror_s(msg, errno);
                cerr << "move_index_receive() delete " << dest_path << " failed: " << msg << endl;
            }
        }
        else
            cerr << "move_index_receive() can't mkdir " << dest_path << endl;
        map_receive_index[*it] = move_ok;
    }
}

void call_process_log(const std::string &storedir, const std::string &sessionId)
{
    char src_name[MAX_PATH];
    sprintf_s(src_name, "%s\\%s", storedir.c_str(), sessionId.c_str());
    if(_chdir(src_name))
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "chdir to " << src_name << " failed: " << msg << endl;
        return;
    }
    if(_mkdir("state") && errno != EEXIST)
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "mkdir state faile: " << msg << endl;
        return;
    }
    HANDLE ht = (HANDLE)_beginthread(test_sim_slow_log_writer, 0, NULL);
    //test_sim_slow_log_writer((void*)sessionId.c_str());
    /*
    int i = 10;
    while(start_write_log == 0 && i < 1000)
    {
        Sleep(i);
        i += 10;
    }
    */
    if(start_write_log >= 0) // start_write_log == 0, start immediately
    {
        scp_store_main_loop(sessionId.c_str(), false);
        //test_consume_log(sessionId.c_str());

        const char *pacs_base = "C:\\usr\\local\\dicom";
        map<string, DWORD> map_move_study_status;
        overwrite_study_archdir(pacs_base, map_move_study_status);

        // test for overwrite_study_index()
        //map_move_study_status["CL\\6F\\47\\0L\\1.2.840.113619.2.55.3.2831208458.63.1326435165.930"] = 0;
        //map_move_study_status["J9\\DD\\O9\\GS\\1.2.840.113619.2.55.3.2831208458.315.1336457410.39"] = 0;
        //map_move_study_status["N3\\LE\\BX\\J5\\1.2.840.113619.2.55.3.2831208458.335.1327645840.955"] = 0;
        
        merge_study_index(pacs_base, true, map_move_study_status);

        map<string, bool> map_receive_index;
        //move_index_receive(pacs_base, map_receive_index);

        // todo: add study xml to patient xml and study date xml
    }
}

void clear_resource()
{
    ReleaseUniqueNoResource();
}
