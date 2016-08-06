#ifndef _MYDEBUG_H
#define _MYDEBUG_H

#define STRONGSC_DEBUG

#ifdef STRONGSC_DEBUG
#define DB(x) x
#define DPRINT(fmt, ...) do { model_print(fmt, ##__VA_ARGS__); } while (0)
#else
#define DB(x) 
#define DPRINT(fmt, ...)
#endif

#endif
