/*
Copyright (c) 2013, Intel Corporation

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions 
are met:

* Redistributions of source code must retain the above copyright 
      notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above 
      copyright notice, this list of conditions and the following 
      disclaimer in the documentation and/or other materials provided 
      with the distribution.
      * Neither the name of Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products 
      derived from this software without specific prior written 
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/

#include <limits.h>
#include <mpi.h>

/* This code appears in MADNESS, which is GPL, but it was
 * written by Jeff Hammond and contributed to multiple projects
 * using an implicit public domain license. */
#define PRK_MPI_THREAD_STRING(level)  \
        ( level==MPI_THREAD_SERIALIZED ? "THREAD_SERIALIZED" : \
            ( level==MPI_THREAD_MULTIPLE ? "THREAD_MULTIPLE" : \
                ( level==MPI_THREAD_FUNNELED ? "THREAD_FUNNELED" : \
                    ( level==MPI_THREAD_SINGLE ? "THREAD_SINGLE" : "THREAD_UNKNOWN" ) ) ) )

static void print_pretty_mpi_error(int rc, char * funcname)
{
    char string[MPI_MAX_ERROR_STRING] = {0};
    int len; /* unused */
    MPI_Error_string(rc, string, &len);
    printf("%s returned %d (%s)\n", funcname, rc, string);
}

#if !(defined(MPI_VERSION) && (MPI_VERSION>=3))
#error You need MPI-3.
#endif

void * prk_rma_malloc(size_t bytes, size_t type_size, MPI_Comm comm, MPI_Win * win)
{
    if (type_size>(size_t)INT_MAX) {
        printf("type_size %zu is too large!\n", type_size);
        MPI_Abort(MPI_COMM_WORLD,1);
        return NULL;
    }
    int disp_unit = (int)type_size;
    MPI_Info info = MPI_INFO_NULL;
    void* baseptr;
    int rc = MPI_Win_allocate((MPI_Aint)bytes, disp_unit, info, comm, &baseptr, win);
    if (rc!=MPI_SUCCESS) {
        print_pretty_mpi_error(rc,"MPI_Win_allocate");
        MPI_Abort(MPI_COMM_WORLD,1);
        return NULL;
    }
    return baseptr;
}

void prk_rma_free(MPI_Win * win)
{
    int rc = MPI_Win_free(win);
    if (rc!=MPI_SUCCESS) {
        print_pretty_mpi_error(rc,"MPI_Win_free");
        MPI_Abort(MPI_COMM_WORLD,1);
    }
}
