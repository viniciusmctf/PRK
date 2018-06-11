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
/// NAME:    transpose
///
/// PURPOSE: This program measures the time for the transpose of a
///          column-major stored matrix into a row-major stored matrix.
///
/// USAGE:   Program input is the matrix order and the number of times to
///          repeat the operation:
///
///          transpose <matrix_size> <# iterations> [tile size]
///
///          An optional parameter specifies the tile size used to divide the
///          individual matrix blocks for improved cache and TLB performance.
///
///          The output consists of diagnostics to make sure the
///          transpose worked and timing statistics.
///
/// HISTORY: Written by  Rob Van der Wijngaart, February 2009.
///          Converted to C++11 by Jeff Hammond, February 2016 and May 2017.
///
//////////////////////////////////////////////////////////////////////

#include "prk_util.h"
#include "prk_opencl.h"

template <typename T>
void run(cl::Context context, int iterations, int order, int tile_size)
{
  auto precision = (sizeof(T)==8) ? 64 : 32;

  cl::Program program(context, prk::opencl::loadProgram("transpose.cl"), true);

  auto function = (precision==64) ? "transpose64" : "transpose32";

  cl_int err;
  auto kernel = cl::make_kernel<int, cl::Buffer, cl::Buffer, int, cl::LocalSpaceArg>(program, function, &err);
  if(err != CL_SUCCESS){
    std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
  }

  cl::CommandQueue queue(context);

  //////////////////////////////////////////////////////////////////////
  /// Allocate space for the input and transpose matrix
  //////////////////////////////////////////////////////////////////////

  const size_t nelems = (size_t)order * (size_t)order;
  std::vector<T> h_a(nelems);
  std::vector<T> h_b(nelems, T(0));

  // fill A with the sequence 0 to order^2-1 as doubles
  std::iota(h_a.begin(), h_a.end(), (T)0);

  // copy input from host to device
  cl::Buffer d_a = cl::Buffer(context, begin(h_a), end(h_a), true);
  cl::Buffer d_b = cl::Buffer(context, begin(h_b), end(h_b), true);
  auto d_t = cl::__local((tile_size+1)*tile_size*sizeof(T));

  double trans_time(0);

  for (auto iter = 0; iter<=iterations; iter++) {

    if (iter==1) trans_time = prk::wtime();

    // transpose the matrix
    kernel(cl::EnqueueArgs(queue, cl::NDRange(order,order)), order, d_a, d_b, tile_size, d_t);
    queue.finish();
#if DEBUG
    if (order<20) {
        cl::copy(queue, d_a, begin(h_a), end(h_a));
        cl::copy(queue, d_b, begin(h_b), end(h_b));
        for (int i=0; i<order; ++i) {
          for (int j=0; j<order; ++j) {
              std::cout << "(" << i << "," << j << ") = " << h_a[i*order+j] << ", " << h_b[i*order+j] << "\n";
          }
        }
    }
#endif
  }
  trans_time = prk::wtime() - trans_time;

  // copy output back to host
  cl::copy(queue, d_b, begin(h_b), end(h_b));

  // compute error
  const double addit = (iterations+1.0) * (0.5*iterations);
  double abserr = 0.0;
  for (auto j=0; j<order; j++) {
    for (auto i=0; i<order; i++) {
      const int ij = i*order+j;
      const int ji = j*order+i;
      const double reference = static_cast<double>(ij)*(iterations+1)+addit;
      abserr += std::fabs(static_cast<double>(h_b[ji]) - reference);
    }
  }
  //
  //////////////////////////////////////////////////////////////////////
  /// Analyze and output results
  //////////////////////////////////////////////////////////////////////

#ifdef VERBOSE
  std::cout << "Sum of absolute differences: " << abserr << std::endl;
#endif

  const double epsilon = (precision==64) ? 1.0e-8 : 1.0e-4;
  if (abserr < epsilon) {
    std::cout << "Solution validates" << std::endl;
    auto avgtime = trans_time/iterations;
    auto bytes = (size_t)order * (size_t)order * sizeof(double);
    std::cout << ( (precision==64) ? "64b" : "32b" )
              << " Rate (MB/s): " << 1.0e-6 * (2.*bytes)/avgtime
              << " Avg time (s): " << avgtime << std::endl;
  } else {
    std::cout << "ERROR: Aggregate squared error " << abserr
              << " exceeds threshold " << epsilon << std::endl;
  }
}

int main(int argc, char* argv[])
{
  std::cout << "Parallel Research Kernels version " << PRKVERSION << std::endl;
  std::cout << "C++11/OpenCL Matrix transpose: B = A^T" << std::endl;

  prk::opencl::listPlatforms();

  //////////////////////////////////////////////////////////////////////
  /// Read and test input parameters
  //////////////////////////////////////////////////////////////////////

  int iterations;
  int order;
  int tile_size;
  try {
      if (argc < 3) {
        throw "Usage: <# iterations> <matrix order> [tile size]";
      }

      // number of times to do the transpose
      iterations  = std::atoi(argv[1]);
      if (iterations < 1) {
        throw "ERROR: iterations must be >= 1";
      }

      // order of a the matrix
      order = std::atol(argv[2]);
      if (order <= 0) {
        throw "ERROR: Matrix Order must be greater than 0";
      } else if (order > std::floor(std::sqrt(INT_MAX))) {
        throw "ERROR: matrix dimension too large - overflow risk";
      }

      // default tile size for tiling of local transpose
      tile_size = (argc>3) ? std::atoi(argv[3]) : 32;
      // a negative tile size means no tiling of the local transpose
      if (tile_size <= 0) tile_size = order;
  }
  catch (const char * e) {
    std::cout << e << std::endl;
    return 1;
  }

  std::cout << "Number of iterations = " << iterations << std::endl;
  std::cout << "Matrix order         = " << order << std::endl;
  std::cout << "Tile size            = " << tile_size << std::endl;

  //////////////////////////////////////////////////////////////////////
  /// Setup OpenCL environment
  //////////////////////////////////////////////////////////////////////

  cl_int err = CL_SUCCESS;

  cl::Context cpu(CL_DEVICE_TYPE_CPU, NULL, NULL, NULL, &err);
  if ( err == CL_SUCCESS && prk::opencl::available(cpu) )
  {
    const int precision = prk::opencl::precision(cpu);

    std::cout << "CPU Precision        = " << precision << "-bit" << std::endl;

    if (precision==64) {
<<<<<<< HEAD
        run<double>(cpu, iterations, order, tile_size);
    }
    run<float>(cpu, iterations, order, tile_size);
=======
        run<double>(cpu, iterations, order);
    }
    run<float>(cpu, iterations, order);
>>>>>>> f7defbce8cbe86ba015f71fe2779ab2aeea7812e
  }

  cl::Context gpu(CL_DEVICE_TYPE_GPU, NULL, NULL, NULL, &err);
  if ( err == CL_SUCCESS && prk::opencl::available(gpu) )
  {
    const int precision = prk::opencl::precision(gpu);

    std::cout << "GPU Precision        = " << precision << "-bit" << std::endl;

    if (precision==64) {
<<<<<<< HEAD
        run<double>(gpu, iterations, order, tile_size);
    }
    run<float>(gpu, iterations, order, tile_size);
=======
        run<double>(gpu, iterations, order);
    }
    run<float>(gpu, iterations, order);
>>>>>>> f7defbce8cbe86ba015f71fe2779ab2aeea7812e
  }

  cl::Context acc(CL_DEVICE_TYPE_ACCELERATOR, NULL, NULL, NULL, &err);
  if ( err == CL_SUCCESS && prk::opencl::available(acc) )
  {
    const int precision = prk::opencl::precision(acc);

    std::cout << "ACC Precision        = " << precision << "-bit" << std::endl;

    if (precision==64) {
<<<<<<< HEAD
        run<double>(acc, iterations, order, tile_size);
    }
    run<float>(acc, iterations, order, tile_size);
=======
        run<double>(acc, iterations, order);
    }
    run<float>(acc, iterations, order);
>>>>>>> f7defbce8cbe86ba015f71fe2779ab2aeea7812e
  }

  return 0;
}
