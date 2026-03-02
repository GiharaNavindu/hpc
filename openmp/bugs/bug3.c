/*
 * FILE: bug3.c
 * BUG: Incorrect variable scoping - array c is private instead of shared
 *
 * ISSUE EXPLANATION:
 * The array 'c' is declared in the private() clause, which means each thread
 * gets its own copy. When using sections, this causes different threads to
 * compute results into different copies of the array, losing data and causing
 * unpredictable behavior.
 *
 * CURRENT CODE PROBLEM:
 * #pragma omp parallel private(c, i, tid, section)  // <-- c is PRIVATE!
 * {
 *     #pragma omp sections
 *     {
 *         #pragma omp section
 *         {
 *             for (i = 0; i < N; i++)
 *                 c[i] = a[i] * b[i];  // Thread 1's copy of c
 *         }
 *         #pragma omp section
 *         {
 *             for (i = 0; i < N; i++)
 *                 c[i] = a[i] + b[i];  // Different thread's copy of c
 *         }
 *     }
 * }
 *
 * WHY IT'S WRONG:
 * - Each thread has its own private copy of array c
 * - Section 1 computes into its private c (other threads don't see it)
 * - Section 2 computes into its private c (different from section 1)
 * - Data computed in one section is invisible to other sections
 * - Results are unpredictable and thread-dependent
 * - print_results() may print garbage or incomplete data
 *
 * HOW TO FIX IT:
 * Solution 1: Make c SHARED so all threads see same array
 * #pragma omp parallel private(i, tid, section) shared(c, a, b, nthreads)
 * {
 *     tid = omp_get_thread_num();
 *     if (tid == 0) {
 *         nthreads = omp_get_num_threads();
 *         printf("Number of threads = %d\\n", nthreads);
 *     }
 *
 *     #pragma omp barrier
 *
 *     #pragma omp sections nowait
 *     {
 *         #pragma omp section
 *         {
 *             section = 1;
 *             for (i = 0; i < N; i++)
 *                 c[i] = a[i] * b[i];  // All threads modify same c
 *             print_results(c, tid, section);
 *         }
 *
 *         #pragma omp section
 *         {
 *             section = 2;
 *             for (i = 0; i < N; i++)
 *                 c[i] = a[i] + b[i];  // All threads modify same c
 *             print_results(c, tid, section);
 *         }
 *     }
 * }
 *
 * Solution 2: Use different arrays for each section
 * #pragma omp parallel private(i, tid) shared(c, d, a, b, nthreads)
 * {
 *     #pragma omp sections
 *     {
 *         #pragma omp section
 *         {
 *             for (i = 0; i < N; i++)
 *                 c[i] = a[i] * b[i];  // Multiplication -> c
 *         }
 *
 *         #pragma omp section
 *         {
 *             for (i = 0; i < N; i++)
 *                 d[i] = a[i] + b[i];  // Addition -> d
 *         }
 *     }
 * }
 *
 * KEY LESSON:
 * - private: Each thread gets its own copy (data not shared between threads)
 * - shared: One copy visible to all threads
 * - Use shared for data that must be combined or visible to all threads
 * - Use private only for thread-local temporary variables
 * - Sections should operate on shared data for results to be visible
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define N 50

int main(int argc, char *argv[])
{
    int i, nthreads, tid, section;
    float a[N], b[N], c[N];
    void print_results(float array[N], int tid, int section);

    /* Some initializations */
    for (i = 0; i < N; i++)
        a[i] = b[i] = i * 1.0;

#pragma omp parallel private(c, i, tid, section)
    {
        tid = omp_get_thread_num();
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }

/*** Use barriers for clean output ***/
#pragma omp barrier
        printf("Thread %d starting...\n", tid);
#pragma omp barrier

#pragma omp sections nowait
        {
#pragma omp section
            {
                section = 1;
                for (i = 0; i < N; i++)
                    c[i] = a[i] * b[i];
                print_results(c, tid, section);
            }

#pragma omp section
            {
                section = 2;
                for (i = 0; i < N; i++)
                    c[i] = a[i] + b[i];
                print_results(c, tid, section);
            }

        } /* end of sections */

/*** Use barrier for clean output ***/
#pragma omp barrier
        printf("Thread %d exiting...\n", tid);

    } /* end of parallel section */
}

void print_results(float array[N], int tid, int section)
{
    int i, j;

    j = 1;
/*** use critical for clean output ***/
#pragma omp critical
    {
        printf("\nThread %d did section %d. The results are:\n", tid, section);
        for (i = 0; i < N; i++)
        {
            printf("%e  ", array[i]);
            j++;
            if (j == 6)
            {
                printf("\n");
                j = 1;
            }
        }
        printf("\n");
    } /*** end of critical ***/

#pragma omp barrier
    printf("Thread %d done and synchronized.\n", tid);
}
