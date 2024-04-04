#ifndef __NNC_x86_64_GEN_H__
#define __NNC_x86_64_GEN_H__

#include "nnc_3a.h"
#include "nnc_misc.h"
#include "nnc_gen_instr_set.h"
#include "nnc_gen_reg_alloc.h"

#define FMEMOPEN_BUF_GROWTH 512

typedef struct _nnc_blob_buf {
    char* blob;
    nnc_u64 len;
    nnc_u64 cap;
} nnc_blob_buf;

typedef struct _nnc_assembly_proc {
    nnc_3a_proc* code;
    nnc_blob_buf impl;
} nnc_assembly_proc;

typedef struct _nnc_assembly_file {
    nnc_blob_buf data_segment_impl;
    vector(nnc_assembly_proc) procs;
    nnc_bool entry_here;
} nnc_assembly_file;

typedef struct _nnc_executable {
    nnc_u32 linker_opts;
    vector(nnc_assembly_file) files;
} nnc_executable;

nnc_assembly_file nnc_gen(
    vector(nnc_3a_proc) procs
);

nnc_blob_buf nnc_build(
    nnc_assembly_file file
);

#endif