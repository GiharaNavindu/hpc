@echo off
cd /d d:\7th\HPC\codebase

echo Committing arrayloops.c...
git add posix/arrayloops.c
git commit -m "Add POSIX threads array processing example with mutex-protected global sum"
git push origin main

echo Committing conditionvars.c...
git add posix/conditionvars.c
git commit -m "Add POSIX condition variables example demonstrating thread synchronization"
git push origin main

echo Committing detached.c...
git add posix/detached.c
git commit -m "Add POSIX detached threads example showing resource conservation pattern"
git push origin main

echo Committing dotprod_mutex.c...
git add posix/dotprod_mutex.c
git commit -m "Add POSIX threaded dot product with mutex synchronization"
git push origin main

echo Committing dotprod_serial.c...
git add posix/dotprod_serial.c
git commit -m "Add serial dot product baseline for pthread comparison"
git push origin main

echo Committing hello.c...
git add posix/hello.c
git commit -m "Add basic POSIX threads hello world example"
git push origin main

echo Committing hello32.c...
git add posix/hello32.c
git commit -m "Add POSIX threads example with 32 concurrent threads and scheduler demonstration"
git push origin main

echo Committing hello_arg1.c...
git add posix/hello_arg1.c
git commit -m "Add POSIX threads example demonstrating safe argument passing to threads"
git push origin main

echo Committing hello_arg2.c...
git add posix/hello_arg2.c
git commit -m "Add POSIX threads example with struct-based argument passing"
git push origin main

echo Committing join.c...
git add posix/join.c
git commit -m "Add POSIX threads join example demonstrating thread completion synchronization"
git push origin main

echo Committing mpi_thread_serial.c...
git add posix/mpi_thread_serial.c
git commit -m "Add serial dot product baseline for MPI/Pthreads hybrid comparison"
git push origin main

echo Committing mpi_thread_thread.c...
git add posix/mpi_thread_thread.c
git commit -m "Add Pthreads-only dot product demonstrating shared memory parallelism"
git push origin main

echo Committing mpi_thread_mpi.c...
git add posix/mpi_thread_mpi.c
git commit -m "Add MPI-only dot product demonstrating distributed memory parallelism"
git push origin main

echo Committing mpi_thread_both.c...
git add posix/mpi_thread_both.c
git commit -m "Add hybrid MPI/Pthreads dot product demonstrating combined parallelism model"
git push origin main

echo All files committed and pushed successfully!