#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 32
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0032
#SBATCH -o transpose_mpi_0032.out

# Run 25 iterations
date
srun -n 768 ../../../MPI1/Transpose/transpose 50 49152
date
