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

NAME:    Pipeline

PURPOSE: This program tests the efficiency with which point-to-point
         synchronization can be carried out. It does so by executing 
         a pipelined algorithm on an m*n grid. The first array dimension
         is distributed among the threads (stripwise decomposition).
  
USAGE:   The program takes as input the
         dimensions of the grid, and the number of iterations on the grid

               <progname> <iterations> <m> <n>
  
         The output consists of diagnostics to make sure the 
         algorithm worked, and of timing statistics.

FUNCTIONS CALLED:

         Other than standard C functions, the following 
         functions are used in this program:

         wtime()

HISTORY: - Written by Rob Van der Wijngaart, February 2009.
           C99-ification by Jeff Hammond, February 2016.
*******************************************************************/

#include <prk_util.h>

#include <math.h> /* fabs */

/* error tolerance */
const double epsilon = 1.e-8;

double p2p(int iterations, int m, int n, double (* restrict vector)[n])
{
  double pipeline_time = -9999.9999;

#ifdef _OPENMP
  _Pragma("omp parallel shared(iterations,m,n,vector,pipeline_time)")
  {
    int nt = omp_get_num_threads();
    int me = omp_get_thread_num();

    for (int iter = 0; iter<=iterations; iter++){

      /* start timer after a warmup iteration */
      if (iter == 1) {
        _Pragma("omp barrier")
        _Pragma("omp master")
        pipeline_time = wtime();
      }

      _Pragma("omp master")
      {
        for (int i=1; i<m; i+=nt) {
          for (int j=1; j<n; j++) {
            vector[i][j] = vector[i][j-1] + vector[i-1][j] - vector[i-1][j-1];
          }
        }

        /* copy top right corner value to bottom left corner to create dependency; we
           need a barrier to make sure the latest value is used. This also guarantees
           that the flags for the next iteration (if any) are not getting clobbered  */
        vector[0][0] = -vector[m-1][n-1];
      } /* omp master */
    } /* iterations */

    _Pragma("omp barrier")
    _Pragma("omp master")
    pipeline_time = wtime() - pipeline_time;
  } /* omp parallel */

#else /* SEQUENTIAL */

  for (int iter = 0; iter<=iterations; iter++) {

    /* start timer after a warmup iteration */
    if (iter == 1) pipeline_time = wtime();

    for (int i=1; i<m; i++) {
      for (int j=1; j<n; j++) {
        vector[i][j] = vector[i-1][j] + vector[i][j-1] - vector[i-1][j-1];
      }
    }

    /* copy top right corner value to bottom left corner to create dependency; we
       need a barrier to make sure the latest value is used. This also guarantees
       that the flags for the next iteration (if any) are not getting clobbered  */
    vector[0][0] = -vector[m-1][n-1];

  } /* iterations */

  pipeline_time = wtime() - pipeline_time;

#endif

  return pipeline_time;
}

int main(int argc, char ** argv)
{
  /*******************************************************************************
  ** process and test input parameters
  ********************************************************************************/

  printf("Parallel Research Kernels version %s\n", PRKVERSION);
  printf("Serial pipeline execution on 2D grid\n");

  if (argc != 4){
    printf("Usage: %s <# iterations> <first array dimension> ", *argv);
    printf("<second array dimension>\n");
    return(EXIT_FAILURE);
  }

  /* number of times to run the pipeline algorithm */
  int iterations  = atoi(argv[1]);
  if (iterations < 1){
    printf("ERROR: iterations must be >= 1 : %d \n",iterations);
    exit(EXIT_FAILURE);
  }

  /* grid dimensions */
  int m  = atol(argv[2]);
  int n  = atol(argv[3]);

  if (m < 1 || n < 1){
    printf("ERROR: grid dimensions must be positive: %d, %d \n", m, n);
    exit(EXIT_FAILURE);
  }

  /* total required length to store grid values */
  size_t bytes = (size_t)m*(size_t)n*sizeof(double);

  /* working set */
  double (* restrict vector)[n] = (double (*)[n]) prk_malloc(bytes);
  if (vector==NULL) {
    printf("ERROR: Could not allocate space for array: %zu\n", bytes);
    exit(EXIT_FAILURE);
  }

  printf("Grid sizes                = %d, %d\n", m, n);
  printf("Number of iterations      = %d\n", iterations);

  /* clear the array */
  for (int i=0; i<m; i++) {
    for (int j=0; j<n; j++) {
      vector[i][j] = 0.0;
    }
  }
  /* set boundary values (bottom and left side of grid) */
  for (int j=0; j<n; j++) {
    vector[0][j] = (double)j;
  }
  for (int i=0; i<m; i++) {
    vector[i][0] = (double)i;
  }

  double pipeline_time = p2p(iterations,m,n,vector);

  /*******************************************************************************
  ** Analyze and output results.
  ********************************************************************************/

  /* verify correctness, using top right value;                                  */
  double corner_val = (double)((iterations+1)*(n+m-2));
  if ( (fabs(vector[m-1][n-1] - corner_val)/corner_val) > epsilon) {
    printf("ERROR: checksum %lf does not match verification value %lf\n",
           vector[m-1][n-1], corner_val);
    exit(EXIT_FAILURE);
  }

  prk_free(vector);

#ifdef VERBOSE
  printf("Solution validates; verification value = %lf\n", corner_val);
#else
  printf("Solution validates\n");
#endif
  double avgtime = pipeline_time/iterations;
  printf("Rate (MFlops/s): %lf Avg time (s): %lf\n",
         2.0e-6 * ((double)((size_t)(m-1)*(size_t)(n-1)))/avgtime, avgtime);

  exit(EXIT_SUCCESS);

  return 0;
}
