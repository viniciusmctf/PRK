#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 2
#SBATCH -t 00:10:00
#SBATCH -J transpose_chapel_0002
#SBATCH -o transpose_chapel_0002.out

# Run 25 iterations
export AMMPI_MPI_THREAD=multiple
export MPICH_MAX_THREAD_SAFETY=multiple
date
srun --nodes=2 --ntasks=2 --tasks-per-node=1 --cpus-per-task=48 ../../../CHAPEL/ChapelMPI/Transpose/transpose.x -nl 2 --iterations=25 --order=49152 --tile=32
date
