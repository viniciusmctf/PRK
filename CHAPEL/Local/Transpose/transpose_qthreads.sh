#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 1
#SBATCH -t 00:15:00
#SBATCH -J transpose_chapel_qthreads
#SBATCH -o transpose_chapel_qthreads.out

# Run 25 iterations
date
srun -n 1 ./transpose_qthreads.x --about
srun --nodes=1 --ntasks=1 --tasks-per-node=1 --cpus-per-task=48 ./transpose_qthreads.x --iterations=25 --order=49152 --tile=32
date
