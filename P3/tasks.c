//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// OpenMP tasks
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
// a simple linked list node
//

typedef struct list_node_s
{
  int                 work_id;    // work identifier
  unsigned int        work_to_do; // loop count
  struct list_node_s *next;       // next node
}
list_node_t;


//
// main program
//

int main(void)
{
  //
  // initialize the linked list
  //
  list_node_t *head,nodes[20];
  for(int idx = 0;idx < (int)(sizeof(nodes) / sizeof(nodes[0]));idx++)
  {
    nodes[idx].work_id = idx;
    nodes[idx].work_to_do = 1000u + ((2654435789u * (unsigned int)idx) & 0x00FFFFFFu);
    nodes[idx].next = (idx < (int)(sizeof(nodes) / sizeof(nodes[0])) - 1) ? &nodes[idx + 1] : NULL;
  }
  head = &nodes[0];
  //
  // process each node of the linked list in parallel (using tasks)
  //
# pragma omp parallel num_threads(n_threads)
  { // the previous line creates a team of threads (pool of threads = team of threads in OpenMP)
#   pragma omp single
    { // the previous line says that a single thread will execute the code inside this block
      for(list_node_t *p = head;p != NULL;p = p->next)
      {
#       pragma omp task firstprivate(p)
        { // the previous line says one of the threads waiting at the end of the single block executes this block, with a private copy of the current value of p
          unsigned int val = 0u;
          for(unsigned int idx = 0u;idx < p->work_to_do;idx++)
            val = 13u * val + 1u;
          printf("thread %2d did job %2d [%8X]\n",omp_get_thread_num(),p->work_id,val);
        }
      }
    } // threads wait at the end of the block; threads waiting here execute tasks; the block is exited when all tasks are done
  } // terminate the team of threads
  //
  // done
  //
  return 0;
}
