import subprocess

template="""#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N %d
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_%04d
#SBATCH -o transpose_mpi_%04d.out

# Run 25 iterations
date
srun -n %d ../../../MPI1/Transpose/transpose %d 49152
date
"""

nnodes=[1,2,4,8,16,32,64,128,256]
procpernode=24 # On Edison

for inode in nnodes :
  fn = "transpose_%04d.sh"%inode
  if inode > 16 :
    niter = 50
  else :
    niter = 25
  comm=template%(inode,inode,inode,inode*procpernode, niter)
  ff = open(fn,"w")
  ff.write(comm)
  ff.close()
  subprocess.call(["sbatch",fn])
  print fn

