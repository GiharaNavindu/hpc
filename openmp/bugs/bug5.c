/*
 * FILE: bug5.c
 * BUG: Deadlock caused by locks acquired in different orders
 *
 * ISSUE EXPLANATION:
 * Two different threads attempt to lock the same two locks (locka, lockb)
 * but in different orders. This creates a circular wait condition:
 * - Thread 1 holds locka, waits for lockb
 * - Thread 2 holds lockb, waits for locka
 * Both threads block forever = DEADLOCK, program hangs!
 *
 * DEADLOCK SCENARIO:
 * Time 1:
 *   Section 1: omp_set_lock(&locka)    <- Acquires LOCK_A successfully
 *   Section 2: omp_set_lock(&lockb)    <- Acquires LOCK_B successfully
 *
 * Time 2:
 *   Section 1: omp_set_lock(&lockb)    <- BLOCKS waiting for LOCK_B (Section 2 holds it)
 *   Section 2: omp_set_lock(&locka)    <- BLOCKS waiting for LOCK_A (Section 1 holds it)
 *
 * Result: CIRCULAR WAIT = DEADLOCK
 * Both threads stuck forever, program never completes!
 *
 * CURRENT CODE PROBLEM:
 * #pragma omp sections nowait
 * {
 *     #pragma omp section
 *     {
 *         omp_set_lock(&locka);        // Section 1: Get lock A
 *         // ... work with a ...
 *         omp_set_lock(&lockb);        // Section 1: Get lock B
 *         // ... work with b ...
 *         omp_unset_lock(&lockb);
 *         omp_unset_lock(&locka);
 *     }
 *
 *     #pragma omp section
 *     {
 *         omp_set_lock(&lockb);        // Section 2: Get lock B (section 1 may have A)
 *         // ... work with b ...
 *         omp_set_lock(&locka);        // Section 2: Get lock A (section 1 may have it!)
 *         // ... work with a ...
 *         omp_unset_lock(&locka);      // DEADLOCK HERE
 *         omp_unset_lock(&lockb);
 *     }
 * }
 *
 * WHY IT'S WRONG:
 * - Locks acquired in different order: Section 1 does A then B
 * - Section 2 does B then A - OPPOSITE ORDER!
 * - Creates opportunity for circular wait
 * - Circular dependencies cause deadlock
 * - Program hangs without error message
 * - Difficult to debug (no crash, just hangs)
 *
 * HOW TO FIX IT:
 * Solution 1: ALWAYS acquire locks in same order everywhere
 * #pragma omp sections nowait
 * {
 *     #pragma omp section
 *     {
 *         omp_set_lock(&locka);        // ALWAYS A first
 *         omp_set_lock(&lockb);        // THEN B
 *
 *         for (i = 0; i < N; i++)
 *             a[i] = i * DELTA;
 *         for (i = 0; i < N; i++)
 *             b[i] += a[i];
 *
 *         omp_unset_lock(&lockb);      // Release in REVERSE order
 *         omp_unset_lock(&locka);
 *     }
 *
 *     #pragma omp section
 *     {
 *         omp_set_lock(&locka);        // ALWAYS A first (same as section 1)
 *         omp_set_lock(&lockb);        // THEN B (same as section 1)
 *
 *         for (i = 0; i < N; i++)
 *             b[i] = i * PI;
 *         for (i = 0; i < N; i++)
 *             a[i] += b[i];
 *
 *         omp_unset_lock(&lockb);      // Release in REVERSE order
 *         omp_unset_lock(&locka);
 *     }
 * }
 *
 * Solution 2: Use critical section (simpler, no custom locks)
 * #pragma omp sections nowait
 * {
 *     #pragma omp section
 *     {
 *         for (i = 0; i < N; i++)
 *             a[i] = i * DELTA;
 *         #pragma omp critical(update_b)
 *         {
 *             for (i = 0; i < N; i++)
 *                 b[i] += a[i];
 *         }
 *     }
 *
 *     #pragma omp section
 *     {
 *         for (i = 0; i < N; i++)
 *             b[i] = i * PI;
 *         #pragma omp critical(update_a)
 *         {
 *             for (i = 0; i < N; i++)
 *                 a[i] += b[i];
 *         }
 *     }
 * }
 *
 * Solution 3: Use atomic operations (if possible)
 * #pragma omp sections
 * {
 *     #pragma omp section
 *     {
 *         for (i = 0; i < N; i++)
 *             a[i] = i * DELTA;
 *         for (i = 0; i < N; i++)
 *             #pragma omp atomic
 *             b[i] += a[i];
 *     }
 * }
 *
 * KEY LESSON:
 * - Lock ordering must be CONSISTENT across all code paths
 * - If code acquires A then B, ALL code must acquire A then B
 * - Release locks in REVERSE order of acquisition (LIFO)
 * - Avoid nested locks if possible - use critical sections instead
 * - Circular wait = deadlock: A waits for B while B waits for A
 * - Test for hangs when using custom locks
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define N 1000000
#define PI 3.1415926535
#define DELTA .01415926535

int main(int argc, char *argv[])
{
    int nthreads, tid, i;
    float a[N], b[N];
    omp_lock_t locka, lockb;

    /* Initialize the locks */
    omp_init_lock(&locka);
    omp_init_lock(&lockb);

/* Fork a team of threads giving them their own copies of variables */
#pragma omp parallel shared(a, b, nthreads, locka, lockb) private(tid)
    {

        /* Obtain thread number and number of threads */
        tid = omp_get_thread_num();
#pragma omp master
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }
        printf("Thread %d starting...\n", tid);
#pragma omp barrier

#pragma omp sections nowait
        {
#pragma omp section
            {
                printf("Thread %d initializing a[]\n", tid);
                omp_set_lock(&locka);
                for (i = 0; i < N; i++)
                    a[i] = i * DELTA;
                omp_set_lock(&lockb);
                printf("Thread %d adding a[] to b[]\n", tid);
                for (i = 0; i < N; i++)
                    b[i] += a[i];
                omp_unset_lock(&lockb);
                omp_unset_lock(&locka);
            }

#pragma omp section
            {
                printf("Thread %d initializing b[]\n", tid);
                omp_set_lock(&lockb);
                for (i = 0; i < N; i++)
                    b[i] = i * PI;
                omp_set_lock(&locka);
                printf("Thread %d adding b[] to a[]\n", tid);
                for (i = 0; i < N; i++)
                    a[i] += b[i];
                omp_unset_lock(&locka);
                omp_unset_lock(&lockb);
            }
        } /* end of sections */
    } /* end of parallel region */
}
