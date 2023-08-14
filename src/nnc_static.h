#ifndef _NNC_STATIC_H
#define _NNC_STATIC_H

#ifdef _NNC_CORE_TESTING_NOW
    // all static functions now accessible
    // for testing and no longer static
    #define nnc_static
#else
    #define nnc_static static
#endif

#endif