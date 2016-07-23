/* Synch_p2p PRK

Notes :
 1. The PRK versions assume a column-major storage order for the
 array. In order to simply use multi-dimensional arrays, we will transpose
 the operations. So, the i, j values are all swapped.
*/

use Time;
use Barrier;
use MPI;
use C_MPI;

const epsilon=1.0e-8;

config const m=64,
             n=64,
             iterations=2,
             ntasks = here.maxTaskPar;


const rank = commRank(CHPL_COMM_WORLD),
      size = commSize(CHPL_COMM_WORLD),
      finalRank = rank == (size-1); // This does the validation, and printing

extern const MPI_REQUEST_NULL : MPI_Request;

proc main() {

  /* Print useful information */
  if finalRank {
    writef("Parallel Research Kernels\n");
    writef("Synch_p2p\n");
    writef("Number of ranks = %i\n",size);
    writef("Number of tasks = %i\n", ntasks);
    writef("Grid size = %i, %i \n",m,n);
    writef("Number of iterations = %i \n",iterations);
  }

  /* Define the grid */
  var FullSpace = {0.. #n, 0.. #m};
  var blockSize = m/size, 
      blockRem = m%size;
  var Space : domain(2);
  var blockStart : int;
  if rank < blockRem {
    blockSize += 1;
    blockStart = rank * (blockSize + 1);
  } else {
    blockStart = m - (size-rank)*blockSize;
  }
  // Include buffer space 
  if rank==0 {
    Space = FullSpace[.., blockStart.. #blockSize]; 
  } else {
    Space = FullSpace[.., (blockStart-1).. #(blockSize+1)];
  }
  var blockEnd = Space.high(2);
  var IterSpaceI = Space.dim(2)[1..(m-1)];
  var IterSpaceJ = 1..(n-1);
  var A : [Space] real;
  A = 0.0;
  // Set boundary values
  if rank==0 then [j in 0.. #n] A[j,0] = j:real;
  [i in Space.dim(2)] A[0,i] = i:real;

  // Define synchronization flags
  var flags : [IterSpaceJ, 0.. #ntasks] sync bool;
  //flags[..,0] = true; // Task 0 is ready to go

  // Define communication buffers
  var sendBuffer : [IterSpaceJ] real;
  var requests : [IterSpaceJ] MPI_Request = MPI_REQUEST_NULL;

  
  // Get ready for iterations
  var t : Timer;
  t.clear();
  var firstiter = new Barrier(ntasks); // Timing barrier
                                       // No reuse necessary



  // Start a loop over the tasks here 
  coforall itask in 0.. #ntasks with (ref t) {

    // Define the space we're iterating over
    var segment = blockSize/ntasks;
    var remainder = blockSize%ntasks;
    var ThreadIterSpaceI : domain(1);
    if itask < remainder {
      var start = itask*(segment+1) + blockStart;
      ThreadIterSpaceI = IterSpaceI[start.. #(segment+1)];
    }
    else {
      var start = blockEnd+1-(ntasks-itask)*segment;
      ThreadIterSpaceI = IterSpaceI[start.. #segment];
    }

    for iiter in 0..iterations {

      if iiter == 1 {
        if itask==0 then MPI_Barrier(CHPL_COMM_WORLD);
        firstiter.barrier();
        if itask==0 then t.start();
      }

      // This loop cannot be in parallel, since
      // there are data dependencies. That is the 
      // entire point of this. I currently do this as
      // a double loop, since we need to block on each j
      for j in IterSpaceJ {

        // Sync and receive if necessary 
        if itask==0 { // Leftmost task
          if rank!=0 { // Not first rank
            var stat : MPI_Status;
            MPI_Recv(A[j,blockStart-1],1,MPI_DOUBLE,rank-1,j:c_int,CHPL_COMM_WORLD,stat);
          }
        } else {
          flags[j,itask]; // Empty variable
        }

        // Iterate over the row
        for i in ThreadIterSpaceI do
          A[j,i] = A[j,i-1] + A[j-1,i] - A[j-1,i-1];

        // Let the right neighbour go
        if itask != (ntasks-1) {
          flags[j,itask+1]=true;
        } else {
          if !finalRank {
            var stat : MPI_Status;
            MPI_Wait(requests[j], stat);
            sendBuffer[j] = A[j, blockEnd];
            MPI_Isend(sendBuffer[j], 1, MPI_DOUBLE, rank+1, j:c_int, CHPL_COMM_WORLD, requests[j]);
          }
        }

      }

      // Corner term
      if finalRank {
        var stat : MPI_Status;
        sendBuffer[n-1] = -A[n-1,m-1];
        MPI_Wait(requests[n-1],stat);
        MPI_Isend(sendBuffer[n-1],1, MPI_DOUBLE, 0, 0, CHPL_COMM_WORLD, requests[n-1]);
      } 
      if rank==0 {
        var stat : MPI_Status;
        MPI_Recv(A[0,0],1,MPI_DOUBLE, size-1:c_int, 0, CHPL_COMM_WORLD, stat);
      }
    }
  
  } // End task loop
  t.stop();

  if finalRank {
    const corner_val : real = (iterations+1)*(n+m-2);
    if abs((A[n-1,m-1] - corner_val)/corner_val) > epsilon {
      writeln("Solution does not match verification value");
      writef("Expected = %r, Actual = %r\n",corner_val,A[n-1,m-1]);
    } else {
      writeln("Solution validates");
    }

    var avgtime = t.elapsed()/iterations;
    var mflops = 1.0e-6*2*(m-1)*(n-1)/avgtime;
    writef("Rate (MFlops/s): %r  Avg time (s) : %r \n", mflops,avgtime);
  }

}


