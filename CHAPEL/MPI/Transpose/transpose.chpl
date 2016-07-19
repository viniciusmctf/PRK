/* 
 Parallel Research Kernels in Chapel
 Matrix Transpose

 single-locale Chapel + MPI

 Nikhil Padmanabhan, 2016
*/

use Time;
use MPI;
use C_MPI;

/* Define constants */
config const order=64, 
             iterations=2, 
             epsilon = 1.0e-8,
             tile=32;

// Decide whether to tile or not
const useTile=(tile>0);



proc main() {

  /* Define some global level variables */
  var rank = commRank(CHPL_COMM_WORLD),
      size = commSize(CHPL_COMM_WORLD);
  /* Verify that the size divides order */
  assert(order%size==0, 
      "Matrix dimension must divide number of processors");
  const colWidth = order/size;


  /* Define the domains and their decomposition */
  var low = rank*colWidth;
  var Space = {low.. #colWidth, 0.. #order};
  const bytes = 2*8*order*order; /* we use real(64) */

  /* Information */
  if rank==0 {
    writeln("Parallel Research Kernels");
    writeln("Matrix Transpose: B= A^T");

    writef("Matrix order = %i\n", order);
    if useTile {
      writef("Tile size=%i\n",tile);
    } else {
      writef("Untiled\n");
    }
    writef("Number of iterations = %i\n", iterations);
  }


  /* Define arrays */
  var Ap, Bp : [Space] real;
  Bp = 0.0;
  [(i,j) in Space] Ap[i,j] = (order*j+i):real;

  // Initialize the timer; do this only on the main Locale
  var t : Timer;
  t.clear();

  // Define workspace here 
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
      t.start();
    }

    // Do the local piece
    if useTile {
      forall (i0,j0) in tileDom {
        var dom1 = colBlock[i0.. #tile, j0.. #tile];
        for (i,j) in dom1 {
          Bp[j+low,i+low] += Ap[i+low,j+low];
          Ap[i+low,j+low] += 1.0;
        }
      }
    } else {
      forall (i,j) in colBlock {
        Bp[j+low,i+low] += Ap[i+low,j+low];
        Ap[i+low,j+low] += 1.0;
      }
    }


    // Do the non-local pieces
    for phase in 1..(size-1) {
      recv_from = ((rank + phase)%size) : c_int;
      send_to = ((rank - phase + size)%size) : c_int;

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
            workOut[i,j] = Ap[jp,ip];
            Ap[jp,ip] += 1.0;
          }
        }
      } else {
        forall (i,j) in colBlock {
          var ip = i + istart;
          var jp = j + low;
          workOut[i,j] = Ap[jp,ip];
          Ap[jp,ip] += 1.0;
        }
      }

      // Post a send
      MPI_Isend(workOut[0,0], blockSize, MPI_DOUBLE,
          send_to, phase:c_int, CHPL_COMM_WORLD, send_req);

      // Wait until receive is complete and then copy data back
      MPI_Wait(recv_req, recv_status);
      istart = recv_from*colWidth;
      [(i,j) in colBlock] 
        Bp[i+low, j+istart] += workIn[i,j];

      // Wait until send is complete
      MPI_Wait(send_req, send_status);


    }


  }



  // Timer stop
  t.stop();
  var mytime = t.elapsed();
  var maxtime : real;
  MPI_Reduce(mytime, maxtime, 1, MPI_DOUBLE, MPI_MAX, 0, CHPL_COMM_WORLD);
  
  // Note that this is a distributed, parallel operation
  const addit = ((iterations*(iterations+1)):real)*0.5;
  [(i,j) in Space] Bp[i,j] = 
    abs(Bp[i,j] - (order*i+j)*(iterations+1)-addit);
  var abserr1 = + reduce Bp;
  var abserr : real;
  MPI_Reduce(abserr1, abserr, 1, MPI_DOUBLE, MPI_SUM, 0, CHPL_COMM_WORLD);

  if rank==0 {
    // Check to see if solution is valid
    if abserr < epsilon {
      writeln("Solution validates");
    } else {
      writef("Error %r exceeds threshold %r \n",abserr, epsilon);
    }

    // Output timing information
    var avgtime = maxtime/iterations;
    writef("Rate (MB/s): %r  Avg time (s): %r\n",
        1.0e-6*bytes/avgtime, avgtime);
  }


  // All done

}

