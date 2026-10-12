//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// broadcast a structure from rank 0 (for example) to all other ranks
//

#include <mpi.h>
#include <stdio.h>

typedef struct my_data_type_s
{
  int i[2];    // two integers
  double d[3]; // three doubles
}
my_data_type_t;

int main(int argc,char **argv)
{
  MPI_Datatype my_data_type_mpi;
  my_data_type_t data;
  int n_processes,rank;

  //
  // initialize the MPI environment, and get the number of processes and the MPI number of our process (the rank)
  //
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&n_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //
  // description of the custom data type
  //
  {
    MPI_Datatype array_of_types[2];
    int array_of_block_lengths[2];
    MPI_Aint array_of_displacements[2];

    array_of_types        [0] = MPI_INT;
    array_of_block_lengths[0] = 2;
    array_of_displacements[0] = (MPI_Aint)offsetof(my_data_type_t,i);
    array_of_types        [1] = MPI_DOUBLE;
    array_of_block_lengths[1] = 3;
    array_of_displacements[1] = (MPI_Aint)offsetof(my_data_type_t,d);
    // create the custom data type
    MPI_Type_create_struct(2,array_of_block_lengths,array_of_displacements,array_of_types,&my_data_type_mpi);
    // make it available in MPI_Send() and MPI_Recv()
    MPI_Type_commit(&my_data_type_mpi);
  }
  //
  // rank 0 initialization
  //
  if(rank == 0)
  {
    data.i[0] = 3;
    data.i[1] = -1;
    data.d[0] = 1.234;
    data.d[1] = 5.678;
    data.d[2] = -2.0;
  }
  //
  // broadcast it to all other ranks
  //
  MPI_Bcast((void *)&data,1,my_data_type_mpi,0,MPI_COMM_WORLD); // 1 data item, sent from rank 0
  //
  printf("rank %d: i[0]=%d i[1]=%d d[0]=%.3f d[1]=%.3f d[2]=%.f\n",rank,data.i[0],data.i[1],data.d[0],data.d[1],data.d[2]);
  //
  // clean up and terminate the MPI environment
  //
  MPI_Type_free(&my_data_type_mpi);
  MPI_Finalize();
}
