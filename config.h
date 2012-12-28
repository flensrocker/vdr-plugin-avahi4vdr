#ifndef __AVAHI4VDR_CONFIG_H

#include <avahi-common/address.h>

#include <vdr/config.h>

class cAvahiClient;


class cAvahiServicesConfig : public cListObject {
private:
  static cConfig<cAvahiServicesConfig> Services;
  
  cString _line;

public:
  cString        _id;
  bool           _is_valid;
  cString        _name;
  AvahiProtocol  _protocol;
  cString        _type;
  int            _port;
  cStringList    _subtypes;
  cStringList    _txts;

  cAvahiServicesConfig(void);
  virtual ~cAvahiServicesConfig(void);

  void Clear(void);
  bool Parse(const char *s);
  bool Save(FILE *f);

  static cString _config_file;

  static void StartServices(cAvahiClient *client);
  static void StopServices(cAvahiClient *client);
};

#endif
