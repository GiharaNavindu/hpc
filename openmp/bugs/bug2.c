/*\n * FILE: bug2.c\n * BUG: Race condition with unprotected shared variable 'total'\n *\n * ISSUE EXPLANATION:\n * The variable 'total' is NOT declared in any scoping clause, which means it\n * implicitly becomes SHARED (default in OpenMP). Multiple threads execute\n * the same for loop and update 'total' without synchronization, causing\n * a RACE CONDITION. The final result is unpredictable and incorrect.\n *\n * RACE CONDITION EXAMPLE:\n * Thread 1: Read total=0.0, add value, write total=100.0\n * Thread 2: Read total=0.0, add value, write total=200.0  <- Lost Thread 1's work!\n * Thread 3: Read total=200.0, add value, write total=300.0 <- Lost Thread 2's work!\n * \n * Expected: total = 100.0 + 200.0 + 300.0\n * Actual: total = 300.0 (depends on timing)\n *\n * CURRENT CODE PROBLEM:\n * #pragma omp parallel\n * {\n *     #pragma omp for schedule(dynamic, 10)\n *     for (i = 0; i < 1000000; i++)\n *         total = total + i * 1.0;  // total is SHARED by default\n *                                  // All threads access same variable\n *                                  // No synchronization = RACE CONDITION\n * }\n *\n * WHY IT'S WRONG:\n * - Variable 'total' has no scoping clause\n * - Default is shared (all threads access same memory)\n * - Multiple threads read-modify-write to same variable\n * - Operations not atomic - can interleave incorrectly\n * - Result depends on thread scheduling (non-deterministic)\n * - Each run may produce different (wrong) answer\n *\n * HOW TO FIX IT:\n * Solution 1: Use reduction (BEST for this pattern)\n * #pragma omp parallel private(tid)\n * {\n *     tid = omp_get_thread_num();\n *     if (tid == 0) {\n *         nthreads = omp_get_num_threads();\n *         printf(\"Number of threads = %d\\\\n\", nthreads);\n *     }\n *     printf(\"Thread %d is starting...\\\\n\", tid);\n * \n * #pragma omp barrier\n * \n *     #pragma omp for reduction(+:total) schedule(dynamic, 10)\n *     for (i = 0; i < 1000000; i++)\n *         total = total + i * 1.0;  // Each thread has own copy\n *                                    // Results combined automatically\n * \n *     printf(\"Thread %d is done! Total= %e\\\\n\", tid, total);\n * }\n *\n * Solution 2: Make total private and combine results\n * #pragma omp parallel private(tid, total)  // Make total PRIVATE\n * {\n *     tid = omp_get_thread_num();\n *     if (tid == 0) {\n *         nthreads = omp_get_num_threads();\n *         printf(\"Number of threads = %d\\\\n\", nthreads);\n *     }\n *     printf(\"Thread %d is starting...\\\\n\", tid);\n * \n *     total = 0.0;  // Each thread initializes own copy\n * \n * #pragma omp barrier\n * \n *     #pragma omp for schedule(dynamic, 10)\n *     for (i = 0; i < 1000000; i++)\n *         total = total + i * 1.0;  // Each thread updates own copy\n * \n *     printf(\"Thread %d is done! Total= %e\\\\n\", tid, total);\n * }\n *\n * Solution 3: Use atomic directive\n * #pragma omp for schedule(dynamic, 10)\n * for (i = 0; i < 1000000; i++) {\n *     double temp = i * 1.0;\n *     #pragma omp atomic\n *     total = total + temp;  // Atomic ensures correct combination\n * }\n *\n * KEY LESSON:\n * - Unscoped variables are shared by default (not private)\n * - Multiple threads accessing same variable = RACE CONDITION\n * - Use reduction() for combining numeric results\n * - Use atomic for single variable updates\n * - Use critical sections for complex operations\n * - Explicit scoping prevents accidental race conditions\n */\n\n #include<omp.h>\n #include<stdio.h>\n #include<stdlib.h>\n\nint main(int argc, char *argv[])\n
{
    \n int nthreads, i, tid;
    \n float total;
    \n\n /*** Spawn parallel region ***/
#pragma omp parallel
    {
        /* Obtain thread number */
        tid = omp_get_thread_num();
        /* Only master thread does this */
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }
        printf("Thread %d is starting...\n", tid);

#pragma omp barrier

        /* do some work */
        total = 0.0;
#pragma omp for schedule(dynamic, 10)
        for (i = 0; i < 1000000; i++)
            total = total + i * 1.0;

        printf("Thread %d is done! Total= %e\n", tid, total);

    } /*** End of parallel region ***/
}
