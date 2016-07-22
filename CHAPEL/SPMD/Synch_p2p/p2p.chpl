/* Synch_p2p PRK

Notes :
 1. The PRK versions assume a column-major storage order for the
 array. In order to simply use multi-dimensional arrays, we will transpose
 the operations. So, the i, j values are all swapped.
*/

use Time;
use Barrier;

const epsilon=1.0e-8;

config const m=64,
             n=64,
             iterations=2,
             ntasks = here.maxTaskPar;


proc main() {

  /* Print useful information */
  writef("Parallel Research Kernels\n");
  writef("Synch_p2p\n");
  writef("Number of tasks = %i\n", ntasks);
  writef("Grid size = %i, %i \n",m,n);
  writef("Number of iterations = %i \n",iterations);

  /* Define the grid */
  var Space = {0.. #n, 0.. #m};
  var IterSpaceI = 1..(m-1);
  var IterSpaceJ = 1..(n-1);
  var A : [Space] real;
  A = 0.0;
  // Set boundary values
  [j in 0.. #n] A[j,0] = j:real;
  [i in 0.. #m] A[0,i] = i:real;


  // Define synchronization flags
  var flags : [IterSpaceJ, 0.. #ntasks] sync bool;
  flags[..,0] = true; // Task 0 is ready to go
  
  // Get ready for iterations
  var t : Timer;
  t.clear();
  var firstiter = new Barrier(ntasks); // Timing barrier
                                       // No reuse necessary


  // Start a loop over the tasks here 
  coforall itask in 0.. #ntasks with (ref t) {

    // Define the space we're iterating over
    var segment = m/ntasks;
    var remainder = m%ntasks;
    var ThreadIterSpaceI : domain(1);
    if itask < remainder {
      var start = itask*(segment+1);
      ThreadIterSpaceI = IterSpaceI[start.. #(segment+1)];
    }
    else {
      var start = m-(ntasks-itask)*segment;
      ThreadIterSpaceI = IterSpaceI[start.. #segment];
    }

    for iiter in 0..iterations {

      if iiter == 1 {
        firstiter.barrier();
        if itask==0 then t.start();
      }

      // This loop cannot be in parallel, since
      // there are data dependencies. That is the 
      // entire point of this. I currently do this as
      // a double loop, since we need to block on each j
      for j in IterSpaceJ {
        flags[j,itask]; // Empty variable
        for i in ThreadIterSpaceI do 
          A[j,i] = A[j,i-1] + A[j-1,i] - A[j-1,i-1];
        // Let the right neighbour go
        if itask != (ntasks-1) then flags[j,itask+1]=true;
      }

      // Corner term
      if itask==(ntasks-1) {
        A[0,0] = -A[n-1,m-1];
        flags[..,0] = true; // Let the first loop go again.
      }
    }
  
  } // End task loop
  t.stop();

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


