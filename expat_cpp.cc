#include <string.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <list>
#include <string>
#include <expat.h>
#include <vector>

using namespace std;

class XMLObj;

class XMLObjIter {
  typedef map<string, XMLObj *>::iterator map_iter_t;
  map_iter_t cur;
  map_iter_t end;
public:
  XMLObjIter();
  ~XMLObjIter();
  void set(const XMLObjIter::map_iter_t &_cur, const XMLObjIter::map_iter_t &_end);
  XMLObj *get_next();
};
class XMLObj
{
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

class RGWXMLParser : public XMLObj
{
  XML_Parser p;
  char *buf;
  int buf_len;
  XMLObj *cur_obj;
  vector<XMLObj *> objs;
protected:
  virtual XMLObj *alloc_obj(const char *el) = 0;
public:
  RGWXMLParser();
  virtual ~RGWXMLParser();
  bool init();
  bool xml_start(const char *el, const char **attr);
  bool xml_end(const char *el);
  void handle_data(const char *s, int len);

  bool parse(const char *buf, int len, int done);
  const char *get_xml() { return buf; }
  void set_failure() { success = false; }

private:
  bool success;
};

XMLObjIter::
XMLObjIter()
{
}

XMLObjIter::
~XMLObjIter()
{
}

void XMLObjIter::
set(const XMLObjIter::map_iter_t &_cur, const XMLObjIter::map_iter_t &_end)
{
  cur = _cur;
  end = _end;
}

XMLObj *XMLObjIter::
get_next()
{
  XMLObj *obj = NULL;
  if (cur != end) {
    obj = cur->second;
    ++cur;
  }
  return obj;
}

ostream& operator<<(ostream& out, XMLObj& obj) {
   out << obj.obj_type << ": " << obj.data;
   return out;
}

XMLObj::
~XMLObj()
{
}

bool XMLObj::
xml_start(XMLObj *parent, const char *el, const char **attr)
{
  this->parent = parent;
  obj_type = el;
    
  for (int i = 0; attr[i]; i += 2) {
    attr_map[attr[i]] = string(attr[i + 1]);
  }
  return true;
}

bool XMLObj::
xml_end(const char *el)
{
  return true;
}

void XMLObj::
xml_handle_data(const char *s, int len)
{
  data.append(s, len);
}

string& XMLObj::
XMLObj::get_data()
{
  return data;
}

XMLObj *XMLObj::
XMLObj::get_parent()
{
  return parent;
}

void XMLObj::
add_child(string el, XMLObj *obj)
{
  children.insert(pair<string, XMLObj *>(el, obj));
}

bool XMLObj::
get_attr(string name, string& attr)
{
  map<string, string>::iterator iter = attr_map.find(name);
  if (iter == attr_map.end())
    return false;
  attr = iter->second;
  return true;
}

XMLObjIter XMLObj::
find(string name)
{
  XMLObjIter iter;
  map<string, XMLObj *>::iterator first;
  map<string, XMLObj *>::iterator last;
  first = children.find(name);
  if (first != children.end()) {
    last = children.upper_bound(name);
  }else
    last = children.end();
  iter.set(first, last);
  return iter;
}

XMLObj *XMLObj::
find_first(string name)
{
  XMLObjIter iter;
  map<string, XMLObj *>::iterator first;
  first = children.find(name);
  if (first != children.end())
    return first->second;
  return NULL;
}

static void xml_start(void *data, const char *el, const char **attr) {

  RGWXMLParser *handler = static_cast<RGWXMLParser *>(data);
    
  if (!handler->xml_start(el, attr))
    handler->set_failure();
}
static void xml_end(void *data, const char *el) {
  RGWXMLParser *handler = static_cast<RGWXMLParser *>(data);
  if (!handler->xml_end(el))
    handler->set_failure();
  
}
static void handle_data(void *data, const char *s, int len)
{
  RGWXMLParser *handler = static_cast<RGWXMLParser *>(data);

  handler->handle_data(s, len);
}

RGWXMLParser::
RGWXMLParser() : buf(NULL), buf_len(0), cur_obj(NULL), success(true)
{
  p = XML_ParserCreate(NULL);
}

RGWXMLParser::
~RGWXMLParser()
{
  XML_ParserFree(p);

  free(buf);
  vector<XMLObj *>::iterator iter;
  for (iter = objs.begin(); iter != objs.end(); ++iter) {
    XMLObj *obj = *iter;
    delete obj;
  }
}

bool RGWXMLParser::xml_start(const char *el, const char **attr) {
  XMLObj * obj = alloc_obj(el);
  if (!obj) {
    obj = new XMLObj();
  }
  if (!obj->xml_start(cur_obj, el, attr))
    return false;
  if (cur_obj) {
    cur_obj->add_child(el, obj);
  } else {
    children.insert(pair<string, XMLObj *>(el, obj));
  }
  cur_obj = obj;

  objs.push_back(obj);
  return true;
}

bool RGWXMLParser::xml_end(const char *el) {
  XMLObj *parent_obj = cur_obj->get_parent();
  if (!cur_obj->xml_end(el))
    return false;
  cur_obj = parent_obj;
  return true;
}

void RGWXMLParser::handle_data(const char *s, int len)
{
  cur_obj->xml_handle_data(s, len);
}


bool RGWXMLParser::init()
{
  if (!p) {
    return false;
  }
  XML_SetElementHandler(p, ::xml_start, ::xml_end);
  XML_SetCharacterDataHandler(p, ::handle_data);
  XML_SetUserData(p, (void *)this);
  return true;
}

bool RGWXMLParser::parse(const char *_buf, int len, int done)
{
  int pos = buf_len;
  char *tmp_buf;
  tmp_buf = (char *)realloc(buf, buf_len + len);
  if (tmp_buf == NULL){
    free(buf);
    buf = NULL;
    return false;
  } else {
    buf = tmp_buf;
  }

  memcpy(&buf[buf_len], _buf, len);
  buf_len += len;

  success = true;
  if (!XML_Parse(p, &buf[pos], len, done)) {
    fprintf(stderr, "Parse error at line %d:\n%s\n",
	      (int)XML_GetCurrentLineNumber(p),
	      XML_ErrorString(XML_GetErrorCode(p)));
    success = false;
  }

  return success;
}
