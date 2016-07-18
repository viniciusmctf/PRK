import subprocess

template="""#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N %d
#SBATCH -t 00:10:00
#SBATCH -J transpose_chapel_%04d
#SBATCH -o transpose_chapel_%04d.out

# Run 25 iterations
export AMMPI_MPI_THREAD=multiple
export MPICH_MAX_THREAD_SAFETY=multiple
date
%s ../../../CHAPEL/ChapelMPI/Transpose/transpose.x -nl %i --iterations=%i --order=49152 --tile=32
date
"""

nnodes=[1,2,4,8,16,32,64]
nnodes=[2,4,8,16,32,64,128,256]
procpernode=24 # On Edison
hyper=2

for inode in nnodes :
  fn = "transpose_%04d.sh"%inode 
  if inode > 32 :
    niter = 50
  else :
    niter = 25
  srun = "srun --nodes=%i --ntasks=%i --tasks-per-node=1 --cpus-per-task=%i"%(inode,inode,hyper*procpernode)
  comm=template%(inode,inode,inode,srun,inode,niter)
  ff = open(fn,"w")
  ff.write(comm)
  ff.close()
  subprocess.call(["sbatch",fn])
  print fn

