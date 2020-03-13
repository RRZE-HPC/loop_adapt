#ifndef LOOP_ADAPT_ERROR_H
#define LOOP_ADAPT_ERROR_H

#include <string.h>
#include <errno.h>

#include <loop_adapt.h>

#define error_h_str(x) #x
#define error_h_tostring(x) error_h_str(x)
#define error_h_min(x,y) ((x) < (y) ? (x) : (y))

typedef enum {
    LOOP_ADAPT_DEBUGLEVEL_ONLYERROR = 0,
    LOOP_ADAPT_DEBUGLEVEL_INFO,
    LOOP_ADAPT_DEBUGLEVEL_DEBUG
} LoopAdaptDebugLevel;
#define LOOP_ADAPT_DEBUGLEVEL_MAX LOOP_ADAPT_DEBUGLEVEL_DEBUG
#define LOOP_ADAPT_DEBUGLEVEL_MIN LOOP_ADAPT_DEBUGLEVEL_ONLYERROR

extern LoopAdaptDebugLevel loop_adapt_verbosity;

#define ERROR_PRINT(fmt, ...) \
   fprintf(stderr, "ERROR - [%s:%s:%d] %s.\n" error_h_str(fmt) "\n", __FILE__, __func__, __LINE__, strerror(errno), ##__VA_ARGS__); \
   fflush(stdout);

#define WARN_PRINT(fmt, ...)  \
   fprintf(stdout, "WARN - " error_h_str(fmt) "\n", ##__VA_ARGS__); \
   fflush(stdout);

#define DEBUG_PRINT(lev, fmt, ...) \
    if ((lev >= LOOP_ADAPT_DEBUGLEVEL_MIN) && (lev <= error_h_min(loop_adapt_verbosity, LOOP_ADAPT_DEBUGLEVEL_MAX))) { \
        fprintf(stdout, "DEBUG - [%s:%d] " error_h_str(fmt) "\n", __func__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
    }

#define TODO_PRINT(fmt, ...)  \
    fprintf(stdout, "TODO - " error_h_str(fmt) "\n", ##__VA_ARGS__);

#endif /* LOOP_ADAPT_ERROR_H */
