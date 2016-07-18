/* 
 Parallel Research Kernels in Chapel
 Matrix Transpose

 Multi-locale version, using mixed Chapel+MPI

 Nikhil Padmanabhan, 2016
*/

use Time;
use MPI;
use C_MPI;
use BlockDist;

/* Define constants */
config const order=64, 
             iterations=2, 
             epsilon = 1.0e-8,
             tile=32;

// Decide whether to tile or not
const useTile=(tile>0);


/* Verify that the numLocales divides order */
assert(order%numLocales==0, 
    "Matrix dimension must divide number of processors");
const colWidth = order/numLocales;


proc main() {

  /* Define the domains and their decomposition */
  var Dom = {0.. #order, 0.. #order};
  var target : [0.. #numLocales, 0..0] locale;
  target[..,0] = Locales;
  var Space = Dom dmapped Block(Dom, targetLocales=target);
  const bytes = 2*8*order*order; /* we use real(64) */

  /* Information */
  writeln("Parallel Research Kernels");
  writeln("Matrix Transpose: B= A^T");

  writef("Matrix order = %i\n", order);
  if useTile {
    writef("Tile size=%i\n",tile);
  } else {
    writef("Untiled\n");
  }
  writef("Number of iterations = %i\n", iterations);


  /* Define arrays */
  var Ap, Bp : [Space] real;
  Bp = 0.0;
  [(i,j) in Space] Ap[i,j] = (order*j+i):real;

  // Initialize the timer; do this only on the main Locale
  var t : Timer;
  t.clear();

  // Transpose iteration loop goes here
  // For this case, we'd like to avoid introducing a barrier
  // between each of the iterations, so we run the iteration 
  // loop internal to a coforall loop 

  coforall loc in Locales 
    with (ref t) // Timer is a record, so need to use a reference intent
    do on loc {

      // Define workspace here 
      var mySpace = Space.localSubdomain();
      var low = mySpace.low(1);
      var colBlock = {0.. #colWidth, 0.. #colWidth};
      const blockSize = (colWidth*colWidth):c_int;
      var workIn, workOut : [colBlock] real;

      // Tiling
      var tileDom : domain(2, stridable=true);
      if useTile then tileDom = colBlock by tile;

      // Communication
      var send_to, recv_from : c_int;
      var send_req, recv_req : MPI_Request;
      var send_status, recv_status : MPI_Status;

      // Start iterating
      for iiter in 0..iterations {

        // Start the timer, after a barrier
        if iiter==1 {
          MPI_Barrier(CHPL_COMM_WORLD);
          if here.id==0 then on t do t.start();
        }

        local {

          // Do the local piece
          if useTile {
            forall (i0,j0) in tileDom {
              var dom1 = colBlock[i0.. #tile, j0.. #tile];
              for (i,j) in dom1 {
                Bp.localAccess[j+low,i+low] += Ap.localAccess[i+low,j+low];
                Ap.localAccess[i+low,j+low] += 1.0;
              }
            }
          } else {
            forall (i,j) in colBlock {
              Bp.localAccess[j+low,i+low] += Ap.localAccess[i+low,j+low];
              Ap.localAccess[i+low,j+low] += 1.0;
            }
          }


          // Do the non-local pieces
          for phase in 1..(numLocales-1) {
            recv_from = ((here.id + phase)%numLocales) : c_int;
            send_to = ((here.id - phase + numLocales)%numLocales) : c_int;

            // Post a receive
            MPI_Irecv(workIn[0,0], blockSize, MPI_DOUBLE,
                recv_from, phase:c_int, CHPL_COMM_WORLD, recv_req);


            // Copy to work
            var istart = send_to*colWidth;
            if useTile {
              forall (i0,j0) in tileDom {
                var dom1 = colBlock[i0.. #tile, j0.. #tile];
                for (i,j) in dom1 {
                  var ip = i + istart;
                  var jp = j + low;
                  workOut[i,j] = Ap.localAccess[jp,ip];
                  Ap.localAccess[jp,ip] += 1.0;
                }
              }
            } else {
              forall (i,j) in colBlock {
                var ip = i + istart;
                var jp = j + low;
                workOut[i,j] = Ap.localAccess[jp,ip];
                Ap.localAccess[jp,ip] += 1.0;
              }
            }

            // Post a send
            MPI_Isend(workOut[0,0], blockSize, MPI_DOUBLE,
                send_to, phase:c_int, CHPL_COMM_WORLD, send_req);

            // Wait until receive is complete and then copy data back
            MPI_Wait(recv_req, recv_status);
            istart = recv_from*colWidth;
            [(i,j) in colBlock] 
              Bp.localAccess[i+low, j+istart] += workIn[i,j];

            // Wait until send is complete
            MPI_Wait(send_req, send_status);


          }
          // end local block
        }


      }



    }

  


  // Timer stop
  t.stop();
  
  // Note that this is a distributed, parallel operation
  const addit = ((iterations*(iterations+1)):real)*0.5;
  [(i,j) in Space] Bp[i,j] = 
    abs(Bp[i,j] - (order*i+j)*(iterations+1)-addit);
  var abserr = + reduce Bp;

  // Check to see if solution is valid
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

