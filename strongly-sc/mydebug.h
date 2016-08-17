#ifndef _MYDEBUG_H
#define _MYDEBUG_H

#define STRONGSC_DEBUG
//#define STRONGSC_VERBOSE_DEBUG

#ifdef STRONGSC_DEBUG
#define DB(x) x
#define DPRINT(fmt, ...) do { model_print(fmt, ##__VA_ARGS__); } while (0)
#else
#define DB(x) 
#define DPRINT(fmt, ...)
#endif

#ifdef STRONGSC_VERBOSE_DEBUG
#define VDB(x) x
#define VDPRINT(fmt, ...) do { model_print(fmt, ##__VA_ARGS__); } while (0)
#else
#define VDB(x) 
#define VDPRINT(fmt, ...)
#endif

#endif
