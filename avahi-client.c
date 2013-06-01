#include "avahi-client.h"
#include "avahi-browser.h"
#include "avahi-service.h"

#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

#include <vdr/plugin.h>


cAvahiClient::cAvahiClient(bool run_loop)
 :_run_loop(run_loop)
 ,_loop(NULL)
 ,_client(NULL)
 ,_started(false)
 ,_loop_quit(false)
{
  Start();
  while (!_started)
        cCondWait::SleepMs(10);
}

cAvahiClient::~cAvahiClient(void)
{
  Stop();
  _browsers.Clear();
  _services.Clear();
}

cAvahiBrowser *cAvahiClient::GetBrowser(const char *id) const
{
  if (id == NULL)
     return NULL;
  cAvahiBrowser *browser = _browsers.First();
  while ((browser != NULL) && (strcmp(*browser->Id(), id) != 0))
        browser = _browsers.Next(browser);
  return browser;
}

cAvahiService *cAvahiClient::GetService(const char *id) const
{
  if (id == NULL)
     return NULL;
  cAvahiService *service = _services.First();
  while ((service != NULL) && (strcmp(*service->Id(), id) != 0))
        service = _services.Next(service);
  return service;
}

void  cAvahiClient::BrowserError(cAvahiBrowser *browser)
{
  esyslog("avahi4vdr-client: browser error");
}

void  cAvahiClient::ServiceError(cAvahiService *service)
{
  esyslog("avahi4vdr-client: service error");
}

bool cAvahiClient::ServerIsRunning(void)
{
  bool runs = false;
  if (_client != NULL)
     runs = (avahi_client_get_state(_client) == AVAHI_CLIENT_S_RUNNING);
  return runs;
}

void cAvahiClient::ClientCallback(AvahiClient *client, AvahiClientState state, void *userdata)
{
  if (userdata == NULL)
     return;
  ((cAvahiClient*)userdata)->ClientCallback(client, state);
}

void cAvahiClient::ClientCallback(AvahiClient *client, AvahiClientState state)
{
  if ((_client != NULL) && (_client != client)) {
     isyslog("avahi4vdr-client: unexpected client callback (%p != %p), state %d", _client, client, state);
     return;
     }

  switch (state) {
    case AVAHI_CLIENT_S_RUNNING:
     {
      Lock();
      for (cAvahiBrowser *browser = _browsers.First(); browser ; browser = _browsers.Next(browser))
          browser->Create(client);
      for (cAvahiService *service = _services.First(); service ; service = _services.Next(service))
          service->Create(client);
      Unlock();
      break;
     }
    case AVAHI_CLIENT_FAILURE:
     {
      esyslog("avahi4vdr-client: client failure: %s", avahi_strerror(avahi_client_errno(client)));
      if (!_run_loop) {
         cMutexLock MutexLock(&_loop_mutex);
         _loop_quit = true;
         _loop_cond.Broadcast();
         }
      else if (_loop != NULL)
         g_main_loop_quit(_loop);
      break;
     }
    case AVAHI_CLIENT_S_COLLISION:
    case AVAHI_CLIENT_S_REGISTERING:
     {
      Lock();
      for (cAvahiService *service = _services.First(); service ; service = _services.Next(service))
          service->Reset();
      Unlock();
      break;
     }
    case AVAHI_CLIENT_CONNECTING:
      break;
    }
}

void  cAvahiClient::NotifyCaller(const char *caller, const char *event, const char *id, const char *data) const
{
  if ((caller == NULL) || (event == NULL) || (id == NULL))
     return;
  cPlugin *plugin = cPluginManager::GetPlugin(caller);
  if (plugin == NULL)
     return;
  cString call = cString::sprintf("event=%s,id=%s%s%s", event, id, (data != NULL ? "," : ""), (data != NULL ? data : ""));
  plugin->Service("avahi4vdr-event", (void*)(*call));
  isyslog("avahi4vdr-client: notify %s on event %s", caller, *call);
}

cString cAvahiClient::CreateBrowser(const char *caller, AvahiProtocol protocol, const char *type, bool ignore_local)
{
  Lock();
  cAvahiBrowser *browser = new cAvahiBrowser(this, caller, protocol, type, ignore_local);
  if (browser == NULL) {
     Unlock();
     return "";
     }
  cString id = browser->Id();
  _browsers.Add(browser);
  if (ServerIsRunning())
     browser->Create(_client);
  Unlock();
  return id;
}

void    cAvahiClient::DeleteBrowser(const char *id)
{
  Lock();
  cAvahiBrowser *browser = GetBrowser(id);
  if (browser != NULL) {
     browser->Delete();
     _browsers.Del(browser);
     }
  Unlock();
}

cString cAvahiClient::CreateService(const char *caller, const char *name, AvahiProtocol protocol, const char *type, int port, const cStringList& subtypes, const cStringList& txts)
{
  Lock();
  cAvahiService *service = new cAvahiService(this, caller, name, protocol, type, port, subtypes, txts);
  if (service == NULL) {
     Unlock();
     return "";
     }
  cString id = service->Id();
  _services.Add(service);
  if (ServerIsRunning())
     service->Create(_client);
  Unlock();
  return id;
}

void cAvahiClient::DeleteService(const char *id)
{
  Lock();
  cAvahiService *service = GetService(id);
  if (service != NULL) {
     service->Delete();
     _services.Del(service);
     }
  Unlock();
}

void cAvahiClient::Stop(void)
{
  Cancel(-1);
  Lock();
  if (!_run_loop) {
     isyslog("avahi4vdr-client: stopping client");
     cMutexLock MutexLock(&_loop_mutex);
     _loop_quit = true;
     _loop_cond.Broadcast();
     }
  else if (_loop != NULL) {
     isyslog("avahi4vdr-client: stopping GMainLoop");
     g_main_loop_quit(_loop);
     }
  Unlock();
  Cancel(5);
}

void cAvahiClient::Action(void)
{
  _started = true;
  isyslog("avahi4vdr-client: started");

  GMainContext *thread_context = NULL;
  if (_run_loop) {
     thread_context = g_main_context_new();
     g_main_context_push_thread_default(thread_context);
     }

  AvahiGLibPoll *glib_poll = NULL;
  int avahiError = 0;
  int reconnectLogCount = 0;
  while (Running()) {
        if (_run_loop && (_loop == NULL)) {
           // don't get too verbose...
           if (reconnectLogCount < 5)
              isyslog("avahi4vdr-client: create GMainLoop");
           else if (reconnectLogCount > 15) // ...and too quiet
              reconnectLogCount = 0;

           Lock();
           _loop = g_main_loop_new(thread_context, false);
           if (_loop == NULL) {
              Unlock();
              esyslog("avahi4vdr-client: error on creating GMainLoop");
              cCondWait::SleepMs(1000);
              if (!Running())
                 break;
              reconnectLogCount++;
              continue;
              }
           Unlock();
           reconnectLogCount = 0;
           }

        if (_client == NULL) {
           // don't get too verbose...
           if (reconnectLogCount < 5)
              isyslog("avahi4vdr-client: create client");
           else if (reconnectLogCount > 15) // ...and too quiet
              reconnectLogCount = 0;

           Lock();
           glib_poll = avahi_glib_poll_new(thread_context, G_PRIORITY_DEFAULT);
           _client = avahi_client_new(avahi_glib_poll_get(glib_poll), (AvahiClientFlags)0, ClientCallback, this, &avahiError);
           if (_client == NULL) {
              avahi_glib_poll_free(glib_poll);
              glib_poll = NULL;
              Unlock();
              esyslog("avahi4vdr-client: error on creating client: %s", avahi_strerror(avahiError));
              cCondWait::SleepMs(1000);
              if (!Running())
                 break;
              reconnectLogCount++;
              continue;
              }
           Unlock();
           reconnectLogCount = 0;
           }

        if (!_run_loop) {
           cMutexLock MutexLock(&_loop_mutex);
           while (Running()) {
                 _loop_cond.TimedWait(_loop_mutex, 1000);
                 if (_loop_quit) {
                    _loop_quit = false;
                    dsyslog("avahi4vdr-client: quit loop");
                    break;
                    }
                 }
           }
        else if (_loop != NULL)
           g_main_loop_run(_loop);

        Lock();
        for (cAvahiBrowser *browser = _browsers.First(); browser ; browser = _browsers.Next(browser))
            browser->Delete();
        for (cAvahiService *service = _services.First(); service ; service = _services.Next(service))
            service->Delete();
        if (_client != NULL) {
           avahi_client_free(_client);
           _client = NULL;
           }
        if (glib_poll != NULL) {
           avahi_glib_poll_free(glib_poll);
           glib_poll = NULL;
           }
        if (_loop != NULL) {
           g_main_loop_unref(_loop);
           _loop = NULL;
           }
        Unlock();
        }
  if (thread_context != NULL) {
     g_main_context_pop_thread_default(thread_context);
     g_main_context_unref(thread_context);
     }

  isyslog("avahi4vdr-client: stopped");
}
