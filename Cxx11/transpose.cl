//
// This is a NAIVE implementation that may perform badly.
//
// Examples of better implementations include:
// - https://developer.apple.com/library/content/samplecode/OpenCL_Matrix_Transpose_Example/Introduction/Intro.html
// - https://github.com/sschaetz/nvidia-opencl-examples/blob/master/OpenCL/src/oclTranspose/transpose.cl
//

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void transpose32(const int order, __global float * a, __global float * b, int tile_size, __local float * t)
{
    const int i = get_global_id(0);
    const int j = get_global_id(1);

    if (order < 10) {
        if (get_global_id(0) == 0 && get_global_id(1) == 0 && get_local_id(0) == 0 && get_local_id(1) == 0) {
            printf("global_size = (%d,%d) group_size = (%d,%d) local_size  = (%d,%d)\n",
                   get_global_size(0), get_global_size(1),
                   get_num_groups(0), get_num_groups(1),
                   get_local_size(0), get_local_size(1));
        }
        printf("global_id = (%d,%d) group_id = (%d,%d) local_id  = (%d,%d)\n",
               get_global_id(0), get_global_id(1),
               get_group_id(0), get_group_id(1),
               get_local_id(0), get_local_id(1));

        printf("enumerating i,j=%d,%d\n", i, j);
    }

    if ((i<order) && (j<order)) {
        b[i*order+j] += a[j*order+i];
        a[j*order+i] += 1.0;
    }
}

__kernel void transpose64(const int order, __global double * a, __global double * b, int tile_size, __local double * t)
{
    const int i = get_global_id(0);
    const int j = get_global_id(1);

    printf("i,j=%d,%d\n", i, j);
    if ((i<order) && (j<order)) {
        b[i*order+j] += a[j*order+i];
        a[j*order+i] += 1.0;
    }
}
