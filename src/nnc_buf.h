#ifndef __NNC_BUF_H__
#define __NNC_BUF_H__

#include <stddef.h>
#include "nnc_arena.h"

#define NNC_BUF_INICAP 32

typedef struct _nnc_buf {
	nnc_u64 cap;
	nnc_u64 len;
	char buffer[0];
} nnc_buf;

#define nncbuf__hdr(buf) ((nnc_buf*)((char*)(buf) - offsetof(nnc_buf, buffer)))
#define nncbuf__len(buf) nncbuf__hdr(buf)->len
#define nncbuf__cap(buf) nncbuf__hdr(buf)->cap

#define nncbuf__grow(buf) nncbuf_grow((void*)(buf), sizeof(*(buf)), NNC_BUF_INICAP)
#define nncbuf__need(buf) ((buf) ? (nncbuf__cap(buf) <= nncbuf__len(buf)) : 1)
#define nncbuf__fits(buf) ((nncbuf__need(buf)) ? ((buf) = nncbuf__grow(buf)) : 0)


/*
    Macro just to make buf type definition more readable
*/
#define _vec_(valtype)   valtype*
#define vector(valtype)  valtype*
 
#define buf_len(buf) 		((buf) ? nncbuf__len(buf) : 0)
#define buf_cap(buf) 		((buf) ? nncbuf__cap(buf) : 0)
#define buf_add(buf, item) 	(nncbuf__fits(buf), (buf)[nncbuf__len(buf)++] = (item))
#define buf_pop(buf)        ((buf) ? (nncbuf__len(buf)--) : 0)
#define buf_free(buf) 		((buf) ? nnc_dispose(nncbuf__hdr(buf)) : (void)0)
#define buf_last(buf)		((buf)[buf_len(buf)-1])

void* nncbuf_grow(void* buf, nnc_u64 type, nnc_u16 inicap);

#endif