//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// OpenMP critical regions
//

#include <omp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>


//
// number of threads
//

#ifndef n_threads
# define n_threads   4
#endif
#if n_threads < 2 || n_threads > 16
# error "too few or too many threads"
#endif


//
// wall time (POSIX)
//

static double wall_time(void)
{
  struct timespec current_time;

  if(clock_gettime(CLOCK_REALTIME,&current_time) != 0)
    exit(1); // silent exit: clock_gettime() failed!!!
  return (double)current_time.tv_sec + 1.0e-9 * (double)current_time.tv_nsec;
}


//
// main program
//

int main(void)
{
  volatile int shared_counter;         // the shared counter (declared volatile, so the compiler does not optimize acesses to it)
  const int loop_iterations = 1000000; // number of times each thread increments the shared counter
  double t0,t1;

  //
  // set the default number of threads
  //
  omp_set_num_threads(n_threads);
  //
  // increment the counter without a critical region
  //
  shared_counter = 0;
  t0 = wall_time();
# pragma omp parallel
  {
    for(int idx = 0;idx < loop_iterations;idx++)
      shared_counter++;
  }
  t1 = wall_time();
  printf("shared_counter=%d (expected %d)   time=%.3f (no critical section)\n",shared_counter,n_threads * loop_iterations,t1 - t0);
  //
  // increment the counter with a critical region
  //
  shared_counter = 0;
  t0 = wall_time();
# pragma omp parallel
  {
    for(int idx = 0;idx < loop_iterations;idx++)
#   pragma omp critical (lock_increment_unlock)
    {
      shared_counter++;
    }
  }
  t1 = wall_time();
  printf("shared_counter=%d (expected %d)   time=%.3f (with critical section)\n",shared_counter,n_threads * loop_iterations,t1 - t0);
  //
  // done
  //
  return 0;
}
