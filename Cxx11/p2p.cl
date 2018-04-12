#pragma OPENCL EXTENSION cl_khr_fp64 : enable

// INFO (this is not the best way to get this info...)

__kernel void info(const int n, __global void * grid)
{
    const int gi0 = get_global_id(0);
    const int gi1 = get_global_id(1);
    const int li0 = get_local_id(0);
    const int li1 = get_local_id(1);
    if (gi0==0 && li0==0) {
        unsigned wd  = get_work_dim();
        printf("get_work_dim()     = %u\n", wd);
        for (unsigned d=0; d<wd; d++) {
          size_t gs0 = get_global_size(0);
          size_t ls0 = get_local_size(0);
          size_t ng0 = get_num_groups(0);
          printf("get_global_size(%u) = %lu\n", d, gs0);
          printf("get_local_size(%u)  = %lu\n", d, ls0);
          printf("get_num_groups(%u)  = %lu\n", d, ng0);
        }
    }
}

// CONSOLIDATED (the whole sweep at once)

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
    if (j==0) {
      grid[0] = -grid[n*n-1];
    }
    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
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

// INTERMEDIATE/ITERATION (a single anti-diagonal)

__kernel void p2p32i(const int i, const int n, __global float * grid)
{
    const int j = get_global_id(0);
    // for (int j=std::max(2,i-n+2); j<=std::min(i,n); j++) {
    if ( ( j >= max(2,i-n+2) ) && ( j <= min(i,n) ) )
    {
        const int x = i-j+2-1;
        const int y = j-1;
        grid[x*n+y] = grid[(x-1)*n+  y  ]
                    + grid[  x  *n+(y-1)]
                    - grid[(x-1)*n+(y-1)];
    }
}

__kernel void p2p64i(const int i, const int n, __global double * grid)
{
    const int j = get_global_id(0);
    // for (int j=std::max(2,i-n+2); j<=std::min(i,n); j++) {
    if ( ( j >= max(2,i-n+2) ) && ( j <= min(i,n) ) )
    {
        const int x = i-j+2-1;
        const int y = j-1;
        grid[x*n+y] = grid[(x-1)*n+  y  ]
                    + grid[  x  *n+(y-1)]
                    - grid[(x-1)*n+(y-1)];
    }
}

// FINAL KERNEL (the corner to corner part)

__kernel void p2p32f(const int n, __global float * grid)
{
    const int j = get_global_id(0);
    if (j==0) {
      grid[0] = -grid[n*n-1];
    }
}

__kernel void p2p64f(const int n, __global double * grid)
{
    const int j = get_global_id(0);
    if (j==0) {
      grid[0] = -grid[n*n-1];
    }
}
