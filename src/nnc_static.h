#ifndef __NNC_STATIC_H__
#define __NNC_STATIC_H__

#ifdef _NNC_CORE_TESTING_NOW
    // all static functions now accessible
    // for testing and no longer static
    #define nnc_static
#else
    #define nnc_static static
#endif

#endif