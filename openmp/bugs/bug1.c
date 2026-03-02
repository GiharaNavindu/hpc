/*
 * FILE: bug1.c
 * BUG: Incorrect syntax with extra braces in parallel for directive
 *
 * ISSUE EXPLANATION:
 * The #pragma omp parallel for directive is followed by explicit braces {},
 * which is syntactically incorrect. The parallel for directive should be
 * followed directly by the for loop without extra braces in between.
 *
 * CURRENT CODE PROBLEM:
 * #pragma omp parallel for shared(a, b, c, chunk) ...
 * {
 *     tid = omp_get_thread_num();
 *     for (i = 0; i < N; i++) {...}  // Loop inside braces
 * }
 *
 * WHY IT'S WRONG:
 * - The braces create an extra scope that interferes with the pragma
 * - The for loop is not directly under the pragma
 * - Compiler may misinterpret the directive scope
 * - Behavior is undefined or produces compilation warnings
 *
 * HOW TO FIX IT:
 * Option 1: Use combined parallel for without extra braces
 * #pragma omp parallel for shared(...) private(...) schedule(static, chunk)
 * for (i = 0; i < N; i++) {
 *     // loop body
 * }
 *
 * Option 2: Separate parallel and for with proper nesting
 * #pragma omp parallel shared(...) private(...)
 * {
 *     #pragma omp for schedule(static, chunk)
 *     for (i = 0; i < N; i++) {
 *         // loop body
 *     }
 * }
 *
 * KEY LESSON:
 * - #pragma omp parallel for must be followed directly by a for loop
 * - Do not add braces between the pragma and the for statement
 * - If using separate pragmas, nest them properly with braces
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#define N 50
#define CHUNKSIZE 5

int main(int argc, char *argv[])
{
    int i, chunk, tid;
    float a[N], b[N], c[N];

    /* Some initializations */
    for (i = 0; i < N; i++)
        a[i] = b[i] = i * 1.0;
    chunk = CHUNKSIZE;

#pragma omp parallel for shared(a, b, c, chunk) \
    private(i, tid)                             \
    schedule(static, chunk)
    {
        tid = omp_get_thread_num();
        for (i = 0; i < N; i++)
        {
            c[i] = a[i] + b[i];
            printf("tid= %d i= %d c[i]= %f\n", tid, i, c[i]);
        }
    } /* end of parallel for construct */
}
