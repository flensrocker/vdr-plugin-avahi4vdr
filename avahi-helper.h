#ifndef __AVAHI_HELPER_H
#define __AVAHI_HELPER_H

#include <avahi-common/strlst.h>


class cAvahiHelper
{
private:
  AvahiStringList  *_list;

public:
  cAvahiHelper(const char *Options)
  {
    _list = NULL;
    if (Options != NULL) {
       char *s = strdup(Options);
       size_t len = strlen(Options);
       size_t pos1 = 0;
       dsyslog("avahi4vdr-helper: options = '%s'", s);
       // skip initial commas
       while ((pos1 < len) && (s[pos1] == ','))
             pos1++;
       size_t pos2 = pos1;
       while (pos2 < len) {
             // de-escape backslashed characters
             if ((s[pos2] == '\\') && (pos2 < (len - 1))) {
                memmove(s + pos2, s + pos2 + 1, len - pos2 - 1);
                len--;
                s[len] = 0;
                pos2++;
                continue;
                }
             if ((s[pos2] == ',') && (s[pos2 - 1] != '\\')) {
                s[pos2] = 0;
                dsyslog("avahi4vdr-helper: add '%s'", s + pos1);
                if (_list == NULL)
                   _list = avahi_string_list_new(s + pos1, NULL);
                else
                   _list = avahi_string_list_add(_list, s + pos1);
                pos1 = pos2 + 1;
                }
             pos2++;
             }
       if (pos2 > pos1) {
          dsyslog("avahi4vdr-helper: add '%s'", s + pos1);
          if (_list == NULL)
             _list = avahi_string_list_new(s + pos1, NULL);
          else
             _list = avahi_string_list_add(_list, s + pos1);
          }
       free(s);
       // with avahi_string_list_add new entries are added to the beginning
       // so reverse order to get the parameters in the right order
       if (_list != NULL)
          _list = avahi_string_list_reverse(_list);
       }
  };

  ~cAvahiHelper(void)
  {
    if (_list != NULL) {
       avahi_string_list_free(_list);
       _list = NULL;
       }
  };

  const char *Get(const char *Name, int Number = 0)
  {
    if (_list == NULL)
       return NULL;
    if (Name == NULL)
       return NULL;
    size_t name_len = strlen(Name);
    if (name_len == 0)
       return NULL;

    AvahiStringList *l = _list;
    while (l != NULL) {
          size_t len = avahi_string_list_get_size(l);
          if (len > name_len) {
             const char *text = (const char*)avahi_string_list_get_text(l);
             if ((strncmp(text, Name, name_len) == 0) && (text[name_len] == '=')) {
                Number--;
                if (Number < 0) {
                   dsyslog("avahi4vdr-helper: found parameter '%s'", text);
                   return text + name_len + 1;
                   }
                }
             }
          l = avahi_string_list_get_next(l);
          }
    return NULL;
  };

  int Count(const char *Name)
  {
    int count = 0;
    if (_list == NULL)
       return count;
    if (Name == NULL)
       return count;
    size_t name_len = strlen(Name);
    if (name_len == 0)
       return count;

    AvahiStringList *l = _list;
    while (l != NULL) {
          size_t len = avahi_string_list_get_size(l);
          if (len > name_len) {
             const char *text = (const char*)avahi_string_list_get_text(l);
             if ((strncmp(text, Name, name_len) == 0) && (text[name_len] == '='))
                count++;
             }
          l = avahi_string_list_get_next(l);
          }
    return count;
  };
};

#endif

