/* 
 Parallel Research Kernels in Chapel
 Matrix Transpose

 Nikhil Padmanabhan, 2016
*/

/* Define constants */
config const order=64, 
             iterations=2, 
             epsilon = 1.0e-8;

use Time;


proc main() {
  /* Define the matrices */
  var Dom = {0.. #order, 0.. #order};
  var Ap : [Dom] real, 
      Bp : [Dom] real;

  const bytes = 2*8*order*order; /* we use real(64) */

  /* Information */
  writeln("Parallel Research Kernels");
  writeln("Matrix Transpose: B= A^T");

  writef("Matrix order = %i\n", order);
  writef("Untiled\n");
  writef("Number of iterations = %i\n", iterations);

  // Initialize A & B
  Bp = 0.0;
  [(i,j) in Dom] Ap[i,j] = (order*j+i):real;

  // Initialize the timer
  var t : Timer;
  t.clear();

  for iiter in 0..iterations {

    // Warmup with iteration 1
    if (iiter==1) {
      t.start();
    }

    forall (i,j) in Dom {
      Bp[j,i] += Ap[i,j];
      Ap[i,j] += 1.0;
    }

  }

  // Timer stop
  t.stop();

  var abserr = 0.0;
  var addit = ((iterations*(iterations+1)):real)*0.5;
  for (i,j) in Dom {
    abserr += abs(Bp[i,j] - (order*i+j)*(iterations+1)-addit);
  }

  if abserr < epsilon {
    writeln("Solution validates");
  } else {
    writef("Error %r exceeds threshold %r \n",abserr, epsilon);
  }




  // Output timing information
  var avgtime = t.elapsed()/iterations;
  writef("Rate (MB/s): %r  Avg time (s): %r\n",
      1.0e-6*bytes/avgtime, avgtime);


  // All done
}

