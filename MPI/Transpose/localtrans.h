/*
Copyright (c) 2013, Intel Corporation

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions 
are met:

* Redistributions of source code must retain the above copyright 
      notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above 
      copyright notice, this list of conditions and the following 
      disclaimer in the documentation and/or other materials provided 
      with the distribution.
* Neither the name of Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products 
      derived from this software without specific prior written 
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/

/*******************************************************************

HISTORY: Written by Tim Mattson, April 1999.  
         Updated by Rob Van der Wijngaart, December 2005.
         Updated by Rob Van der Wijngaart, October 2006.
         Updated by Jeff Hammond, September 2014.
  
*******************************************************************/

/*******************************************************************
** Copy the transpose of an array slice inside matrix A into 
** the an array slice inside matrix B.
**
**  Parameters:
**
**       A, B                base address of the slices
**       sub_rows, sub_cols  Numb of rows/cols in the slice.
** 
*******************************************************************/

#ifndef LOCAL_TRANSPOSE_H
#define LOCAL_TRANSPOSE_H

/* This header is included prior to this file in the parent.
 * #include <par-res-kern_general.h>
 */

static void transpose(
  const double * RESTRICT A,    /* input matrix                   */
  double * RESTRICT B,          /* output matrix                  */
  int tile_size,                /* local tile size                */
  int sub_rows, int sub_cols)   /* size of slice to  transpose    */
{
  int    i, j, it, jt;

  /*  Transpose the  matrix.  */

  /* tile only if the tile size is smaller than the matrix block  */
  if (tile_size < sub_cols) {
    for (i=0; i<sub_cols; i+=tile_size) { 
      for (j=0; j<sub_rows; j+=tile_size) { 
        for (it=i; it<MIN(sub_cols,i+tile_size); it++){ 
          for (jt=j; jt<MIN(sub_rows,j+tile_size);jt++){ 
            B[it+sub_cols*jt] = A[jt+sub_rows*it]; 
          } 
        } 
      } 
    } 
  }
  else {
    for (i=0;i<sub_cols; i++) {
      for (j=0;j<sub_rows;j++) {
        B[i+sub_cols*j] = A[j+i*sub_rows];
      }
    }	
  }
}
#endif LOCAL_TRANSPOSE_H
