/* Synch_p2p PRK

Notes :
 1. The PRK versions assume a column-major storage order for the
 array. In order to simply use multi-dimensional arrays, we will transpose
 the operations. So, the i, j values are all swapped.
*/

use Time;

const epsilon=1.0e-8;

config const m=64,
             n=64,
             iterations=2;


proc main() {

  /* Print useful information */
  writef("Grid size = %i, %i \n",m,n);
  writef("Number of iterations = %i \n",iterations);

  /* Define the grid */
  var Space = {0.. #n, 0.. #m};
  var A : [Space] real;
  A = 0.0;
  // Set boundary values
  [j in 0.. #n] A[j,0] = j:real;
  [i in 0.. #m] A[0,i] = i:real;


  // Define the space we're iterating over
  var IterSpace = {1.. #(n-1), 1.. #(m-1)};

  // Get ready for iterations
  var t : Timer;
  t.clear();

  for iiter in 0..iterations {

    if iiter == 1 then t.start();

    // This loop cannot be in parallel, since
    // there are data dependencies. That is the 
    // entire point of this.
    for (j,i) in IterSpace do
      A[j,i] = A[j,i-1] + A[j-1,i] - A[j-1,i-1];

    A[0,0] = -A[n-1,m-1];
  }
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


