#ifndef __AVAHI4VDR_CLIENT_H
#define __AVAHI4VDR_CLIENT_H

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>
#include <avahi-common/strlst.h>
#include <avahi-glib/glib-watch.h>

#include <vdr/thread.h>
#include <vdr/tools.h>

class cAvahiBrowser;
class cAvahiService;


class cAvahiClient : public cThread
{
friend class cAvahiBrowser;
friend class cAvahiService;

private:
  bool             _run_loop;
  GMainLoop       *_loop;
  AvahiClient     *_client;
  cList<cAvahiBrowser> _browsers;
  cList<cAvahiService> _services;
  bool _started;
  cMutex   _loop_mutex;
  cCondVar _loop_cond;
  bool     _loop_quit;

  cAvahiBrowser *GetBrowser(const char *id) const;
  cAvahiService *GetService(const char *id) const;

  void  BrowserError(cAvahiBrowser *browser);
  void  ServiceError(cAvahiService *service);

  static void ClientCallback(AvahiClient *client, AvahiClientState state, void *userdata);
  void ClientCallback(AvahiClient *client, AvahiClientState state);

  void  NotifyCaller(const char *caller, const char *event, const char *id, const char *data) const;

protected:
  virtual void Action(void);

public:
  cAvahiClient(bool run_loop);
  virtual ~cAvahiClient(void);

  bool  ServerIsRunning(void);

  cString CreateBrowser(const char *caller, AvahiProtocol protocol, const char *type, bool ignore_local);
  void    DeleteBrowser(const char *id);

  cString CreateService(const char *caller, const char *name, AvahiProtocol protocol, const char *type, int port, const cStringList& subtypes, const cStringList& txts);
  void    DeleteService(const char *id);

  void Stop(void);
};

#endif
