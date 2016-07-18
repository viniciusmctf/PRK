#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 64
#SBATCH -t 00:10:00
#SBATCH -J transpose_chapel_0064
#SBATCH -o transpose_chapel_0064.out

# Run 25 iterations
export AMMPI_MPI_THREAD=multiple
export MPICH_MAX_THREAD_SAFETY=multiple
date
srun --nodes=64 --ntasks=64 --tasks-per-node=1 --cpus-per-task=48 ../../../CHAPEL/ChapelMPI/Transpose/transpose.x -nl 64 --iterations=25 --order=49152 --tile=32
date
