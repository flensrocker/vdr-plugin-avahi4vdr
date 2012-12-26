/*
 * avahi4vdr.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "avahi-browser.h"
#include "avahi-client.h"
#include "avahi-service.h"

#include <vdr/plugin.h>

static const char *VERSION        = "1";
static const char *DESCRIPTION    = trNOOP("publish and browse for network services");
static const char *MAINMENUENTRY  = NULL;

class cPluginAvahi4vdr : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cAvahiClient  *_avahi_client;

  cAvahiClient  *AvahiClient()
  {
    if (_avahi_client == NULL)
       _avahi_client = new cAvahiClient();
    return _avahi_client;
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
  return true;
}

bool cPluginAvahi4vdr::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginAvahi4vdr::Stop(void)
{
  // Stop any background activities the plugin is performing.
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
  return false;
}

const char **cPluginAvahi4vdr::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginAvahi4vdr::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  if ((Command == NULL) || (Option == NULL))
     return NULL;

  cString option = Option;

  if (strcmp(Command, "CreateService") == 0) {
     const char *caller = NULL;
     const char *name = NULL;
     AvahiProtocol protocol = AVAHI_PROTO_UNSPEC;
     const char *type = NULL;
     int port = -1;
     int subtypes_len = 0;
     const char **subtypes = NULL;
     int txts_len = 0;
     const char **txts = NULL;

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

     ReplyCode = 900;
     cString id = AvahiClient()->CreateService(caller, name, protocol, type, port, subtypes_len, subtypes, txts_len, txts);
     return cString::sprintf("id=%s", *id);
     }
  else if (strcmp(Command, "DeleteService") == 0) {
     const char *id = NULL;

     if (id == NULL) {
        ReplyCode = 501;
        return "error=missing service id";
        }

     ReplyCode = 900;
     AvahiClient()->DeleteService(id);
     return cString::sprintf("deleted service with id %s", id);
     }
  else if (strcmp(Command, "CreateBrowser") == 0) {
     const char *caller = NULL;
     AvahiProtocol protocol = AVAHI_PROTO_UNSPEC;
     const char *type = NULL;
     bool ignore_local = false;

     if (caller == NULL) {
        ReplyCode = 501;
        return "error=missing caller";
        }
     if (type == NULL) {
        ReplyCode = 501;
        return "error=missing type";
        }

     ReplyCode = 900;
     cString id = AvahiClient()->CreateBrowser(caller, protocol, type, ignore_local);
     return cString::sprintf("id=%s", *id);
     }
  else if (strcmp(Command, "DeleteBrowser") == 0) {
     const char *id = NULL;

     if (id == NULL) {
        ReplyCode = 501;
        return "error=missing browser id";
        }

     ReplyCode = 900;
     AvahiClient()->DeleteBrowser(id);
     return cString::sprintf("deleted browser with id %s", id);
     }

  ReplyCode = 500;
  return NULL;
}

VDRPLUGINCREATOR(cPluginAvahi4vdr); // Don't touch this!
