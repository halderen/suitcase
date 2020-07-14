#ifndef SETTINGS_H
#define SETTINGS_H

typedef void* settings_handle;

int settings_access(settings_handle*, int basefd, const char* filename);
int settings_getlong(settings_handle, long* resultvalue, const long* defaultvalue, const char* fmt,...);
int settings_getenum(settings_handle, int* resultvalue, const int* defaultvalue, const char** enums, const char* fmt, ...);
int settings_getenum2(settings_handle, int* resultvalue, const int* defaultvalue, const char** enumstrings, const int* enumvalues, const char* fmt, ...);
int settings_getcount(settings_handle, long* resultvalue, const long* defaultvalue, const char* fmt,...);
int settings_getnamed(settings_handle handle, long* resultvalue, const long* defaultvalue, int (*translate)(const char*,long*resultvalue), const char* fmt, ...);
int settings_getstring(settings_handle, char** resultvalue, const char* defaultvalue, const char* fmt, ...);
int settings_getcompound(settings_handle, int* resultvalue, const char* fmt, ...);

int settings_configure(settings_handle* cfghandleptr, char* sysconfdir, char* sysconffile, int cmdline_verbosity);

#endif
