//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P2, section 7
//
// Mandelbrot set count, parallel version 3: critical region (mutex) work distribution
// the 8193x8193 square is divided in 482x482 blocks (17x17 = 289 blocks, since
// ceil(8193/482) = 17); each thread grabs the next unprocessed block, protected by a mutex
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define n_samples   8193
#define block_size   482
#define n_blocks_per_dim  ((n_samples + block_size - 1) / block_size) // 17
#define total_blocks      (n_blocks_per_dim * n_blocks_per_dim)       // 289

static const double c_re_min = -2.05;
static const double c_re_max = +0.55;
static const double c_im_min = -1.30;
static const double c_im_max = +1.30;

static double wall_time(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  return (double)ts.tv_sec + 1.0e-9 * (double)ts.tv_nsec;
}

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

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int next_block = 0; // next unprocessed block index, protected by lock

typedef struct
{
  int thread_number;
  long count;
  int blocks_done; // for curiosity: how many blocks this thread processed
}
thread_data_t;

static void *worker_thread(void *arg)
{
  thread_data_t *td = (thread_data_t *)arg;
  long local_count = 0;
  int blocks_done = 0;
  for(;;)
  {
    int my_block;
    if(pthread_mutex_lock(&lock) != 0) { fprintf(stderr,"pthread_mutex_lock() failed\n"); exit(1); }
    my_block = next_block;
    if(my_block < total_blocks)
      next_block++;
    if(pthread_mutex_unlock(&lock) != 0) { fprintf(stderr,"pthread_mutex_unlock() failed\n"); exit(1); }
    if(my_block >= total_blocks)
      break; // no more work
    blocks_done++;
    int block_row = my_block / n_blocks_per_dim;
    int block_col = my_block % n_blocks_per_dim;
    int re_start = block_col * block_size, re_end = re_start + block_size;
    int im_start = block_row * block_size, im_end = im_start + block_size;
    if(re_end > n_samples) re_end = n_samples;
    if(im_end > n_samples) im_end = n_samples;
    for(int re_idx = re_start;re_idx < re_end;re_idx++)
    {
      double c_re = c_re_min + (double)re_idx * ((c_re_max - c_re_min) / (double)n_samples);
      for(int im_idx = im_start;im_idx < im_end;im_idx++)
      {
        double c_im = c_im_min + (double)im_idx * ((c_im_max - c_im_min) / (double)n_samples);
        local_count += mandelbrot_point(c_re,c_im);
      }
    }
  }
  td->count = local_count;
  td->blocks_done = blocks_done;
  printf("thread %2d: end at %.6f (blocks processed: %d)\n",td->thread_number,wall_time(),blocks_done);
  return NULL;
}

int main(int argc,char **argv)
{
  int n_threads = (argc > 1) ? atoi(argv[1]) : 4;
  if(n_threads < 1)  n_threads = 1;
  if(n_threads > 64) n_threads = 64;

  pthread_t tid[n_threads];
  thread_data_t td[n_threads];

  next_block = 0;
  double t0 = wall_time();
  for(int i = 0;i < n_threads;i++)
  {
    td[i].thread_number = i;
    if(pthread_create(&tid[i],NULL,worker_thread,&td[i]) != 0)
    {
      fprintf(stderr,"pthread_create() failed for idx=%d\n",i);
      exit(1);
    }
  }
  long total = 0;
  for(int i = 0;i < n_threads;i++)
  {
    if(pthread_join(tid[i],NULL) != 0)
    {
      fprintf(stderr,"pthread_join() failed for idx=%d\n",i);
      exit(1);
    }
    total += td[i].count;
  }
  double t1 = wall_time();
  printf("%ld -- %7.3f (n_threads=%d, critical region, %d blocks of %dx%d)\n",
         total,t1 - t0,n_threads,total_blocks,block_size,block_size);
  return 0;
}
