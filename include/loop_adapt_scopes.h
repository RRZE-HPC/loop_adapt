#ifndef LOOP_ADAPT_SCOPES_H
#define LOOP_ADAPT_SCOPES_H

#include <hwloc.h>

#define LOOP_ADAPT_SCOPE_THREAD_OFFSET 0
#define LOOP_ADAPT_SCOPE_CORE_OFFSET 1
#define LOOP_ADAPT_SCOPE_LLCACHE_OFFSET 2
#define LOOP_ADAPT_SCOPE_NUMANODE_OFFSET 3
#define LOOP_ADAPT_SCOPE_SOCKET_OFFSET 4
#define LOOP_ADAPT_SCOPE_SYSTEM_OFFSET 5

#define LOOP_ADAPT_NUM_SCOPES 6


#define LOOP_ADAPT_SCOPE_MIN HWLOC_OBJ_TYPE_MIN
typedef enum {
    LOOP_ADAPT_SCOPE_THREAD = HWLOC_OBJ_PU,
    LOOP_ADAPT_SCOPE_CORE = HWLOC_OBJ_CORE,
    LOOP_ADAPT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE,
    LOOP_ADAPT_SCOPE_SYSTEM = HWLOC_OBJ_MACHINE,
    LOOP_ADAPT_SCOPE_NUMANODE = HWLOC_OBJ_NUMANODE,
    LOOP_ADAPT_SCOPE_LLCACHE = HWLOC_OBJ_L3CACHE,
    LOOP_ADAPT_SCOPE_MAX
} LoopAdaptScope_t;


static const int LoopAdaptScopeList[LOOP_ADAPT_NUM_SCOPES] = {
    [LOOP_ADAPT_SCOPE_THREAD_OFFSET] = LOOP_ADAPT_SCOPE_THREAD,
    [LOOP_ADAPT_SCOPE_CORE_OFFSET] = LOOP_ADAPT_SCOPE_CORE,
    [LOOP_ADAPT_SCOPE_LLCACHE_OFFSET] = LOOP_ADAPT_SCOPE_LLCACHE,
    [LOOP_ADAPT_SCOPE_NUMANODE_OFFSET] = LOOP_ADAPT_SCOPE_NUMANODE,
    [LOOP_ADAPT_SCOPE_SOCKET_OFFSET] = LOOP_ADAPT_SCOPE_SOCKET,
    [LOOP_ADAPT_SCOPE_SYSTEM_OFFSET] = LOOP_ADAPT_SCOPE_SYSTEM,
};


#define LOOP_ADAPT_VALID_SCOPE(scope) \
    ((scope) == LOOP_ADAPT_SCOPE_THREAD || \\
     (scope) == LOOP_ADAPT_SCOPE_CORE || \\
     (scope) == LOOP_ADAPT_SCOPE_SOCKET || \\
     (scope) == LOOP_ADAPT_SCOPE_SYSTEM || \\
     (scope) == LOOP_ADAPT_SCOPE_NUMANODE || \\
     (scope) == LOOP_ADAPT_SCOPE_LLCACHE)


#endif /* LOOP_ADAPT_SCOPES_H */