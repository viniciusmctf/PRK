///
/// Copyright (c) 2013, Intel Corporation
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///
/// * Redistributions of source code must retain the above copyright
///       notice, this list of conditions and the following disclaimer.
/// * Redistributions in binary form must reproduce the above
///       copyright notice, this list of conditions and the following
///       disclaimer in the documentation and/or other materials provided
///       with the distribution.
/// * Neither the name of Intel Corporation nor the names of its
///       contributors may be used to endorse or promote products
///       derived from this software without specific prior written
///       permission.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
/// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
/// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
/// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
/// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
/// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
/// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
/// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
/// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
/// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.

//////////////////////////////////////////////////////////////////////
///
/// NAME:    Pipeline
///
/// PURPOSE: This program tests the efficiency with which point-to-point
///          synchronization can be carried out. It does so by executing
///          a pipelined algorithm on an m*n grid. The first array dimension
///          is distributed among the threads (stripwise decomposition).
///
/// USAGE:   The program takes as input the
///          dimensions of the grid, and the number of iterations on the grid
///
///                <progname> <iterations> <m> <n>
///
///          The output consists of diagnostics to make sure the
///          algorithm worked, and of timing statistics.
///
/// FUNCTIONS CALLED:
///
///          Other than standard C functions, the following
///          functions are used in this program:
///
///          wtime()
///
/// HISTORY: - Written by Rob Van der Wijngaart, February 2009.
///            C99-ification by Jeff Hammond, February 2016.
///            C++11-ification by Jeff Hammond, May 2017.
///            TBB-flowgraph by Jeff Hammond*, July 2017
///
/// * based upon https://software.intel.com/en-us/blogs/2011/09/09/implementing-a-wave-front-computation-using-the-intel-threading-building-blocks-flow-graph
///
//////////////////////////////////////////////////////////////////////

#include "prk_util.h"

void sweep_tile(int startm, int endm,
                int startn, int endn,
                int n, std::vector<double> & grid)
{
  std::cout << "sweep_tile(" << startm << "," << endm << "," << startn << "," << endn << ",..)" << std::endl;
  for (auto i=startm; i<endm; i++) {
    for (auto j=startn; j<endn; j++) {
      grid[i*n+j] = grid[(i-1)*n+j] + grid[i*n+(j-1)] - grid[(i-1)*n+(j-1)];
    }
  }
}

int main(int argc, char* argv[])
{
  std::cout << "Parallel Research Kernels version " << PRKVERSION << std::endl;
  std::cout << "C++11/TBB pipeline execution on 2D grid" << std::endl;

  tbb::task_scheduler_init init(tbb::task_scheduler_init::automatic);
  auto num_threads = init.default_num_threads();

  //////////////////////////////////////////////////////////////////////
  // Process and test input parameters
  //////////////////////////////////////////////////////////////////////

  int iterations;
  int m, n;
  int mc, nc;
  try {
      if (argc < 4){
        throw " <# iterations> <first array dimension> <second array dimension> [<first chunk dimension> <second chunk dimension>]";
      }

      // number of times to run the pipeline algorithm
      iterations  = std::atoi(argv[1]);
      if (iterations < 1) {
        throw "ERROR: iterations must be >= 1";
      }

      // grid dimensions
      m = std::atoi(argv[2]);
      n = std::atoi(argv[3]);
      if (m < 1 || n < 1) {
        throw "ERROR: grid dimensions must be positive";
      } else if ( static_cast<size_t>(m)*static_cast<size_t>(n) > INT_MAX) {
        throw "ERROR: grid dimension too large - overflow risk";
      }

      // chunk size for each grid dimension
      mc = (argc > 4) ? std::atoi(argv[4]) : m/num_threads;
      nc = (argc > 5) ? std::atoi(argv[5]) : n/num_threads;
      if (mc < 1 || mc > m || nc < 1 || nc > n) {
        std::cout << "WARNING: grid chunk dimensions invalid: " << mc <<  nc << " (ignoring)" << std::endl;
        mc = m;
        nc = n;
      }
  }
  catch (const char * e) {
    std::cout << e << std::endl;
    return 1;
  }

  std::cout << "Number of iterations = " << iterations << std::endl;
  std::cout << "Grid sizes           = " << m << ", " << n << std::endl;
  std::cout << "Grid chunk sizes     = " << mc << ", " << nc << std::endl;

  //////////////////////////////////////////////////////////////////////
  // Allocate space and perform the computation
  //////////////////////////////////////////////////////////////////////

  auto pipeline_time = 0.0;

  std::vector<double> grid;
  grid.resize(m*n,0.0);

  for (auto j=0; j<n; j++) {
    grid[0*n+j] = static_cast<double>(j);
  }
  for (auto i=0; i<m; i++) {
    grid[i*n+0] = static_cast<double>(i);
  }

  // number of blocks
  int mb = (m/mc) + (m%mc > 0);
  int nb = (n/nc) + (n%nc > 0);
  std::cout << "mb,nb=" << mb << "," << nb << std::endl;

  tbb::flow::graph graph;
  std::vector< tbb::flow::continue_node<tbb::flow::continue_msg> * > nodes;
  nodes.resize((mb+1)*(nb+1));
  for( int i=mb; i>=0; i--) {
    for( int j=nb; j>=0; j--) {
      std::cout << "i,j=" << i << "," << j << std::endl;
      nodes[i*mb+j] =
        new tbb::flow::continue_node<tbb::flow::continue_msg>( graph,
                         [=,&grid]( const tbb::flow::continue_msg & ) {
                             //std::cout << "i,j=" << i << "," << j << std::endl;
                             int start_i = i*mc;
                             int end_i   = std::min(m,(i+1)*mc);
                             int start_j = j*nc;
                             int end_j   = std::min(n,(j+1)*nc);
                             //std::cout << "args=" << start_i << "," << end_i << "," << start_j << "," << end_j << std::endl;
                             sweep_tile(start_i, end_i, start_j, end_j, n, grid);
                         } );
      if ( i + 1 < mb ) tbb::flow::make_edge( *nodes[i*mb + j], *nodes[(i+1)*mb + j] );
      if ( j + 1 < nb ) tbb::flow::make_edge( *nodes[i*mb + j], *nodes[i*mb + (j+1)] );
    }
  }

  for (auto iter = 0; iter<=iterations; iter++) {

    if (iter==1) pipeline_time = prk::wtime();

#if 0
    for (auto i=1; i<m; i+=mc) {
      for (auto j=1; j<n; j+=nc) {
        sweep_tile(i, std::min(m,i+mc), j, std::min(n,j+nc), n, grid);
      }
    }
#endif
    nodes[0]->try_put(tbb::flow::continue_msg());
    graph.wait_for_all();
    grid[0] = -grid[(m-1)*n+(n-1)];
  }

  pipeline_time = prk::wtime() - pipeline_time;

  //////////////////////////////////////////////////////////////////////
  // Analyze and output results.
  //////////////////////////////////////////////////////////////////////

  // error tolerance
  const double epsilon = 1.e-8;

  // verify correctness, using top right value
  auto corner_val = ((iterations+1.)*(n+m-2.));
  if ( (std::fabs(grid[(m-1)*n+(n-1)] - corner_val)/corner_val) > epsilon) {
    std::cout << "ERROR: checksum " << grid[(m-1)*n+(n-1)]
              << " does not match verification value " << corner_val << std::endl;
    return 1;
  }

#ifdef VERBOSE
  std::cout << "Solution validates; verification value = " << corner_val << std::endl;
#else
  std::cout << "Solution validates" << std::endl;
#endif
  auto avgtime = pipeline_time/iterations;
  std::cout << "Rate (MFlops/s): "
            << 2.0e-6 * ( (m-1)*(n-1) )/avgtime
            << " Avg time (s): " << avgtime << std::endl;

  return 0;
}
