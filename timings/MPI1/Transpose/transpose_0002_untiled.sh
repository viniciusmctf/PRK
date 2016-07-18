#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 2
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_0002
#SBATCH -o transpose_mpi_0002_untiled.out

# Run 25 iterations
date
srun -v -n 48 ../../../MPI1/Transpose/transpose 25 49152 0
date
