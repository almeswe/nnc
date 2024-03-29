#include "nnc_buf.h"

/**
 * @brief Extends array's max capacity.
 * @param buf Pointer to stretchy buffer. 
 * @param type Size of array's member in bytes.
 * @param inicap Initial capacity of the buffer.
 * @return Pointer to allocated or extended array. 
 */
void* nncbuf_grow(void* buf, nnc_u64 type, nnc_u16 inicap) {
	// set new capacity as 32 by default.
	// in case when buffer is not NULL this
	// capacity will be changed, otherwise it will
	// be initial one for allocation of the array for the first time.
	nnc_u64 new_cap = inicap;
	// variable to store reallocated `nnc_buf` header.
	nnc_buf* new_hdr = NULL;
	nnc_buf* old_hdr = NULL;
	// set new array's size as size of `nnc_buf` header
	// by default, further size will be added after capacity is determined.
	nnc_u64 new_size = offsetof(nnc_buf, buffer);
	// old array size will be used when reallocating.
	nnc_u64 old_size = offsetof(nnc_buf, buffer);
	if (buf != NULL) {
		new_cap = nncbuf__cap(buf) * 2;
	}
	new_size += type * new_cap;
	// if buffer is NULL it means
	// that it must be allocated for the first time.
	if (buf == NULL) {
		new_hdr = (nnc_buf*)nnc_alloc(new_size);
	}
	// otherwise just reallocate the existing one.
	else {
		old_hdr = nncbuf__hdr(buf);
		old_size += old_hdr->cap * type;
		new_hdr = (nnc_buf*)nnc_alloc(new_size);
		memcpy(new_hdr, old_hdr, old_size);
		nnc_dispose(old_hdr);
	}
	new_hdr->cap = new_cap;
	return new_hdr->buffer;
}