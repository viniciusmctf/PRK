#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <mpi.h>

int main(int argc, char* argv[])
{
#ifdef _OPENMP
    /* FUNNELED  is consistent with OpenMP fork-join usage where
     * all MPI calls are outside OpenMP (or are called inside of
     * an OpenMP "master" region).  It should have no overhead 
     * other than requiring MPI to use thread-safe system libraries
     * (e.g. malloc should be thread-safe), although some MPI 
     * implementations fall short in this respect and instead
     * introduce substantial runtime overhead for any thread
     * level above SINGLE (cough OpenMPI cough). */
    int requested = MPI_THREAD_FUNNELED;
#else
    int requested = MPI_THREAD_SINGLE;
#endif
    int provided;
    MPI_Init_thread(&argc, &argv, requested, &provided);

    if (provided<requested) {
        printf("Thread support insufficient.  Get a new MPI!\n");
        MPI_Abort(MPI_COMM_WORLD, requested-provided);
    }

    /* In case it is not obvious, w is for world (i.e. MPI_COMM_WORLD) here. */
    int wrank, wsize;
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* This check is here because Jeff failed at bookkeeping somewhere. */
    if (wsize != (int)pow(floor(sqrt(wsize)),2)) {
        printf("nproc must be a square for now\n");
        MPI_Abort(MPI_COMM_WORLD, wsize);
    }

    /* Cartesian communicator setup:
     * While not necessary, it is possible that the MPI library will
     * do a better-than-naive job of mapping the output communicator
     * ranks to the network/node topology for a 2D grid. */

    MPI_Comm comm2d;
    { /* scoped so I can reuse symbols with impunity */
        int dims[2]    = {0,0};
        int periods[2] = {0,0};
        int reorder    = 0;
        MPI_Dims_create(wsize, 2, dims);
        MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &comm2d );
    }

    /* In case it is not obvious, c is for cartesian (i.e. 2D grid comm) here. */
    int crank, csize;
    int csizex, csizey;
    int crankx, cranky;
    MPI_Comm_size(comm2d, &csize);
    MPI_Comm_rank(comm2d, &crank);
    { /* scoped so I can reuse symbols with impunity */
        int dims[2]    = {0,0};
        int periods[2] = {0,0};
        int coords[2]  = {0,0};
        MPI_Cart_get(comm2d, 2, dims, periods, coords);
        csizex = dims[0];
        csizey = dims[1];
        crankx = coords[0];
        cranky = coords[1];
    }

    /* Input parsing:
     * We round down to avoid having to manage edge-case bookkeeping. */

    int matdim = (argc>1) ? atoi(argv[1]) : 2520; /* 9x8x7x5 is divisible by lots of things */
    int tilex  = matdim/csizex;
    int tiley  = matdim/csizey;
    matdim = tilex * csizex; /* round down to something evenly divisible */

    if (wrank==0) {
        printf("matrix tiles of %d by %d on a process grid of %d by %d \n",
                tilex, tiley, csizex, csizey);
    }

    /* Caching these to avoid computing integer multiplies on the stack is debatable. */
    int      tilecount = tilex * tiley;
    MPI_Aint tilebytes = tilecount * sizeof(double);

    /* Window allocation:
     * MPI_WIN_ALLOCATE is the union of MPI_ALLOC_MEM and MPI_WIN_CREATE.
     * It provides the local memory as _output_, rather than taking it as 
     * input.  This allows the implementation to do smart things with
     * symmetric memory allocation, buffer registration, NUNA, etc. */

    MPI_Win matwin1;
    MPI_Win matwin2;
    double * matptr1 = NULL;
    double * matptr2 = NULL;
    {
        MPI_Info info = MPI_INFO_NULL; /* FIXME */
        MPI_Win_allocate(tilebytes, sizeof(double), info, comm2d, &matptr1, &matwin1);
        MPI_Win_allocate(tilebytes, sizeof(double), info, comm2d, &matptr2, &matwin2);
        /* MPI_WIN_LOCK_ALL (WIN_LOCK_SHARED is implied) causes MPI-3 RMA to behave
         * like PGAS models such as ARMCI and OpenSHMEM, i.e. we can do RMA at any time
         * from now on.  There are no useful assertions right now (TODO: Jeff in MPI-4). */
        MPI_Win_lock_all(0,matwin1);
        MPI_Win_lock_all(0,matwin2);

        /* For educational purposes only, this is how one determines what the
         * memory model of RMA is.  All cache-coherent systems show up as
         * MPI_WIN_UNIFIED, even though x86 consistency is stronger than
         * that of PowerPC and ARM in practice. */
        void * pmm = NULL; /* attribute output is pointer to pointer... */
        int flag;
        MPI_Win_get_attr(matwin1, MPI_WIN_MODEL, &pmm, &flag);
        if (flag && wrank==0) {
            int * imm = (int*) pmm; /* because attributes are not typed... */
            fprintf(stderr, "RMA memory model is %s\n",
                    (*imm)==MPI_WIN_UNIFIED ? "UNIFIED" : "SEPARATE" );
            fflush(stderr);
        }
    }

    /* initialization */

    for (int iy=0; iy<tiley; iy++) {
        for (int ix=0; ix<tilex; ix++) {
            int tx = crankx * tilex + ix;
            int ty = cranky * tiley + iy;
            int t2 = ty * matdim + tx;
            matptr1[iy*tiley+ix] = (double)t2;
            if (matdim<100) {
                /* debug printing for tiny tiles... */
                printf("%d: cry=%d crx=%d iy=%d ix=%d ty=%d tx=%d t2=%d mat=%lf\n",
                        crank, cranky, crankx, iy, ix, ty, tx, t2, matptr1[iy*tiley+ix]);
            }
        }
    }
    /* MPI_WIN_SYNC synchronizes the public (RMA-accessible) and private 
     * (load-store-accessible) views of the window.  These are _eventually_ consistent
     * within the UNIFIED model and _inconsistent_ in the SEPARATE model until 
     * this call is made. */
    MPI_Win_sync(matwin1);

    /* This will show as NaN if not overwritten, which helps debugging. */
    memset(matptr2, 255, tilex * tiley * sizeof(double));
    MPI_Win_sync(matwin2);

    /* To ensure all initializations are finished before proceeding. */
    MPI_Barrier(comm2d);

    if (matdim<100) {
        /* Gigantic hammer on stdout to keep debug printing lined up... */
        fflush(stdout);
        MPI_Barrier(comm2d);
        fflush(stdout);
        MPI_Barrier(comm2d);
        if (wrank==0) {
            printf("====================================\n");
            fflush(stdout);
        }
        fflush(stdout);
        MPI_Barrier(comm2d);
        fflush(stdout);
        MPI_Barrier(comm2d);
    }

    /* Perform the transpose:
     *************************************************
     * Method 1 - local transpose followed by RMA put.
     * Method 2 - RMA get followed by local transpose.
     * Method 3 - Send-Recv.
     *************************************************/

    /* M1: Local transpose - should be replaced with Tim's code. */
    double * temp = NULL;
    MPI_Alloc_mem(tilebytes, MPI_INFO_NULL, &temp);
    for (int ix=0; ix<tilex; ix++) {
        for (int iy=0; iy<tiley; iy++) {
            temp[ix*tilex+iy] = matptr1[iy*tiley+ix];
        }
    }

    /* M1: Network transpose */
    int transrank = csizey * cranky + crankx;
    MPI_Put(temp, tilecount, MPI_DOUBLE, transrank, 0 /* disp */, tilecount, MPI_DOUBLE, matwin2);
    MPI_Win_flush(transrank, matwin2); /* ensure remote completion but lacks notification */

    /* heavy hammer for remote notification */
    MPI_Barrier(comm2d);

    MPI_Free_mem(temp);
    /* notify-wait would go here... */
    MPI_Win_sync(matwin2); /* ensure win and local views are in sync */
    for (int iy=0; iy<tiley; iy++) {
        for (int ix=0; ix<tilex; ix++) {
            int tx = crankx * tilex + ix;
            int ty = cranky * tiley + iy;
            int t2 = ty * matdim + tx;
            if (matdim<100) {
                printf("%d: cry=%d crx=%d iy=%d ix=%d ty=%d tx=%d t2=%d mat=%lf\n",
                        crank, cranky, crankx, iy, ix, ty, tx, t2, matptr2[iy*tiley+ix]);
            }
        }
    }

    /* deallocation */

    MPI_Win_unlock_all(matwin2);
    MPI_Win_unlock_all(matwin1);
    MPI_Win_free(&matwin2);
    MPI_Win_free(&matwin1);

    MPI_Comm_free(&comm2d);

    MPI_Finalize();
    return 0;
}
