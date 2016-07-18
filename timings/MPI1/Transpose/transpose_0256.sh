#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 256
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0256
#SBATCH -o transpose_mpi_0256.out

# Run 25 iterations
date
srun -n 6144 ../../../MPI1/Transpose/transpose 50 49152
date
