//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// OpemMP parallelization reduction
// * add in parallel using a reduction
//

#include <omp.h>
#include <stdio.h>


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
// main program
//

int main(void)
{
  const int array_size = 1000;
  int array[array_size];

  //
  // set the default number of threads
  //
  omp_set_num_threads(n_threads);
  //
  // initialize array (in parallel :-)
  //
# pragma omp parallel for
  for(int idx = 0;idx < array_size;idx++)
    array[idx] = idx;
  //
  // add the array contents in parallel
  // * partial sums in different variables
  // * locks only at the end to update the reduction variable
  //
  int sum = 0;
# pragma omp parallel for reduction(+:sum)
  for(int idx = 0;idx < array_size;idx++)
    sum += array[idx];
  printf("sum=%d (expected %d)\n",sum,(array_size * (array_size - 1)) >> 1);
  //
  // minimum and maximum, also computed with a reduction, in the same loop
  // * each thread keeps its own partial min/max (no locking at all during the loop)
  // * at the end, the partial values are combined using the min/max operator
  //
  int min_val = array[0];
  int max_val = array[0];
# pragma omp parallel for reduction(min:min_val) reduction(max:max_val)
  for(int idx = 0;idx < array_size;idx++)
  {
    if(array[idx] < min_val) min_val = array[idx];
    if(array[idx] > max_val) max_val = array[idx];
  }
  printf("min=%d (expected %d)\n",min_val,0);
  printf("max=%d (expected %d)\n",max_val,array_size - 1);
  //
  // done
  //
  return 0;
}
