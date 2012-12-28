#include "config.h"

#include "avahi-client.h"
#include "avahi-helper.h"


cConfig<cAvahiServicesConfig> cAvahiServicesConfig::Services;
cString cAvahiServicesConfig::_config_file;

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
     else {
        if (strstr(*_name, "%h") != NULL) {
           char hostname[HOST_NAME_MAX];
           if (gethostname(hostname, HOST_NAME_MAX) == 0) {
              char *n = strreplace(strdup(*_name), "%h", hostname);
              _name = n;
              free(n);
              }
           }
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

void cAvahiServicesConfig::StartServices(cAvahiClient *client)
{
  Services.Load(*_config_file, true, false);
  if (client == NULL)
     return;
  for (cAvahiServicesConfig *service = cAvahiServicesConfig::Services.First(); service; service = cAvahiServicesConfig::Services.Next(service)) {
      if (service->_is_valid) {
         cAvahiHelper id(*client->CreateService(NULL, service->_name, service->_protocol, service->_type, service->_port, service->_subtypes, service->_txts));
         service->_id = id.Get("id");
         }
      }
}

void cAvahiServicesConfig::StopServices(cAvahiClient *client)
{
  if (client == NULL)
     return;
  for (cAvahiServicesConfig *service = cAvahiServicesConfig::Services.First(); service; service = cAvahiServicesConfig::Services.Next(service)) {
      if (service->_is_valid && (*service->_id != NULL))
         client->DeleteService(*service->_id);
      }
}
