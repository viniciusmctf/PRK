#include <stdio.h>
#include <stdlib.h>
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

    {
        if (wsize != (int)pow(floor(sqrt(wsize)),2)) {
            printf("nproc must be a square for now\n");
            MPI_Abort(MPI_COMM_WORLD, wsize);
        }
    }

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

    int matdim = (argc>1) ? atoi(argv[1]) : 2520; /* 9x8x7x5 is divisible by lots of things */
    int tilex  = matdim/csizex;
    int tiley  = matdim/csizey;
    matdim = tilex * csizex; /* round down to something evenly divisible */

    if (wrank==0) {
        printf("matrix tiles of %d by %d on a process grid of %d by %d \n",
                tilex, tiley, csizex, csizey);
    }

    MPI_Win matwin1;
    MPI_Win matwin2;
    double * matptr1 = NULL;
    double * matptr2 = NULL;
    {
        MPI_Aint bytes = tilex * tiley * sizeof(double);
        MPI_Info info = MPI_INFO_NULL; /* FIXME */
        MPI_Win_allocate(bytes, sizeof(double), info, comm2d, &matptr1, &matwin1);
        MPI_Win_allocate(bytes, sizeof(double), info, comm2d, &matptr2, &matwin2);
        MPI_Win_lock_all(0,matwin1);
        MPI_Win_lock_all(0,matwin2);
    }
    MPI_Barrier(comm2d); /* this barrier is necessary */

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
    MPI_Win_sync(matwin1);
    MPI_Barrier(comm2d);

    MPI_Win_unlock_all(matwin2);
    MPI_Win_unlock_all(matwin1);
    MPI_Win_free(&matwin2);
    MPI_Win_free(&matwin1);

    MPI_Comm_free(&comm2d);

    MPI_Finalize();
    return 0;
}
