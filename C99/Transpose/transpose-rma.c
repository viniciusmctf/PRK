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

/*******************************************************************

NAME:    transpose

PURPOSE: This program measures the time for the transpose of a
         column-major stored matrix into a row-major stored matrix.

USAGE:   Program input is the matrix order and the number of times to
         repeat the operation:

         transpose <matrix_size> <# iterations> [tile size]

         An optional parameter specifies the tile size used to divide the
         individual matrix blocks for improved cache and TLB performance.

         The output consists of diagnostics to make sure the
         transpose worked and timing statistics.


FUNCTIONS CALLED:

         Other than standard C functions, the following
         functions are used in this program:

         wtime()          portable wall-timer interface.

HISTORY: Written by  Rob Van der Wijngaart, February 2009.
         Modernized by Jeff Hammond, February 2016.
*******************************************************************/

#include <prk_util.h>
#include <prk_openmp.h>
#include <prk_mpi_util.h>

#include <math.h>

int main(int argc, char * argv[])
{
    int requested=MPI_THREAD_FUNNELED, provided;
    MPI_Init_thread(&argc, &argv, requested, &provided);

    int me, npes;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);

    if (provided<requested) {
        //printf("MPI_Init_thread: requested=%s provided=%s\n"
        //        PRK_MPI_THREAD_STRING(requested)
        //        PRK_MPI_THREAD_STRING(provided));
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    /*********************************************************************
    ** read and test input parameters
    *********************************************************************/

    if (me==0) {
        printf("Parallel Research Kernels version %s\n", PRKVERSION);
#ifdef _OPENMP
        printf("MPI+OpenMP Matrix transpose: B = A^T\n");
#else
        printf("MPI Matrix transpose: B = A^T\n");
#endif
    }

    if (argc != 4 && argc != 3) {
      printf("Usage: %s <# iterations> <matrix order> [tile size]\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    int iterations, order, tile_size = 32;
    if (me==0) {
        iterations  = atoi(argv[1]); /* number of times to do the transpose */
        if (iterations < 1) {
            printf("ERROR: iterations must be >= 1 : %d \n", iterations);
            MPI_Abort(MPI_COMM_WORLD,1);
        }
        order = atoi(argv[2]); /* order of a the matrix */
        if (order <= 0) {
            printf("ERROR: Matrix Order must be greater than 0 : %d \n", order);
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        tile_size = 32; /* default tile size for tiling of local transpose */
        if (argc == 4) {
            tile_size = atoi(argv[3]);
        }
        /* a non-positive tile size means no tiling of the local transpose */
        if (tile_size <=0) {
            tile_size = order;
        }

#ifdef _OPENMP
        printf("Number of threads     = %d\n", omp_get_max_threads());
#endif
        printf("Matrix order          = %d\n", order);
        if (tile_size < order) {
            printf("tile size             = %d\n", tile_size);
        } else {
            printf("Untiled\n");
        }
        printf("Number of iterations  = %d\n", iterations);
    }
    {
        int temp[3] = { iterations, order, tile_size };
        if (MPI_Bcast(temp, 3, MPI_INT, 0, MPI_COMM_WORLD)!=MPI_SUCCESS) {
            MPI_Abort(MPI_COMM_WORLD,1);
        }
        iterations = temp[0];
        order      = temp[1];
        tile_size  = temp[2];
    }

    if (order%npes != 0) {
        printf("ERROR: matrix order %d should be divisible by # images %d\n", order, npes);
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    int row_per_pe = order/npes;

    /*********************************************************************
    ** Allocate space for the input and transpose matrix
    *********************************************************************/

    size_t bytes = (size_t)row_per_pe * (size_t)order * sizeof(double);
    MPI_Win A_win, B_win;
    /* A[row_per_pe][order] */
    double (* const restrict A)[order] = (double (*)[order]) prk_rma_malloc(bytes, sizeof(double), MPI_COMM_WORLD, &A_win);
    /* B[row_per_pe][order] */
    double (* const restrict B)[order] = (double (*)[order]) prk_rma_malloc(bytes, sizeof(double), MPI_COMM_WORLD, &B_win);
    /* T[row_per_pe][row_per_pe] */
    double (* const restrict T)[row_per_pe] = (double (*)[row_per_pe]) prk_malloc(row_per_pe*row_per_pe*sizeof(double));

    MPI_Win_lock_all(MPI_MODE_NOCHECK,A_win);
    MPI_Win_lock_all(MPI_MODE_NOCHECK,B_win);

    double trans_time = 0.0;

    OMP_PARALLEL()
    {
        /* initialization */
        /* local row index j corresponds to global row index row_per_pe*me+j */
        OMP_FOR()
        for (int j=0; j<row_per_pe; j++) {
          OMP_SIMD()
          for (int i=0; i<order; i++) {
            const double val = (double) ((size_t)order*(size_t)(row_per_pe*me+j)+(size_t)i);
            A[j][i] = val;
            B[j][i] = 0.0;
          }
        }
        OMP_FOR()
        for (int j=0; j<row_per_pe; j++) {
          OMP_SIMD()
          for (int i=0; i<row_per_pe; i++) {
            T[j][i] = 0.0;
          }
        }

        /* execution of timing */
        for (int iter = 0; iter<=iterations; iter++) {
          /* start timer after a warmup iteration */
          if (iter==1) {
            OMP_BARRIER
            OMP_MASTER
            { trans_time = wtime(); }
          }

          for (int r=0; r<npes; r++) {
            int col_start = me*row_per_pe;
            for (int j=0; j<row_per_pe; j++) {
                MPI_Get(&(T[j][0]),row_per_pe,MPI_DOUBLE,
                        r,col_start,row_per_pe,MPI_DOUBLE,A_win);
            }

            /* transpose the  matrix */
            int row_start = r*row_per_pe;
            if (tile_size < order) {
              OMP_FOR()
              for (int it=0; it<row_per_pe; it+=tile_size) {
                for (int jt=0; jt<row_per_pe; jt+=tile_size) {
                  for (int i=it; i<MIN(row_per_pe,it+tile_size); i++) {
                    OMP_SIMD()
                    for (int j=jt; j<MIN(row_per_pe,jt+tile_size); j++) {
                      B[row_start+i][j] += A[j][i];
                      //A[j][i] += 1.0;
                    }
                  }
                }
              }
            } else {
              OMP_FOR()
              for (int i=0;i<row_per_pe; i++) {
                OMP_SIMD()
                for (int j=0;j<row_per_pe;j++) {
                  B[i][row_start+j] += A[j][i];
                  //A[j][i] += 1.0;
                }
              }
            } /* end transpose */
          } /* end loop over PEs */
        } /* end iterations */
        OMP_BARRIER
        OMP_MASTER
        { trans_time = wtime() - trans_time; }

    } /* end OMP_PARALLEL */

    /*********************************************************************
    ** Analyze and output results.
    *********************************************************************/

    double abserr = 0.0;
    const double addit = ((double)(iterations+1) * (double) (iterations))/2.0;
    OMP_PARALLEL(shared(abserr))
    {
        OMP_FOR(reduction(+:abserr))
        for (int j=0;j<order;j++) {
          for (int i=0;i<order; i++) {
            const size_t offset_ij = (size_t)i*(size_t)order+(size_t)j;
            abserr += fabs(B[j][i] - ((double)offset_ij*(iterations+1.)+addit));
          }
        }
    } /* end OMP_PARALLEL */

    MPI_Win_unlock_all(A_win);
    MPI_Win_unlock_all(B_win);

    prk_free(T);
    prk_rma_free(&B_win);
    prk_rma_free(&A_win);

#ifdef VERBOSE
    printf("Sum of absolute differences: %f\n",abserr);
#endif

    const double epsilon = 1.e-8;
    if (abserr < epsilon) {
      printf("Solution validates\n");
      double avgtime = trans_time/iterations;
      printf("Rate (MB/s): %lf Avg time (s): %lf\n", 1.0E-06 * (2L*bytes)/avgtime, avgtime);
      exit(EXIT_SUCCESS);
    }
    else {
      printf("ERROR: Aggregate squared error %e exceeds threshold %e\n", abserr, epsilon);
      exit(EXIT_FAILURE);
    }

    return 0;
}


