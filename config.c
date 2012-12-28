#include "config.h"

#include "avahi-helper.h"


cConfig<cAvahiServicesConfig> cAvahiServicesConfig::Services;


cAvahiServicesConfig::cAvahiServicesConfig(void)
{
  Clear();
}

cAvahiServicesConfig::~cAvahiServicesConfig(void)
{
}

void cAvahiServicesConfig::Clear(void)
{
  _line = NULL;
  _is_valid = false;
  _name = NULL;
  _protocol = AVAHI_PROTO_UNSPEC;
  _type = NULL;
  _port = -1;
  _subtypes.Clear();
  _txts.Clear();
}

bool cAvahiServicesConfig::Parse(const char *s)
{
  Clear();
  _line = s;
  const char *tmp = skipspace(s);
  if ((tmp != NULL) && (tmp[0] != '#')) {
     cAvahiHelper options(tmp);
     _name = options.Get("name");
     tmp = options.Get("protocol");
     if (tmp != NULL) {
        if (strcmp(tmp, avahi_proto_to_string(AVAHI_PROTO_INET)) == 0)
           _protocol = AVAHI_PROTO_INET;
        else if (strcmp(tmp, avahi_proto_to_string(AVAHI_PROTO_INET6)) == 0)
           _protocol = AVAHI_PROTO_INET6;
        }
     _type = options.Get("type");
     tmp = options.Get("port");
     if ((tmp != NULL) && isnumber(tmp))
        _port = atoi(tmp);
     int tmp_nr = 0;
     while (true) {
           tmp = options.Get("subtype", tmp_nr);
           if (tmp == NULL)
              break;
           _subtypes.Append(strdup(tmp));
           tmp_nr++;
           }
     tmp_nr = 0;
     while (true) {
           tmp = options.Get("txt", tmp_nr);
           if (tmp == NULL)
              break;
           _txts.Append(strdup(tmp));
           tmp_nr++;
           }
     _is_valid = true;
     if (*_name == NULL) {
        esyslog("avahi4vdr/services.conf: missing name at line '%s'", s);
        _is_valid = false;
        }
     if (*_type == NULL) {
        esyslog("avahi4vdr/services.conf: missing type at line '%s'", s);
        _is_valid = false;
        }
     if (_port <= 0) {
        esyslog("avahi4vdr/services.conf: missing port at line '%s'", s);
        _is_valid = false;
        }
     }
  return true;
}

bool cAvahiServicesConfig::Save(FILE *f)
{
  return fprintf(f, "%s\n", *_line) > 0;
}
