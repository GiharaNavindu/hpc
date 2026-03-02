# OpenMP Bug Fix Guide

This guide explains the bugs found in the `bugs/` directory and provides solutions to fix them. Each bug file demonstrates a common OpenMP programming mistake.

---

## Table of Contents

1. [Bug 1: Missing Schedule Specification](#bug-1-missing-schedule-specification)
2. [Bug 2: Incorrect Parallel For Syntax](#bug-2-incorrect-parallel-for-syntax)
3. [Bug 3: Incorrect Variable Scoping in Sections](#bug-3-incorrect-variable-scoping-in-sections)
4. [Bug 4: Stack Overflow with Private Arrays](#bug-4-stack-overflow-with-private-arrays)
5. [Bug 5: Deadlock with Nested Locks](#bug-5-deadlock-with-nested-locks)
6. [Bug 6: Uninitialized Reduction Variable](#bug-6-uninitialized-reduction-variable)

---

## Bug 1: Missing Schedule Specification

**File:** `bugs/bug1.c`

### The Problem

The `#pragma omp parallel for` directive lacks an explicit `schedule` clause, which can lead to unpredictable work distribution and suboptimal performance. While this may not always cause a runtime error, it results in:

- **Implicit default scheduling** varies by compiler/system
- **Unpredictable work distribution** among threads
- **Potential load imbalance** if iterations have different execution times
- **Difficult debugging** - behavior changes across systems

### Current Problematic Code

```c
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
}
```

**Wait - this code DOES have schedule!** The actual bug is:

### The Real Issue

The code structure is incorrect:

- `#pragma omp parallel for` should be followed directly by the `for` loop
- The **explicit braces `{}`** around the directive body are wrong
- Loop variable `i` should not be inside braces after the pragma

### How to Fix It

**Remove the braces:**

```c
#pragma omp parallel for shared(a, b, c, chunk) \
    private(i, tid)                             \
    schedule(static, chunk)
for (i = 0; i < N; i++)
{
    c[i] = a[i] + b[i];
    printf("tid= %d i= %d c[i]= %f\n", tid, i, c[i]);
}
```

### Key Lesson

- `#pragma omp parallel for` applies directly to the following `for` statement
- Never wrap `#pragma omp for` directives in extra braces
- The for loop body can have braces, but not between pragma and for

---

## Bug 2: Incorrect Parallel For Syntax

**File:** `bugs/bug2.c`

### The Problem

The code attempts to declare a local thread ID variable inside the`#pragma omp for` region but doesn't properly scope it. This creates confusion about variable ownership and can result in:

- **Undefined behavior** - variable scope mismatch
- **Race conditions** - if tid is accidentally shared
- **Compiler warnings or errors** - improper scoping

### Current Problematic Code

```c
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
}
```

### The Issue

Same as Bug 1 - the extra braces are the problem. Additionally:

- `tid` initialization outside the loop (inside braces) is problematic
- Creating scope issues with work-sharing directives

### How to Fix It

**Combine parallel and for correctly:**

```c
#pragma omp parallel shared(a, b, c, chunk) private(i, tid) schedule(static, chunk)
{
    tid = omp_get_thread_num();

#pragma omp for
    for (i = 0; i < N; i++)
    {
        c[i] = a[i] + b[i];
        printf("tid= %d i= %d c[i]= %f\n", tid, i, c[i]);
    }
}
```

OR use the combined directive:

```c
#pragma omp parallel for shared(a, b, c, chunk) \
    private(i, tid) schedule(static, chunk)
for (i = 0; i < N; i++)
{
    c[i] = a[i] + b[i];
    printf("tid= %d i= %d c[i]= %f\n", tid, i, c[i]);
}
```

### Key Lesson

- `#pragma omp parallel for` is a combined directive
- Do not add extra braces between the pragma and the for loop
- If adding braces and separating into `parallel` and `for`, nest them properly
- Proper scoping of variables prevents race conditions

---

## Bug 3: Incorrect Variable Scoping in Sections

**File:** `bugs/bug3.c`

### The Problem

The array `c` is declared **private** to each thread, which means each thread has its own copy. This causes:

- **Data loss** - results computed in one section aren't visible to others
- **Incorrect output** - different threads overwrite the same private variable
- **Unpredictable results** - which thread's data gets printed depends on scheduling

### Current Problematic Code

```c
#pragma omp parallel private(c, i, tid, section)
{
    // ...
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
    }
}
```

### The Issue

- `c` is in the `private()` clause
- Each thread gets its own private copy of array `c`
- Section 1 computes into thread's private `c`
- Section 2 computes into a different private `c`
- Results don't accumulate - they overwrite thread-local data

### How to Fix It

**Remove `c` from private clause - make it shared:**

```c
#pragma omp parallel private(i, tid, section) shared(c, a, b, nthreads)
{
    tid = omp_get_thread_num();
    if (tid == 0)
    {
        nthreads = omp_get_num_threads();
        printf("Number of threads = %d\n", nthreads);
    }

#pragma omp barrier
    printf("Thread %d starting...\n", tid);
#pragma omp barrier

#pragma omp sections nowait
    {
#pragma omp section
        {
            section = 1;
            for (i = 0; i < N; i++)
                c[i] = a[i] * b[i];  // Now writes to shared c
            print_results(c, tid, section);
        }

#pragma omp section
        {
            section = 2;
            for (i = 0; i < N; i++)
                c[i] = a[i] + b[i];  // Now writes to shared c
            print_results(c, tid, section);
        }
    }
}
```

### Alternative: Use Different Arrays

If you want each section to compute into a separate array:

```c
#pragma omp parallel private(i, tid, section) shared(c, d, a, b, nthreads)
{
    // ...
#pragma omp sections nowait
    {
#pragma omp section
        {
            for (i = 0; i < N; i++)
                c[i] = a[i] * b[i];  // Multiplication into c
        }

#pragma omp section
        {
            for (i = 0; i < N; i++)
                d[i] = a[i] + b[i];  // Addition into d
        }
    }
}
```

### Key Lesson

- **shared**: Variable accessible to all threads (one copy)
- **private**: Each thread gets its own copy (data not shared)
- Use `shared` for data that must be combined or visible across threads
- Use `private` for thread-local temporary variables only

---

## Bug 4: Stack Overflow with Private Arrays

**File:** `bugs/bug4.c`

### The Problem

A large 2D array (1048 × 1048) is declared as **private**, causing each thread to allocate ~8.8 MB on its stack. With multiple threads, this exhausts stack memory and causes:

- **Stack overflow crash** - insufficient stack space per thread
- **Segmentation fault** - memory access violation
- **Program termination** - unable to continue execution

### Current Problematic Code

```c
#pragma omp parallel shared(nthreads) private(i, j, tid, a)
{
    tid = omp_get_thread_num();
    if (tid == 0)
    {
        nthreads = omp_get_num_threads();
        printf("Number of threads = %d\n", nthreads);
    }
    printf("Thread %d starting...\n", tid);

    // Each thread allocates 1048 × 1048 × 8 bytes ≈ 8.8 MB on stack!!!
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            a[i][j] = tid + i + j;

    printf("Thread %d done. Last element= %f\n", tid, a[N - 1][N - 1]);
}
```

### Memory Analysis

```
Array dimensions: 1048 × 1048
Element size: 8 bytes (double)
Memory per thread: 1048 × 1048 × 8 = 8,781,824 bytes ≈ 8.8 MB
Number of threads: typically 4-8
Total memory needed: 8.8 MB × num_threads ≈ 35-70 MB
Default stack size per thread: usually 1-8 MB
Result: STACK OVERFLOW
```

### How to Fix It

**Option 1: Make the array static (global)**

```c
#define N 1048
double a[N][N];  // Global - allocated on heap/data segment

int main(int argc, char *argv[])
{
    // ...
#pragma omp parallel shared(nthreads, a) private(i, j, tid)
    {
        // ... rest of code - a is now shared
    }
}
```

**Option 2: Dynamically allocate on heap**

```c
#pragma omp parallel shared(nthreads, a) private(i, j, tid)
{
    // Use dynamically allocated memory or local small working arrays
    double temp[100][100];  // Small working array

    // ...
}
```

**Option 3: Make it shared to use single copy**

```c
#pragma omp parallel shared(nthreads, a) private(i, j, tid)
{
    tid = omp_get_thread_num();
    if (tid == 0)
    {
        nthreads = omp_get_num_threads();
        printf("Number of threads = %d\n", nthreads);
    }
    printf("Thread %d starting...\n", tid);

#pragma omp for collapse(2)
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            a[i][j] = tid + i + j;

    printf("Thread %d done. Last element= %f\n", tid, a[N - 1][N - 1]);
}
```

### Key Lesson

- **Stack space is limited per thread** - typically 1-8 MB
- **Private large arrays cause stack overflow** - don't use private for big data
- **Global/static arrays** allocate on heap/data segment (plenty of space)
- **Shared arrays** use single copy (all threads access same memory)
- For large data, prefer global or shared over private

---

## Bug 5: Deadlock with Nested Locks

**File:** `bugs/bug5.c`

### The Problem

Two locks are acquired in **different orders** in different sections, causing a circular wait condition. When one thread holds a lock that another needs, and vice versa:

- **Deadlock occurs** - both threads wait forever
- **Program hangs** - never completes
- **No error message** - appears to be running but frozen

### Current Problematic Code

```c
#pragma omp sections nowait
{
#pragma omp section
    {
        printf("Thread %d initializing a[]\n", tid);
        omp_set_lock(&locka);           // Acquire lock A
        for (i = 0; i < N; i++)
            a[i] = i * DELTA;
        omp_set_lock(&lockb);           // Try to acquire lock B - WAIT
        printf("Thread %d adding a[] to b[]\n", tid);
        for (i = 0; i < N; i++)
            b[i] += a[i];
        omp_unset_lock(&lockb);
        omp_unset_lock(&locka);
    }

#pragma omp section
    {
        printf("Thread %d initializing b[]\n", tid);
        omp_set_lock(&lockb);           // Acquire lock B
        for (i = 0; i < N; i++)
            b[i] = i * PI;
        omp_set_lock(&locka);           // Try to acquire lock A - WAIT
        printf("Thread %d adding b[] to a[]\n", tid);
        for (i = 0; i < N; i++)
            a[i] += b[i];
        omp_unset_lock(&locka);
        omp_unset_lock(&lockb);
    }
}
```

### Deadlock Scenario

```
Time 1:
  Thread executing Section 1: Acquires LOCK_A
  Thread executing Section 2: Acquires LOCK_B

Time 2:
  Thread 1: Waits for LOCK_B (held by Thread 2)
  Thread 2: Waits for LOCK_A (held by Thread 1)

Result: DEADLOCK - Neither thread can proceed
Both threads block forever on omp_set_lock() call
```

### How to Fix It

**Solution: Enforce consistent lock ordering**

Acquire locks in same order everywhere:

```c
#pragma omp sections nowait
{
#pragma omp section
    {
        printf("Thread %d initializing a[]\n", tid);
        omp_set_lock(&locka);           // ALWAYS acquire A first
        omp_set_lock(&lockb);           // THEN acquire B

        for (i = 0; i < N; i++)
            a[i] = i * DELTA;
        printf("Thread %d adding a[] to b[]\n", tid);
        for (i = 0; i < N; i++)
            b[i] += a[i];

        omp_unset_lock(&lockb);         // Release in reverse order
        omp_unset_lock(&locka);
    }

#pragma omp section
    {
        printf("Thread %d initializing b[]\n", tid);
        omp_set_lock(&locka);           // ALWAYS acquire A first (same order)
        omp_set_lock(&lockb);           // THEN acquire B

        for (i = 0; i < N; i++)
            b[i] = i * PI;
        printf("Thread %d adding b[] to a[]\n", tid);
        for (i = 0; i < N; i++)
            a[i] += b[i];

        omp_unset_lock(&lockb);         // Release in reverse order
        omp_unset_lock(&locka);
    }
}
```

### Alternative: Use Critical Section (Simpler)

```c
#pragma omp section
{
    printf("Thread %d initializing a[]\n", tid);
    for (i = 0; i < N; i++)
        a[i] = i * DELTA;

#pragma omp critical
    {
        printf("Thread %d adding a[] to b[]\n", tid);
        for (i = 0; i < N; i++)
            b[i] += a[i];
    }
}

#pragma omp section
{
    printf("Thread %d initializing b[]\n", tid);
    for (i = 0; i < N; i++)
        b[i] = i * PI;

#pragma omp critical
    {
        printf("Thread %d adding b[] to a[]\n", tid);
        for (i = 0; i < N; i++)
            a[i] += b[i];
    }
}
```

### Key Lesson

- **Lock ordering must be consistent** across all code paths
- **Circular dependencies cause deadlock** - A waits for B, B waits for A
- **Release locks in reverse order** of acquisition (LIFO)
- **Minimize critical sections** - lock only what you need
- **Consider alternatives** - atomic operations or critical sections may be simpler

---

## Bug 6: Uninitialized Reduction Variable

**File:** `bugs/bug6.c`

### The Problem

A reduction variable `sum` is declared **locally in a function** but **shared from main**, causing two separate variables with the same name. The function's local uninitialized `sum` is used in reduction, resulting in:

- **Garbage values** - uninitialized local sum contains random data
- **Incorrect computation** - reduction operates on undefined data
- **Wrong result** - final answer is meaningless
- **Silent failure** - no error, just garbage output

### Current Problematic Code

```c
float dotprod()
{
    int i, tid;
    float sum;  // LOCAL UNINITIALIZED VARIABLE - WRONG!

    tid = omp_get_thread_num();
#pragma omp for reduction(+ : sum)
    for (i = 0; i < VECLEN; i++)
    {
        sum = sum + (a[i] * b[i]);  // sum starts with garbage!
    }
}

int main(int argc, char *argv[])
{
    int i;
    float sum;  // DIFFERENT VARIABLE - ignored by function

    for (i = 0; i < VECLEN; i++)
        a[i] = b[i] = 1.0 * i;
    sum = 0.0;

#pragma omp parallel shared(sum)  // Main's sum is shared
        dotprod();                 // But function uses local sum!

    printf("Sum = %f\n", sum);  // sum was never updated by function
}
```

### Variable Scope Problem

```
main() scope:
  float sum = 0.0;           ← Shared via shared(sum)

dotprod() scope:
  float sum;                 ← Different variable (local, uninitialized)
  #pragma omp for reduction(+ : sum)  ← Operates on local sum
```

**Two separate variables:**

- Main's `sum`: Shared, initialized to 0.0, never modified by function
- Function's `sum`: Local, uninitialized, used in reduction
- Result printed: Garbage or zero (depending on earlier memory content)

### How to Fix It

**Solution 1: Use global variable (simplest)**

```c
float a[VECLEN], b[VECLEN], sum;  // Global - visible to all functions

float dotprod()
{
    int i, tid;
    tid = omp_get_thread_num();
#pragma omp for reduction(+ : sum)
    for (i = 0; i < VECLEN; i++)
    {
        sum = sum + (a[i] * b[i]);  // Uses global sum
    }
}

int main(int argc, char *argv[])
{
    int i;

    for (i = 0; i < VECLEN; i++)
        a[i] = b[i] = 1.0 * i;
    sum = 0.0;

#pragma omp parallel
        dotprod();  // Now works correctly with global sum

    printf("Sum = %f\n", sum);
}
```

**Solution 2: Pass sum as parameter**

```c
float dotprod(float *sum_ptr)
{
    int i, tid;
    tid = omp_get_thread_num();
#pragma omp for reduction(+ : (*sum_ptr))
    for (i = 0; i < VECLEN; i++)
    {
        *sum_ptr = *sum_ptr + (a[i] * b[i]);
    }
    return *sum_ptr;
}

int main(int argc, char *argv[])
{
    int i;
    float sum;

    for (i = 0; i < VECLEN; i++)
        a[i] = b[i] = 1.0 * i;
    sum = 0.0;

#pragma omp parallel
        dotprod(&sum);  // Pass reference to sum

    printf("Sum = %f\n", sum);
}
```

**Solution 3: Initialize local variable**

```c
float dotprod()  // If must be local
{
    int i, tid;
    float sum = 0.0;  // Initialize local sum

    tid = omp_get_thread_num();
#pragma omp for reduction(+ : sum)
    for (i = 0; i < VECLEN; i++)
    {
        sum = sum + (a[i] * b[i]);
    }
    return sum;
}

int main(int argc, char *argv[])
{
    int i;
    float total = 0.0;

    for (i = 0; i < VECLEN; i++)
        a[i] = b[i] = 1.0 * i;

#pragma omp parallel
    {
        float partial_sum = dotprod();
#pragma omp critical
        total += partial_sum;
    }

    printf("Sum = %f\n", total);
}
```

### Key Lesson

- **Variable scope matters** - local and global variables with same name are different
- **Uninitialized variables contain garbage** - always initialize before use
- **Reduction applies to variable in current scope** - ensure correct scope
- **Prefer globals or parameters** for variables shared across function boundaries
- **Initialize all variables** before using in reduction operations

---

## Summary of All Bugs

| Bug | File   | Issue                                     | Fix                               |
| --- | ------ | ----------------------------------------- | --------------------------------- |
| 1   | bug1.c | Extra braces in parallel for              | Remove braces after pragma        |
| 2   | bug2.c | Syntax error with scope                   | Proper parallel/for nesting       |
| 3   | bug3.c | Array c incorrectly private               | Make c shared                     |
| 4   | bug4.c | Large private array causes stack overflow | Use global or shared array        |
| 5   | bug5.c | Deadlock - inconsistent lock ordering     | Acquire locks in consistent order |
| 6   | bug6.c | Uninitialized local reduction variable    | Use global or initialize          |

---

## Testing After Fixes

After applying fixes, test each bug file:

```bash
gcc -fopenmp -o bug1 bugs/bug1.c
./bug1

gcc -fopenmp -o bug2 bugs/bug2.c
./bug2

gcc -fopenmp -o bug3 bugs/bug3.c
./bug3

gcc -fopenmp -o bug4 bugs/bug4.c
./bug4

gcc -fopenmp -o bug5 bugs/bug5.c         # Watch for hang (deadlock)
./bug5

gcc -fopenmp -o bug6 bugs/bug6.c
./bug6
```

Expected outcomes after fixes:

- Programs compile without errors
- Run without warnings
- Produce correct numerical results
- No crashes, hangs, or undefined behavior
