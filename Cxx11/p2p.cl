#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void p2p32(const int n, __global float * grid)
{
    const int j = get_global_id(0);
    for (int i=2; i<=2*n-2; i++) {
      // for (int j=std::max(2,i-n+2); j<=std::min(i,n); j++) {
      if ( ( j >= max(2,i-n+2) ) && ( j <= min(i,n) ) )
      {
          const int x = i-j+2-1;
          const int y = j-1;
          grid[x*n+y] = grid[(x-1)*n+  y  ]
                      + grid[  x  *n+(y-1)]
                      - grid[(x-1)*n+(y-1)];
      }
      barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
    }
#if 1
    if (j==0) {
      grid[0] = -grid[n*n-1];
    }
    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
#endif
}

__kernel void p2p64(const int n, __global double * grid)
{
    const int j = get_global_id(0);
    for (int i=2; i<=2*n-2; i++) {
      // for (int j=std::max(2,i-n+2); j<=std::min(i,n); j++) {
      if ( ( j >= max(2,i-n+2) ) && ( j <= min(i,n) ) )
      {
          const int x = i-j+2-1;
          const int y = j-1;
          grid[x*n+y] = grid[(x-1)*n+  y  ]
                      + grid[  x  *n+(y-1)]
                      - grid[(x-1)*n+(y-1)];
      }
      barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
    }
#if 1
    if (j==0) {
      grid[0] = -grid[n*n-1];
    }
    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
#endif
}
