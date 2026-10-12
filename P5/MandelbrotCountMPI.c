//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P5, section 6
//
// Mandelbrot set count, MPI version
//
// usage: mpirun -np N ./MandelbrotCountMPI [cyclic|dynamic] [rows_per_item]
//
//   cyclic  (default): static distribution, rank R does the rows R, R+N, R+2N, ... (interleaved rows, like the
//                      static stripes of AAD_P2 / schedule(static,1) of AAD_P3); the partial counts are added with
//                      a single MPI_Reduce()
//   dynamic          : master/worker, rank 0 hands out blocks of rows_per_item rows (default 64) to the other ranks,
//                      which send back their counts (same idea as the server/clients of AAD_P4, but with MPI)
//

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define n_samples  8193
static const double c_re_min = -2.05;
static const double c_re_max = +0.55;
static const double c_im_min = -1.30;
static const double c_im_max = +1.30;

#define tag_work    1 // master -> worker: data = { re_first, re_last }
#define tag_stop    2 // master -> worker: no more work
#define tag_result  3 // worker -> master: data = count

static inline int mandelbrot_point(double c_re,double c_im)
{
  int n_iter = 0;
  double z_re = 0.0,z_im = 0.0;
  while(n_iter < 256 && z_re * z_re + z_im * z_im <= 4.0)
  {
    double tmp = z_re * z_re - z_im * z_im;
    z_im = 2.0 * z_re * z_im + c_im;
    z_re = tmp + c_re;
    n_iter++;
  }
  return (n_iter < 256) ? 1 : 0;
}

static int mandelbrot_row(int re_idx)
{
  double c_re = c_re_min + (double)re_idx * ((c_re_max - c_re_min) / (double)n_samples);
  int count = 0;

  for(int im_idx = 0;im_idx < n_samples;im_idx++)
  {
    double c_im = c_im_min + (double)im_idx * ((c_im_max - c_im_min) / (double)n_samples);
    count += mandelbrot_point(c_re,c_im);
  }
  return count;
}


//
// dynamic distribution: master (rank 0) code
//

static void master(int n_processes,int rows_per_item)
{
  int next_row = 0,active_workers = 0,data[2],count;
  MPI_Status status;

  // give one block to each worker (or tell it to stop if there is no work for it)
  for(int w = 1;w < n_processes;w++)
    if(next_row < n_samples)
    {
      data[0] = next_row;
      data[1] = (next_row + rows_per_item < n_samples) ? next_row + rows_per_item : n_samples;
      next_row = data[1];
      MPI_Send(data,2,MPI_INT,w,tag_work,MPI_COMM_WORLD);
      active_workers++;
    }
    else
      MPI_Send(data,0,MPI_INT,w,tag_stop,MPI_COMM_WORLD);
  // each time a worker returns a result, give it another block or tell it to stop
  while(active_workers > 0)
  {
    MPI_Recv(&count,1,MPI_INT,MPI_ANY_SOURCE,tag_result,MPI_COMM_WORLD,&status);
    (void)count; // the counts are added at the end with MPI_Reduce()
    if(next_row < n_samples)
    {
      data[0] = next_row;
      data[1] = (next_row + rows_per_item < n_samples) ? next_row + rows_per_item : n_samples;
      next_row = data[1];
      MPI_Send(data,2,MPI_INT,status.MPI_SOURCE,tag_work,MPI_COMM_WORLD);
    }
    else
    {
      MPI_Send(data,0,MPI_INT,status.MPI_SOURCE,tag_stop,MPI_COMM_WORLD);
      active_workers--;
    }
  }
}


//
// dynamic distribution: worker (rank > 0) code; returns the sum of the counts of all blocks it did
//

static int worker(int *n_blocks)
{
  int data[2],count,my_count = 0;
  MPI_Status status;

  *n_blocks = 0;
  for(;;)
  {
    MPI_Recv(data,2,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
    if(status.MPI_TAG == tag_stop)
      break;
    count = 0;
    for(int re_idx = data[0];re_idx < data[1];re_idx++)
      count += mandelbrot_row(re_idx);
    my_count += count;
    (*n_blocks)++;
    MPI_Send(&count,1,MPI_INT,0,tag_result,MPI_COMM_WORLD);
  }
  return my_count;
}


int main(int argc,char **argv)
{
  int n_processes,rank,dynamic,rows_per_item,my_count,total_count,my_items;
  double t0,t1,my_time,max_time;

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&n_processes);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  dynamic = (argc > 1 && strcmp(argv[1],"dynamic") == 0) ? 1 : 0;
  rows_per_item = (argc > 2) ? atoi(argv[2]) : 64;
  if(rows_per_item < 1)
    rows_per_item = 1;
  if(dynamic != 0 && n_processes < 2)
  {
    if(rank == 0)
      fprintf(stderr,"the dynamic distribution needs at least 2 processes (1 master + workers); using cyclic\n");
    dynamic = 0;
  }
  //
  // do the work (all processes start at the same time)
  //
  MPI_Barrier(MPI_COMM_WORLD);
  t0 = MPI_Wtime();
  my_count = my_items = 0;
  if(dynamic == 0)
  { // cyclic: rows rank, rank + n_processes, rank + 2 * n_processes, ...
    for(int re_idx = rank;re_idx < n_samples;re_idx += n_processes)
    {
      my_count += mandelbrot_row(re_idx);
      my_items++;
    }
  }
  else if(rank == 0)
    master(n_processes,rows_per_item);
  else
    my_count = worker(&my_items);
  t1 = MPI_Wtime();
  //
  // add the partial counts of all processes (the result is placed in rank 0), and get the time of the slowest process
  //
  MPI_Reduce(&my_count,&total_count,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
  my_time = t1 - t0;
  MPI_Reduce(&my_time,&max_time,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  //
  // report (the per-rank data is gathered in rank 0 and printed there, because the output of different ranks
  // may appear in any order)
  //
  int all_items[n_processes];
  double all_times[n_processes];

  MPI_Gather(&my_items,1,MPI_INT,all_items,1,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Gather(&my_time,1,MPI_DOUBLE,all_times,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
  if(rank == 0)
  {
    for(int r = 0;r < n_processes;r++)
      printf("  rank %2d: %s %5d, compute time %7.3f s\n",r,(dynamic == 0) ? "rows  " : "blocks",all_items[r],all_times[r]);
    if(dynamic == 0)
      printf("%d -- %7.3f (n_processes=%d, cyclic)\n",total_count,max_time,n_processes);
    else
      printf("%d -- %7.3f (n_processes=%d, dynamic, rows_per_item=%d)\n",total_count,max_time,n_processes,rows_per_item);
  }
  MPI_Finalize();
  return 0;
}
