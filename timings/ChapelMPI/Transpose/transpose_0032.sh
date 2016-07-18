#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 32
#SBATCH -t 00:10:00
#SBATCH -J transpose_chapel_0032
#SBATCH -o transpose_chapel_0032.out

# Run 25 iterations
export AMMPI_MPI_THREAD=multiple
export MPICH_MAX_THREAD_SAFETY=multiple
date
srun --nodes=32 --ntasks=32 --tasks-per-node=1 --cpus-per-task=48 ../../../CHAPEL/ChapelMPI/Transpose/transpose.x -nl 32 --iterations=25 --order=49152 --tile=32
date
