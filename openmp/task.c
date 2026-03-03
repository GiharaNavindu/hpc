#include <omp.h>
#include <stdio.h>

int main() {
    int x = 10;

    // 1. THE SETUP: We must first hire a team of workers. Tasks need threads to run them!
    #pragma omp parallel shared(x)
    {
        // 2. THE GENERATOR: We use 'single' so that ONLY ONE thread writes the "Post-it notes".
        // If we didn't use 'single', every thread in the team would generate a duplicate set of tasks.
        #pragma omp single 
        {
            printf("Thread %d is the Manager generating tasks...\n", omp_get_thread_num());

            // ---------------------------------------------------------
            // CONCEPT A: The Basic Task (Deferred Execution)
            // ---------------------------------------------------------
            // Theory: The encountering thread creates an explicit task. 
            // It can run it immediately, OR stick it on the bulletin board for any other thread to grab.
            #pragma omp task firstprivate(x)
            {
                printf("Basic Task executed by thread %d. x = %d\n", omp_get_thread_num(), x);
            }

            // ---------------------------------------------------------
            // CONCEPT B: The 'if' Clause (Conditional Execution)
            // ---------------------------------------------------------
            // Theory: If the expression evaluates to false (0), the task CANNOT be deferred. 
            // The thread that creates it MUST stop what it's doing and execute it immediately.
            #pragma omp task if(0)
            {
                printf("Immediate Task (if(0)) executed strictly by thread %d\n", omp_get_thread_num());
            }

            // ---------------------------------------------------------
            // CONCEPT C: The 'final' and 'mergeable' Clauses
            // ---------------------------------------------------------
            // Theory: final(1) forces all child tasks created inside this block to be executed 
            // immediately (no more deferring). mergeable allows the compiler to reuse the data environment to save memory.
            #pragma omp task final(1) mergeable
            {
                printf("Final Task executed by thread %d\n", omp_get_thread_num());
                
                // Because of final(1) above, this child task ignores the bulletin board 
                // and is executed immediately by whoever is running the parent task.
                #pragma omp task
                {
                    printf("Child of Final Task executed immediately by thread %d\n", omp_get_thread_num());
                }
            }

            // ---------------------------------------------------------
            // CONCEPT D: The 'untied' Clause and 'taskyield'
            // ---------------------------------------------------------
            // Theory: Normally, a task is "tied". If Thread 2 starts it and pauses, Thread 2 MUST finish it.
            // 'untied' removes this rule. If it pauses, ANY thread can pick it back up.
            #pragma omp task untied
            {
                printf("Untied Task started by thread %d\n", omp_get_thread_num());
                
                // taskyield explicitly tells the system: "I am pausing this task here. 
                // Put it back on the board so another thread can take over if needed."
                #pragma omp taskyield 
                
                printf("Untied Task finished by thread %d\n", omp_get_thread_num());
            }

            // ---------------------------------------------------------
            // CONCEPT E: Synchronization (The Stop Sign)
            // ---------------------------------------------------------
            // Theory: The thread that generated all these tasks will halt right here.
            // It will not move to the next line of code until every single child task it created is 100% finished.
            #pragma omp taskwait
            
            printf("Manager Thread %d confirms all generated tasks are complete!\n", omp_get_thread_num());

        } // End of 'single' block. (Implied barrier here for the rest of the team)
        
    } // End of parallel region. Team is destroyed.

    return 0;
}