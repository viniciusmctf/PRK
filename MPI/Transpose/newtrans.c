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
    int requested = MPI_THREAD_FUNNELED;
#else
    int requested = MPI_THREAD_SINGLE;
#endif
    int provided;
    MPI_Init_thread(&argc, &argv, requested, &provided);

    if (provided<requested) {
        printf("thread support insufficient\n");
        MPI_Abort(MPI_COMM_WORLD, requested-provided);
    }

    int wrank, wsize;
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    if (wsize != (int)pow(floor(sqrt(wsize)),2)) {
        printf("nproc must be a square for now\n");
        MPI_Abort(MPI_COMM_WORLD, wsize);
    }

    /* cartesian communicator setup */

    MPI_Comm comm2d;
    {
        int dims[2]    = {0,0};
        int periods[2] = {0,0};
        int reorder    = 0;
        MPI_Dims_create(wsize, 2, dims);
        MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &comm2d );
    }

    int crank, csize;
    int csizex, csizey;
    int crankx, cranky;
    MPI_Comm_size(comm2d, &csize);
    MPI_Comm_rank(comm2d, &crank);
    {
        int dims[2]    = {0,0};
        int periods[2] = {0,0};
        int coords[2]  = {0,0};
        MPI_Cart_get(comm2d, 2, dims, periods, coords);
        csizex = dims[0];
        csizey = dims[1];
        crankx = coords[0];
        cranky = coords[1];
    }

    /* input parsing */

    int matdim = (argc>1) ? atoi(argv[1]) : 2520; /* 9x8x7x5 is divisible by lots of things */
    int tilex  = matdim/csizex;
    int tiley  = matdim/csizey;
    matdim = tilex * csizex; /* round down to something evenly divisible */

    if (wrank==0) {
        printf("matrix tiles of %d by %d on a process grid of %d by %d \n",
                tilex, tiley, csizex, csizey);
    }

    int      tilecount = tilex * tiley;
    MPI_Aint tilebytes = tilecount * sizeof(double);

    /* allocation */

    MPI_Win matwin1;
    MPI_Win matwin2;
    double * matptr1 = NULL;
    double * matptr2 = NULL;
    {
        MPI_Info info = MPI_INFO_NULL; /* FIXME */
        MPI_Win_allocate(tilebytes, sizeof(double), info, comm2d, &matptr1, &matwin1);
        MPI_Win_allocate(tilebytes, sizeof(double), info, comm2d, &matptr2, &matwin2);
        MPI_Win_lock_all(0,matwin1);
        MPI_Win_lock_all(0,matwin2);
    }

    /* initialization */

    for (int iy=0; iy<tiley; iy++) {
        for (int ix=0; ix<tilex; ix++) {
            int tx = crankx * tilex + ix;
            int ty = cranky * tiley + iy;
            int t2 = ty * matdim + tx;
            matptr1[iy*tiley+ix] = (double)t2;
            if (matdim<100) {
                printf("%d: cry=%d crx=%d iy=%d ix=%d ty=%d tx=%d t2=%d mat=%lf\n",
                        crank, cranky, crankx, iy, ix, ty, tx, t2, matptr1[iy*tiley+ix]);
            }
        }
    }
    MPI_Win_sync(matwin1); /* ensure RMA visibility of local writes above (this is a memory barrier) */
    memset(matptr2, 255, tilex * tiley * sizeof(double));
    MPI_Win_sync(matwin2);
    MPI_Barrier(comm2d);

    if (matdim<100) {
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

    /* transpose */

    /* local transpose */
    double * temp = NULL;
    MPI_Alloc_mem(tilebytes, MPI_INFO_NULL, &temp);
    for (int ix=0; ix<tilex; ix++) {
        for (int iy=0; iy<tiley; iy++) {
            temp[ix*tilex+iy] = matptr1[iy*tiley+ix];
        }
    }
    /* network transpose */
    int transrank = csizey * cranky + crankx;
    printf("crank = %d transrank = %d\n", crank, transrank);
    MPI_Put(temp, tilecount, MPI_DOUBLE, transrank, 0 /* disp */, tilecount, MPI_DOUBLE, matwin2);
    MPI_Win_flush(transrank, matwin2);
    MPI_Barrier(comm2d);
    MPI_Free_mem(temp);

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
