/*
 * avahi4vdr.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "avahi-browser.h"
#include "avahi-client.h"
#include "avahi-helper.h"
#include "avahi-service.h"
#include "config.h"

#include <avahi-glib/glib-malloc.h>

#include <vdr/plugin.h>

static const char *VERSION        = "15";
static const char *DESCRIPTION    = trNOOP("publish and browse for network services");
static const char *MAINMENUENTRY  = NULL;

class cPluginAvahi4vdr : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  bool           _run_loop;
  cAvahiClient  *_avahi_client;

  cAvahiClient  *CreateAvahiClient()
  {
    if (_avahi_client == NULL)
       _avahi_client = new cAvahiClient(_run_loop);
    return _avahi_client;
  };

  void DeleteAvahiClient()
  {
    if (_avahi_client != NULL) {
       delete _avahi_client;
       _avahi_client = NULL;
       }
  };

public:
  cPluginAvahi4vdr(void);
  virtual ~cPluginAvahi4vdr();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginAvahi4vdr::cPluginAvahi4vdr(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  _run_loop = true;
  _avahi_client = NULL;
}

cPluginAvahi4vdr::~cPluginAvahi4vdr()
{
  // Clean up after yourself!
}

const char *cPluginAvahi4vdr::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginAvahi4vdr::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginAvahi4vdr::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  avahi_set_allocator(avahi_glib_allocator());
  cAvahiServicesConfig::_config_file = cString::sprintf("%s/services.conf", cPlugin::ConfigDirectory("avahi4vdr"));
  cPlugin *dbus2vdr = cPluginManager::GetPlugin("dbus2vdr");
  if (dbus2vdr != NULL) {
     int replyCode = 0;
     dbus2vdr->SVDRPCommand("RunsMainLoop", NULL, replyCode);
     if (replyCode == 900)
        _run_loop = false;
     }
  return true;
}

bool cPluginAvahi4vdr::Start(void)
{
  // Start any background activities the plugin shall perform.
  cAvahiServicesConfig::StartServices(CreateAvahiClient());
  return true;
}

void cPluginAvahi4vdr::Stop(void)
{
  // Stop any background activities the plugin is performing.
  cAvahiServicesConfig::StopServices(_avahi_client);
  DeleteAvahiClient();
}

void cPluginAvahi4vdr::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginAvahi4vdr::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginAvahi4vdr::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginAvahi4vdr::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginAvahi4vdr::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginAvahi4vdr::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginAvahi4vdr::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginAvahi4vdr::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if ((Id != NULL) && (strcmp(Id, "dbus2vdr-MainLoopStopped") == 0)) {
     Stop();
     return true;
     }
  return false;
}

const char **cPluginAvahi4vdr::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  static const char *HelpPages[] = {
    "ReloadServices\n"
    "    All static services are stopped, the services.conf is reloaded\n"
    "    and the services are started again.",
    NULL
    };
  return HelpPages;
}

cString cPluginAvahi4vdr::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  if ((Command == NULL) || (Option == NULL))
     return NULL;

  if (_avahi_client == NULL) {
     esyslog("avahi4vdr: no avahi client!");
     ReplyCode = 451;
     return "";
     }

  cAvahiHelper options(Option);

  if (strcmp(Command, "ReloadServices") == 0) {
     cAvahiServicesConfig::StopServices(_avahi_client);
     cAvahiServicesConfig::StartServices(_avahi_client);
     ReplyCode = 900;
     return "services.conf reloaded";
     }
  else if (strcmp(Command, "CreateService") == 0) {
     const char *caller = options.Get("caller");
     const char *name = options.Get("name");
     AvahiProtocol protocol = AVAHI_PROTO_UNSPEC;
     const char *type = options.Get("type");
     int port = -1;
     int subtypes_len = options.Count("subtype");
     cStringList subtypes(subtypes_len);
     int txts_len = options.Count("txt");
     cStringList txts(txts_len);

     const char *tmp = options.Get("protocol");
     if (tmp != NULL) {
        if (strcmp(tmp, avahi_proto_to_string(AVAHI_PROTO_INET)) == 0)
           protocol = AVAHI_PROTO_INET;
        else if (strcmp(tmp, avahi_proto_to_string(AVAHI_PROTO_INET6)) == 0)
           protocol = AVAHI_PROTO_INET6;
        }

     tmp = options.Get("port");
     if ((tmp != NULL) && isnumber(tmp))
        port = atoi(tmp);

     if (name == NULL) {
        ReplyCode = 501;
        return "error=missing service name";
        }
     if (type == NULL) {
        ReplyCode = 501;
        return "error=missing type";
        }
     if (port <= 0) {
        ReplyCode = 501;
        return "error=missing port";
        }

     if (subtypes_len > 0) {
        for (int i = 0; i < subtypes_len; i++) {
            subtypes[i] = strdup(options.Get("subtype", i));
            dsyslog("avahi4vdr: found subtype %s", subtypes[i]);
            }
        }

     if (txts_len > 0) {
        for (int i = 0; i < txts_len; i++) {
            txts[i] = strdup(options.Get("txt", i));
            dsyslog("avahi4vdr: found txt %s", txts[i]);
            }
        }

     ReplyCode = 900;
     cString id = _avahi_client->CreateService(caller, name, protocol, type, port, subtypes, txts);
     return cString::sprintf("id=%s", *id);
     }
  else if (strcmp(Command, "DeleteService") == 0) {
     const char *id = options.Get("id");

     if (id == NULL) {
        ReplyCode = 501;
        return "error=missing service id";
        }

     ReplyCode = 900;
     _avahi_client->DeleteService(id);
     return cString::sprintf("message=deleted service with id %s", id);
     }
  else if (strcmp(Command, "CreateBrowser") == 0) {
     const char *caller = options.Get("caller");
     AvahiProtocol protocol = AVAHI_PROTO_UNSPEC;
     const char *type = options.Get("type");
     bool ignore_local = false;

     const char *tmp = options.Get("protocol");
     if (tmp != NULL) {
        if (strcmp(tmp, avahi_proto_to_string(AVAHI_PROTO_INET)) == 0)
           protocol = AVAHI_PROTO_INET;
        else if (strcmp(tmp, avahi_proto_to_string(AVAHI_PROTO_INET6)) == 0)
           protocol = AVAHI_PROTO_INET6;
        }

     tmp = options.Get("ignore_local");
     if ((tmp != NULL) && (strcmp(tmp, "true") == 0))
        ignore_local = true;

     if (caller == NULL) {
        ReplyCode = 501;
        return "error=missing caller";
        }
     if (type == NULL) {
        ReplyCode = 501;
        return "error=missing type";
        }

     ReplyCode = 900;
     cString id = _avahi_client->CreateBrowser(caller, protocol, type, ignore_local);
     return cString::sprintf("id=%s", *id);
     }
  else if (strcmp(Command, "DeleteBrowser") == 0) {
     const char *id = options.Get("id");

     if (id == NULL) {
        ReplyCode = 501;
        return "error=missing browser id";
        }

     ReplyCode = 900;
     _avahi_client->DeleteBrowser(id);
     return cString::sprintf("message=deleted browser with id %s", id);
     }
  else if (strcmp(Command, "Shutdown") == 0) {
     Stop();
     ReplyCode = 900;
     return "avahi4vdr has been stopped";
     }

  ReplyCode = 500;
  return NULL;
}

VDRPLUGINCREATOR(cPluginAvahi4vdr); // Don't touch this!
