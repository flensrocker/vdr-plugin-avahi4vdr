#ifndef __AVAHI4VDR_CONFIG_H

#include <vdr/config.h>


class cAvahiServicesConfig : public cListObject {
private:
  cString _line;

public:
  bool        _is_valid;
  cString     _name;
  cString     _type;
  int         _port;
  cStringList _subtypes;
  cStringList _txts;

  cAvahiServicesConfig(void);
  virtual ~cAvahiServicesConfig(void);

  void Clear(void);
  bool Parse(const char *s);
  bool Save(FILE *f);

  static cConfig<cAvahiServicesConfig> Services;
};

#endif
