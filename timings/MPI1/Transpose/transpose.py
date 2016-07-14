import subprocess

template="""#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N %d
#SBATCH -t 00:03:00
#SBATCH -J transpose_mpi_%d
#SBATCH -o transpose_mpi_%d.out

# Run 25 iterations
date
srun -v -n %d ../../../MPI1/Transpose/transpose 25 49152
date
"""

nnodes=[1,2,4,8,16,32,64]
procpernode=24 # On Edison

for inode in nnodes :
  fn = "transpose_%d.sh"%inode
  comm=template%(inode,inode,inode,inode*procpernode)
  ff = open(fn,"w")
  ff.write(comm)
  ff.close()
  subprocess.call(["sbatch",fn])
  print fn

