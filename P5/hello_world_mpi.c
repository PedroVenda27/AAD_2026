//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// MPI hello world example -- for any number of processes
//

#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc,char **argv)
{
  char host_name[256];
  int n_processes,rank;

  //
  // initialize the MPI environment, and get the number of processes and the MPI number of our process (the rank)
  //
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&n_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //
  // print a hello world message
  //
  if(gethostname(host_name,sizeof(host_name)) < 0)
  {
    host_name[0] = '?';
    host_name[1] = '\0';
  }
  printf("Hello world from %d/%d [%s]\n",rank,n_processes,host_name);
  //
  // terminate the MPI environment
  //
  MPI_Finalize();
}
