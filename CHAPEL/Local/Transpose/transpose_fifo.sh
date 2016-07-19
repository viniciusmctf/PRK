#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 1
#SBATCH -t 00:15:00
#SBATCH -J transpose_chapel_fifo
#SBATCH -o transpose_chapel_fifo.out

# Run 25 iterations
date
srun -n 1 ./transpose_fifo.x --about
srun --nodes=1 --ntasks=1 --tasks-per-node=1 --cpus-per-task=48 ./transpose_fifo.x --iterations=25 --order=49152 --tile=32
date
