/*
 * FILE: bug6.c
 * BUG: Uninitialized reduction variable due to variable scope shadowing
 *
 * ISSUE EXPLANATION:
 * The function dotprod() declares a LOCAL variable 'sum', which shadows
 * (hides) the 'sum' variable declared in main(). The reduction operation
 * applies to the function's local uninitialized 'sum', not main's initialized 'sum'.
 * This causes:
 * - Reduction operates on uninitialized garbage data
 * - Result is meaningless (garbage + computations = garbage)
 * - Main's sum is never updated
 * - Program produces incorrect output silently
 *
 * VARIABLE SCOPE PROBLEM:
 * main() scope:
 *   float sum = 0.0;        <- Initialized to 0 and shared
 *   #pragma omp parallel shared(sum)  <- Trying to share this
 *       dotprod();           <- But function has different 'sum'!
 *
 * dotprod() scope:
 *   float sum;              <- LOCAL variable (shadows main's sum!)
 *                           <- NOT initialized -> contains garbage
 *   #pragma omp for reduction(+ : sum)  <- Operates on THIS local sum
 *                           <- Garbage + garbage = garbage result
 *
 * CURRENT CODE PROBLEM:
 * float dotprod()
 * {
 *     int i, tid;
 *     float sum;        // <-- LOCAL UNINITIALIZED VARIABLE
 *                       // <-- DIFFERENT from main's sum!
 *     tid = omp_get_thread_num();
 * #pragma omp for reduction(+ : sum)
 *     for (i = 0; i < VECLEN; i++)
 *     {
 *         sum = sum + (a[i] * b[i]);  // sum starts with GARBAGE!
 *     }
 * }
 *
 * int main(int argc, char *argv[])
 * {
 *     float sum;           // <-- DIFFERENT variable
 *     sum = 0.0;           // <-- This initialization is IGNORED by function
 *     #pragma omp parallel shared(sum)  // <-- Sharing this sum...
 *         dotprod();       // <-- ...but function uses different sum!
 *     printf("Sum = %f\\n", sum);  // <-- sum was never updated!
 * }
 *
 * TWO DIFFERENT VARIABLES:
 * Variable 1: main's sum (initialized 0.0, shared, never modified)
 * Variable 2: dotprod's sum (uninitialized, local, used in reduction)
 *
 * WHY IT'S WRONG:
 * - Local variable declaration shadows (hides) the outer variable
 * - Uninitialized local sum contains random memory values (garbage)
 * - Reduction adds garbage + computed values = garbage result
 * - Main's sum is never updated by the function
 * - Output shows uninitialized value (garbage) from main's sum
 * - No compilation error - just silent incorrect results
 *
 * HOW TO FIX IT:
 * Solution 1: Use global variables (recommended for this pattern)
 * float a[VECLEN], b[VECLEN], sum;  // sum is GLOBAL
 *
 * float dotprod()
 * {
 *     int i, tid;
 *     tid = omp_get_thread_num();
 * #pragma omp for reduction(+ : sum)  // Now uses GLOBAL sum
 *     for (i = 0; i < VECLEN; i++)
 *     {
 *         sum = sum + (a[i] * b[i]);
 *     }
 * }
 *
 * int main(int argc, char *argv[])
 * {
 *     int i;
 *     for (i = 0; i < VECLEN; i++)
 *         a[i] = b[i] = 1.0 * i;
 *     sum = 0.0;  // Initialize GLOBAL sum
 *
 * #pragma omp parallel
 *         dotprod();  // Function uses global sum
 *
 *     printf("Sum = %f\\n", sum);  // Correct result from global sum
 * }
 *
 * Solution 2: Pass sum by reference
 * float dotprod(float *sum_ptr)
 * {
 *     int i, tid;
 *     tid = omp_get_thread_num();
 * #pragma omp for reduction(+ : (*sum_ptr))
 *     for (i = 0; i < VECLEN; i++)
 *     {
 *         *sum_ptr = *sum_ptr + (a[i] * b[i]);
 *     }
 *     return *sum_ptr;
 * }
 *
 * int main(int argc, char *argv[])
 * {
 *     int i;
 *     float sum;  // Local in main
 *     for (i = 0; i < VECLEN; i++)
 *         a[i] = b[i] = 1.0 * i;
 *     sum = 0.0;
 *
 * #pragma omp parallel
 *         dotprod(&sum);  // Pass reference
 *
 *     printf("Sum = %f\\n", sum);  // Correct result
 * }
 *
 * Solution 3: Initialize local sum and return it
 * float dotprod()  // If must use local sum
 * {
 *     int i, tid;
 *     float sum = 0.0;   // INITIALIZE the local sum
 *     tid = omp_get_thread_num();
 * #pragma omp for reduction(+ : sum)
 *     for (i = 0; i < VECLEN; i++)
 *     {
 *         sum = sum + (a[i] * b[i]);
 *     }
 *     return sum;  // Return computed sum
 * }
 *
 * int main(int argc, char *argv[])
 * {
 *     int i;
 *     float total = 0.0;  // Main accumulator
 *     for (i = 0; i < VECLEN; i++)
 *         a[i] = b[i] = 1.0 * i;
 *
 * #pragma omp parallel
 *     {
 *         float partial = dotprod();
 * #pragma omp critical
 *         total += partial;  // Combine results
 *     }
 *
 *     printf("Sum = %f\\n", total);
 * }
 *
 * KEY LESSON:
 * - Variable scope matters: local and global variables hide each other
 * - Never use uninitialized variables in reduction operations
 * - Prefer global/static variables for cross-function reductions
 * - Or pass variables by reference/pointer
 * - Always initialize reduction variables before use
 * - Test for correct numerical results, not just compilation
 * - Variable shadowing leads to silent bugs that are hard to find
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define VECLEN 100

float a[VECLEN], b[VECLEN];

float dotprod()
{
    int i, tid;
    float sum;

    tid = omp_get_thread_num();
#pragma omp for reduction(+ : sum)
    for (i = 0; i < VECLEN; i++)
    {
        sum = sum + (a[i] * b[i]);
        printf("  tid= %d i=%d\n", tid, i);
    }
}

int main(int argc, char *argv[])
{
    int i;
    float sum;

    for (i = 0; i < VECLEN; i++)
        a[i] = b[i] = 1.0 * i;
    sum = 0.0;

#pragma omp parallel shared(sum)
    dotprod();

    printf("Sum = %f\n", sum);
}
