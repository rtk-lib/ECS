#pragma once

#ifdef NDEBUG
    #define RTK_ASSERT(condition, message) ((void)0)
#else
    #include <cassert>
    #define RTK_ASSERT(condition, message) assert((condition) && (message))
#endif