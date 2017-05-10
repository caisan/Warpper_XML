#ifndef XML_PARSE_H
#define XML_PARSE_H

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <expat.h>

using namespace std;

class XMLObj;

class XMLObjIter {
    typedef map<string, XMLObj *>::iterator map_iter_t;
    map_iter_t cur;
    map_iter_t end;
public:
    XMLObjIter();
    ~XMLObjIter();
    void set(const XMLObjIter::map_iter_t &_cur, const XMLObjIter::map_iter_t *_end);
    XMLObj *get_next();
};
class XMLObj {
    XMLObj *parent;
    string obj_type;
protected:
    string data;
    multimap<string, XMLObj *> children;
    map<string, string> attr_map;
public:
    XMLObj() : parent(NULL) {}
    virtual ~XMLObj();
    bool xml_start(XMLObj *parent, const char *el, const char **attr);
    virtual bool xml_end(const char *el);
    virtual void xml_handle_data(const char *s, int len);
    string& get_data();
    XMLObj *get_parent();
    void add_child(string el, XMLObj *obj);
    bool get_attr(string name, string& attr);
    XMLObjIter find(string name);
    XMLObj *find_first(string name);
    friend ostream& operator<<(ostream& out, XMLObj& obj);
};
class XMLParser : public XMLObj {
    XML_Parser p;
    char *buf;
    int buf_len;
    XMLObj *cur_obj;
    vector<XMLObj *> objs;
protected:
    virtual XMLObj *alloc_obj(const char *el) = 0;
public:
    XMLParser();
    virtual ~XMLParser();
    bool init();
    bool xml_start(const char *el, const char **attr);
    void handle_data(const char *s, int len);
    bool parse(const char* buf, int len, int done);
    const char *get_xml() { return buf; }
    void set_failure() { success = false; }
private:
    bool success;
};

#endif
