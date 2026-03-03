#include <omp.h>    // OpenMP API (pragmas + runtime helpers)
#include <stdio.h>  // printf
#include <stdlib.h> // general utilities (not strictly needed in this file)

int main(int argc, char *argv[]) // program entry point
{
  int i, n;                  // i = loop index, n = number of elements
  float a[100], b[100], sum; // two input arrays and one scalar accumulator

  /* Some initializations */
  n = 100;                  // work size
  for (i = 0; i < n; i++)   // initialize arrays in serial
    a[i] = b[i] = i * 1.0f; // a[i]=i, b[i]=i (as float)

  sum = 0.0f; // initial value for accumulation

// Create a parallel region + split this for-loop across threads.
// reduction(+:sum) means:
// 1) each thread gets a private 'sum' initialized to 0
// 2) each thread computes partial sum for its chunk
// 3) at end, OpenMP combines all partial sums with '+' into global 'sum'
#pragma omp parallel for reduction(+ : sum)
  for (i = 0; i < n; i++)      // iterations distributed among threads
    sum = sum + (a[i] * b[i]); // local partial accumulation per thread

  printf("   Sum = %f\n", sum); // final reduced result (single value)

  return 0; // good practice for int main
}