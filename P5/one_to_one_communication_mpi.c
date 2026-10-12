//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// part 1
//   rank 0 -> rank 1 message -- for two or more processes
// part 2
//   synchonization barrier
// part 3
//   rank k -> rank k+1 message -- for two or more processes (may deadlock...)
//

#include <mpi.h>
#include <stdio.h>

int main(int argc,char **argv)
{
  int n_processes,rank,data[10];
  MPI_Status status;
  int i,n;

  //
  // initialize the MPI environment, and get the number of processes and the MPI number of our process (the rank)
  //
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&n_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //
  // part 1 --- send a message (rank 0) and receive a message (rank 1)
  //
  switch(rank)
  {
    case 0: // initialize data[] and send it to the process with rank 1
      for(i = 0;i < 10;i++)
        data[i] = i;
      // send 10 integers to destination 1 with tag 1234, using the world (global) communicator
      MPI_Send((const void *)&data[0],10,MPI_INT,1,1234,MPI_COMM_WORLD);
      printf("rank %d: message sent to rank 1\n",rank);
      break;
    case 1:
      // receive at most 10 integers from any source with any tag, using the world communicator
      MPI_Recv((void *)&data[0],10,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status); // MPI_STATUS_IGNORE
      MPI_Get_count(&status,MPI_INT,&n);
      printf("rank %d: received %d integer%s from rank %d using tag %d\n",rank,n,(n == 1) ? "" : "s",status.MPI_SOURCE,status.MPI_TAG);
      for(i = 0;i < n;i++)
        printf("  rank %d: data[%d] = %d\n",rank,i,data[i]);
      break;
    default:
      printf("rank %d: inactive\n",rank);
  }
  //
  // part 2 --- example of a synchonization barrier, using the world communicator
  //
  MPI_Barrier(MPI_COMM_WORLD);
  //
  // part 3 --- each process sends a message to its next in rank, and receives a message from its previous in rank
  //
  // WARNING: using MPI_Send() and MPI_Recv() in this way IS NOT SAFE, as it may lead to a deadlock when the messages are large
  //          to see the problem, replace MPI_Send() with MPI_Ssend(), which is a synchronous send (it blocks until the message is received)
  //          an MPI implementation may use synchronous sends for large messages
  //   solution when n_processes is even: processes with even rank first send, then receive
  //                                      processed with odd rank first receive, then send
  //   alternative solutions: use MPI_Isend(), or MPI_Sendrecv(), or MPI_Sendrecv_replace()
  //
  // the version of part 3 is chosen at compile time with -DPART3=...
  //   PART3=0: everybody first sends (synchronous send, MPI_Ssend()) and then receives --- DEADLOCK
  //   PART3=1: fix 1 --- even ranks first send and then receive, odd ranks first receive and then send
  //   PART3=2: fix 2 --- MPI_Sendrecv(), which sends and receives at the same time
  //
#ifndef PART3
# define PART3  0
#endif
  data[0] = rank;
#if PART3 == 0
  // send 1 integer to destination rank+1 with tag 42, using the world (global) communicator
  MPI_Ssend((const void *)&data[0],1,MPI_INT,(rank + 1) % n_processes,42,MPI_COMM_WORLD);
  printf("rank %d: message sent to rank %d\n",rank,(rank + 1) % n_processes);
  // receive at most 10 integers from source rank-1 with tag 42, using the world communicator
  MPI_Recv((void *)&data[0],10,MPI_INT,(rank + n_processes - 1) % n_processes,42,MPI_COMM_WORLD,&status);
#elif PART3 == 1
  if(rank % 2 == 0)
  { // even rank: first send, then receive
    MPI_Ssend((const void *)&data[0],1,MPI_INT,(rank + 1) % n_processes,42,MPI_COMM_WORLD);
    printf("rank %d: message sent to rank %d\n",rank,(rank + 1) % n_processes);
    MPI_Recv((void *)&data[0],10,MPI_INT,(rank + n_processes - 1) % n_processes,42,MPI_COMM_WORLD,&status);
  }
  else
  { // odd rank: first receive, then send (the data to be sent must be saved, because the receive overwrites data[0])
    int my_rank = rank;

    MPI_Recv((void *)&data[0],10,MPI_INT,(rank + n_processes - 1) % n_processes,42,MPI_COMM_WORLD,&status);
    MPI_Ssend((const void *)&my_rank,1,MPI_INT,(rank + 1) % n_processes,42,MPI_COMM_WORLD);
    printf("rank %d: message sent to rank %d\n",rank,(rank + 1) % n_processes);
  }
#else
  { // send and receive in a single call (the send and receive buffers must be different)
    int my_rank = rank;

    MPI_Sendrecv((const void *)&my_rank,1,MPI_INT,(rank + 1) % n_processes,42,
                 (void *)&data[0],10,MPI_INT,(rank + n_processes - 1) % n_processes,42,
                 MPI_COMM_WORLD,&status);
    printf("rank %d: message sent to rank %d\n",rank,(rank + 1) % n_processes);
  }
#endif
  MPI_Get_count(&status,MPI_INT,&n);
  printf("rank %d: received %d integer%s from rank %d using tag %d\n",rank,n,(n == 1) ? "" : "s",status.MPI_SOURCE,status.MPI_TAG);
  for(i = 0;i < n;i++)
    printf("  rank %d: data[%d] = %d\n",rank,i,data[i]);
  //
  // terminate the MPI environment
  //
  MPI_Finalize();
}
