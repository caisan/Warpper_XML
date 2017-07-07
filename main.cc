#include "expat_cpp.cc"
#include <string.h>
class BLLoggingEnabled
{
protected:
   bool status;
   string target_bucket;
   string target_prefix;

public:
   bool target_bucket_specified;
   bool target_prefix_specified;

   BLLoggingEnabled() : status(false), target_bucket_specified(false),
                               target_prefix_specified(false) {};
   ~BLLoggingEnabled(){};
    void set_true() {
      status = true;
    }
    void set_false() {
      status = false;
    }

    bool is_true() const {
      return status == true;
    }

    string get_target_bucket() const {
      return target_bucket;
    }

    string get_target_prefix() const {
      return target_prefix;
    }

    void set_target_bucket(string _bucket) {
      target_bucket =  _bucket;
    }

    void set_target_prefix(string _prefix) {
      target_prefix =  _prefix;
    }
};

class RGWBucketLoggingStatus
{
protected:

  public:
  BLLoggingEnabled enabled;

  RGWBucketLoggingStatus() : enabled() {}
  virtual ~RGWBucketLoggingStatus() {}
  bool is_enabled() const {
       return enabled.is_true();
  }

  string get_target_prefix() const {
      return enabled.get_target_prefix();
  }

  string get_target_bucket() const {
      return enabled.get_target_bucket();
  }
};

class BLTargetBucket_S3 : public XMLObj
{
public:
  BLTargetBucket_S3() {}
  ~BLTargetBucket_S3() override {}
  string& to_str() { return data; }
};

class BLTargetPrefix_S3 : public XMLObj
{
public:
  BLTargetPrefix_S3() {}
  ~BLTargetPrefix_S3() override {}
  string& to_str() { return data; }
};

class BLLoggingEnabled_S3 : public BLLoggingEnabled, public XMLObj
{
public:
  BLLoggingEnabled_S3() : BLLoggingEnabled() {}
  ~BLLoggingEnabled_S3() override {}
  string& to_str() { return data; }

  bool xml_end(const char *el) override;
};

class RGWBLXMLParser_S3 : public RGWXMLParser
{

  XMLObj *alloc_obj(const char *el) override;
public:
  RGWBLXMLParser_S3() {}
};

class RGWBucketLoggingStatus_S3 : public RGWBucketLoggingStatus, public XMLObj
{
public:
  RGWBucketLoggingStatus_S3() : RGWBucketLoggingStatus() {}
  ~RGWBucketLoggingStatus_S3() override {}

  bool xml_end(const char *el) override;
  void to_xml(ostream& out)
  {
    out << "<BucketLoggingStatus xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
    if (is_enabled()) {
      out << "<LoggingEnabled>";

      string _bucket = this->get_target_bucket();
      out << "<TargetBucket>" << _bucket << "</TargetBucket>";

      string _prefix = this->get_target_prefix();
      out << "<TargetPrefix>" << _prefix << "</TargetPrefix>";

      out << "</LoggingEnabled>";
    }
    out << "</BucketLoggingStatus>";
  }

};
bool BLLoggingEnabled_S3::xml_end(const char *el) {
  BLTargetBucket_S3 *bl_target_bucket;
  BLTargetPrefix_S3 *bl_target_prefix;

  target_bucket.clear();
  target_prefix.clear();
  
  bl_target_bucket = static_cast<BLTargetBucket_S3 *>(find_first("TargetBucket"));
  if (bl_target_bucket) {
    target_bucket = bl_target_bucket->get_data();
    target_bucket_specified = true;
  }

  bl_target_prefix = static_cast<BLTargetPrefix_S3 *>(find_first("TargetPrefix"));
  if (bl_target_prefix) {
    target_prefix = bl_target_prefix->get_data();
    target_prefix_specified = true;
  }

  return true;
}

bool RGWBucketLoggingStatus_S3::xml_end(const char *el) {
  BLLoggingEnabled_S3 *bl_enabled = static_cast<BLLoggingEnabled_S3 *>(find_first("LoggingEnabled"));

  if (bl_enabled) {
    enabled.set_true();
    if (bl_enabled->target_bucket_specified) {
      enabled.target_bucket_specified = true;
      enabled.set_target_bucket(bl_enabled->get_target_bucket());
    }
    if (bl_enabled->target_prefix_specified) {
      enabled.target_prefix_specified = true;
      enabled.set_target_prefix(bl_enabled->get_target_prefix());
    }
  } else {
    enabled.set_false();
  }

  return true;
}
XMLObj *RGWBLXMLParser_S3::alloc_obj(const char *el)
{
  XMLObj *obj = nullptr;
  if (strcmp(el, "BucketLoggingStatus") == 0) {
    obj = new RGWBucketLoggingStatus_S3();
  } else if (strcmp(el, "LoggingEnabled") == 0) {
    obj = new BLLoggingEnabled_S3();
  } else if (strcmp(el, "TargetBucket") == 0) {
    obj = new BLTargetBucket_S3();
  } else if (strcmp(el, "TargetPrefix") == 0) {
    obj = new BLTargetPrefix_S3();
  }
  return obj;
}

int main()
{
    RGWBucketLoggingStatus_S3 *status = NULL;
    RGWBucketLoggingStatus_S3 new_status;
    RGWBLXMLParser_S3 parser;

    if(!parser.init()) {
        return 0;
    }
    char* const xml_data = (char*)malloc( sizeof(char)*1000  );
    const char *data = " \
<BucketLoggingStatus> \
      <LoggingEnabled> \
           <TargetBucket>mybucketlogs</TargetBucket> \
           <TargetPrefix>mybucket-access_log-/</TargetPrefix>\
           <TargetGrants>\
              <Grant>\
                  <Grantee>\
                     <EmailAddress>user@company.com</EmailAddress>\
                  </Grantee>\
                  <Permission>READ</Permission>\
              </Grant>\
           </TargetGrants>\
     </LoggingEnabled>\
</BucketLoggingStatus>";

    strncpy(xml_data, data, strlen(data));
    size_t len = strlen(data);
    if (!parser.parse(xml_data,len, 1)) {
        return 0;
    }

    status = static_cast<RGWBucketLoggingStatus_S3*>(parser.find_first("BucketLoggingStatus"));
    bool is_enable = status->is_enabled();
    string target_prefix = status->get_target_prefix();
      
    string target_bucket = status->get_target_bucket() ;
    if(!status)
    {
        return 0;
    }

    return 0;

}
