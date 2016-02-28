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
        if (iterations < 0) {
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
    /* A[row_per_pe][order] */
    double (* const restrict A)[order] = (double (*)[order]) prk_malloc(bytes);
    /* B[row_per_pe][order] */
    double (* const restrict B)[order] = (double (*)[order]) prk_malloc(bytes);
#ifdef PRK_TRANSPOSE_A2A_LOW_MEMORY
    /* T[row_per_pe][row_per_pe] */
    double (* const restrict TA)[order] = (double (*)[order]) prk_malloc(row_per_pe*row_per_pe*sizeof(double));
    double (* const restrict TB)[order] = (double (*)[order]) prk_malloc(row_per_pe*row_per_pe*sizeof(double));
#else
    /* T[order][row_per_pe] */
    double (* const restrict TA)[row_per_pe] = (double (*)[row_per_pe]) prk_malloc(bytes);
    double (* const restrict TB)[row_per_pe] = (double (*)[row_per_pe]) prk_malloc(bytes);
#endif

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
#ifdef PRK_TRANSPOSE_A2A_LOW_MEMORY
        for (int j=0; j<row_per_pe; j++) {
#else
        for (int j=0; j<order; j++) {
#endif
          OMP_SIMD()
          for (int i=0; i<row_per_pe; i++) {
            TA[j][i] = -1.0*INT_MAX;
            TB[j][i] = -1.0*INT_MAX;
          }
        }

        /* execution of timing */
        for (int iter = 0; iter<=iterations; iter++) {

          /* start timer after a warmup iteration */
          if (iter<=1) {
            OMP_BARRIER
            OMP_MASTER
            {
              MPI_Barrier(MPI_COMM_WORLD);
              trans_time = wtime();
            }
          }

          /* debug */
          for (int r=0; r<npes; r++) {
            int col_start = me * row_per_pe;
            if (me==r) {
              for (int j=0; j<row_per_pe; j++) {
                for (int i=0; i<order; i++) {
                  printf("%s %d: %d,%d,%lf\n","BEFORE",me,col_start+j+1,i+1,A[j][i]);
                }
              }
              fflush(stdout);
            }
            MPI_Barrier(MPI_COMM_WORLD);
          }

          /* A[row_per_pe][order] */
          /* TA[order][row_per_pe] */
          for (int r=0; r<npes; r++) {
            for (int j=0;j<row_per_pe;j++) {
              memcpy(&(TA[r*j][0]),&(A[j][r]),row_per_pe*sizeof(double));
            }
          }

          MPI_Alltoall(TA, row_per_pe*row_per_pe, MPI_DOUBLE,
                       TB, row_per_pe*row_per_pe, MPI_DOUBLE, MPI_COMM_WORLD);

          /* debug */
          for (int r=0; r<npes; r++) {
            int col_start = me * row_per_pe;
            if (me==r) {
              for (int j=0; j<order; j++) {
                for (int i=0; i<row_per_pe; i++) {
                  printf("%s %d: %d,%d,%lf\n","DURING",me,j+1,col_start+i+1,TB[j][i]);
                }
              }
              fflush(stdout);
            }
            MPI_Barrier(MPI_COMM_WORLD);
          }

          /* transpose the  matrix */
          if (tile_size < order) {
            OMP_FOR()
            for (int it=0; it<row_per_pe; it+=tile_size) {
              for (int jt=0; jt<order; jt+=tile_size) {
                for (int i=it; i<MIN(row_per_pe,it+tile_size); i++) {
                  OMP_SIMD()
                  for (int j=jt; j<MIN(order,jt+tile_size); j++) {
                    B[i][j] += TB[j][i];
                  }
                }
              }
            }
          } else {
            OMP_FOR()
            for (int i=0;i<row_per_pe; i++) {
              OMP_SIMD()
              for (int j=0;j<order;j++) {
                B[i][j] += TB[j][i];
              }
            }
          }

          /* debug */
          for (int r=0; r<npes; r++) {
            int col_start = me * row_per_pe;
            if (me==r) {
              for (int j=0; j<row_per_pe; j++) {
                for (int i=0; i<order; i++) {
                  printf("%s %d: %d,%d,%lf\n","AFTER",me,col_start+j+1,i+1,B[j][i]);
                }
              }
              fflush(stdout);
            }
            MPI_Barrier(MPI_COMM_WORLD);
          }

          MPI_Barrier(MPI_COMM_WORLD);
          /* update A */
          if (tile_size < order) {
            OMP_FOR()
            for (int it=0; it<row_per_pe; it+=tile_size) {
              for (int jt=0; jt<order; jt+=tile_size) {
                for (int i=it; i<MIN(row_per_pe,it+tile_size); i++) {
                  OMP_SIMD()
                  for (int j=jt; j<MIN(order,jt+tile_size); j++) {
                    A[i][j] += 1.0;
                  }
                }
              }
            }
          } else {
            OMP_FOR()
            for (int i=0;i<row_per_pe; i++) {
              OMP_SIMD()
              for (int j=0;j<order;j++) {
                A[i][j] += 1.0;
              }
            }
          }

        } /* end iterations */
        OMP_BARRIER
        OMP_MASTER
        {
          MPI_Barrier(MPI_COMM_WORLD);
          trans_time = wtime() - trans_time;
        }

    } /* end OMP_PARALLEL */

#if 0
      /* debug */
      for (int r=0; r<npes; r++) {
        if (me==r) {
          for (int j=0; j<row_per_pe; j++) {
            for (int i=0; i<order; i++) {
              printf("%s %d: %d,%d,%lf\n","AFTER",me,j,i,B[j][i]);
            }
          }
          fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
      }
#endif

    /*********************************************************************
    ** Analyze and output results.
    *********************************************************************/

    double abserr = 0.0;
    const double addit = ((double)(iterations+1) * (double) (iterations))/2.0;
    OMP_PARALLEL(shared(abserr))
    {
        OMP_FOR(reduction(+:abserr))
        for (int j=0;j<row_per_pe;j++) {
          for (int i=0;i<order; i++) {
            const size_t offset_ij = (size_t)i*(size_t)order+(size_t)j;
            abserr += fabs(B[j][i] - ((double)offset_ij*(iterations+1.)+addit));
          }
        }
    } /* end OMP_PARALLEL */
    MPI_Allreduce(MPI_IN_PLACE,&abserr,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);

    prk_free(TB);
    prk_free(TA);
    prk_free(B);
    prk_free(A);

#ifdef VERBOSE
    if (me==0) {
        printf("Sum of absolute differences: %f\n",abserr);
    }
#endif

    const double epsilon = 1.e-8;
    int exit_code = 0;
    if (abserr < epsilon) {
        double avgtime = trans_time/iterations;
        if (me==0) {
            printf("Solution validates\n");
            printf("Rate (MB/s): %lf Avg time (s): %lf\n", 1.0E-06 * (2L*bytes)/avgtime, avgtime);
        }
    }
    else {
        if (me==0) {
            printf("ERROR: Aggregate squared error %e exceeds threshold %e\n", abserr, epsilon);
        }
        exit_code = 1;
    }
    MPI_Finalize();

    return exit_code;
}


