/*
 * FILE: bug4.c
 * BUG: Stack overflow from large private array allocation
 *
 * ISSUE EXPLANATION:
 * A very large 2D array (1048 x 1048 doubles) is declared as private.
 * In OpenMP, private variables are allocated on each thread's stack.
 * With 1048² x 8 bytes ≈ 8.8 MB per thread, this exhausts stack memory
 * and causes a crash (segmentation fault / stack overflow).
 *
 * MEMORY ANALYSIS:
 * Array: double a[1048][1048]
 * Element size: 8 bytes (double precision floating point)
 * Total per thread: 1048 × 1048 × 8 = 8,781,824 bytes ≈ 8.8 MB
 *
 * With typical thread count (4-8):
 * 4 threads:  8.8 MB × 4 = 35.2 MB needed
 * 8 threads:  8.8 MB × 8 = 70.4 MB needed
 *
 * Typical stack size per thread: 1-8 MB (varies by system)
 * Result: STACK OVERFLOW - program crashes!
 *
 * CURRENT CODE PROBLEM:
 * #pragma omp parallel shared(nthreads) private(i, j, tid, a)
 *                                                       ^
 *                                    Large array allocated on stack!
 * {
 *     // Each thread tries to allocate 8.8 MB on its 1-8 MB stack
 *     for (i = 0; i < N; i++)
 *         for (j = 0; j < N; j++)
 *             a[i][j] = tid + i + j;  // CRASH: Stack overflow!
 * }
 *
 * WHY IT'S WRONG:
 * - Private variables allocated on thread stack
 * - Stack size is severely limited (1-8 MB per thread)
 * - 8.8 MB array exceeds available stack space
 * - Program crashes with segmentation fault
 * - No compilation error - only runtime failure
 * - Very inefficient memory usage (duplicating 8.8 MB per thread)
 *
 * HOW TO FIX IT:
 * Solution 1: Move array to global scope (BEST for this case)
 * // Move outside main() to global scope
 * #define N 1048
 * double a[N][N];  // Global - allocated on heap/data segment
 *
 * int main(int argc, char *argv[])
 * {
 *     int nthreads, tid, i, j;
 *
 *     #pragma omp parallel shared(nthreads, a) private(i, j, tid)
 *     {  // Now 'a' is single shared copy, all threads use same data
 *         tid = omp_get_thread_num();
 *         if (tid == 0) {
 *             nthreads = omp_get_num_threads();
 *             printf("Number of threads = %d\n", nthreads);
 *         }
 *         printf("Thread %d starting...\n", tid);
 *
 *         #pragma omp for collapse(2)  // Parallelize both loops
 *         for (i = 0; i < N; i++)
 *             for (j = 0; j < N; j++)
 *                 a[i][j] = tid + i + j;
 *
 *         printf("Thread %d done. Last element= %f\n", tid, a[N-1][N-1]);
 *     }
 *     return 0;
 * }
 *
 * Solution 2: Make array static (allocated once, at module scope)
 * int main(int argc, char *argv[])
 * {
 *     static double a[1048][1048];  // Static - allocated on heap
 *
 *     #pragma omp parallel shared(nthreads, a) private(i, j, tid)
 *     {
 *         // Array is shared, no stack overflow
 *         // ...
 *     }
 * }
 *
 * Solution 3: Use shared smaller working arrays
 * #pragma omp parallel shared(nthreads) private(i, j, tid, temp)
 * {
 *     double temp[100][100];  // Small working array on stack - OK
 *     // Use temp for computation
 * }
 *
 * Solution 4: Increase stack size (system-dependent, not recommended)
 * // Linux: ulimit -s unlimited (before running)
 * // Or export OMP_STACKSIZE=100M
 * // Not portable and affects system resources
 *
 * KEY LESSON:
 * - Stack space is limited (1-8 MB per thread)
 * - private variables allocated on thread stack
 * - Large arrays should be global/static or heap-allocated
 * - Global arrays allocate once, all threads share
 * - Use shared for large data structures
 * - Use private only for small temporary variables
 * - Check memory requirements before declaring private large arrays
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define N 1048

int main(int argc, char *argv[])
{
    int nthreads, tid, i, j;
    double a[N][N];

/* Fork a team of threads with explicit variable scoping */
#pragma omp parallel shared(nthreads) private(i, j, tid, a)
    {

        /* Obtain/print thread info */
        tid = omp_get_thread_num();
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }
        printf("Thread %d starting...\n", tid);

        /* Each thread works on its own private copy of the array */
        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++)
                a[i][j] = tid + i + j;

        /* For confirmation */
        printf("Thread %d done. Last element= %f\n", tid, a[N - 1][N - 1]);

    } /* All threads join master thread and disband */
}
