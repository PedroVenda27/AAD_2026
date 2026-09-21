//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P2, section 7
//
// Mandelbrot set count, parallel version 2: static division in horizontal stripes
// each thread processes a contiguous range of im_idx (a "row stripe"), all re_idx
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define n_samples  8193
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

typedef struct
{
  int thread_number;
  int im_start,im_end; // half-open range [im_start,im_end)
  long count;
}
thread_data_t;

static void *worker_thread(void *arg)
{
  thread_data_t *td = (thread_data_t *)arg;
  long local_count = 0;
  for(int im_idx = td->im_start;im_idx < td->im_end;im_idx++)
  {
    double c_im = c_im_min + (double)im_idx * ((c_im_max - c_im_min) / (double)n_samples);
    for(int re_idx = 0;re_idx < n_samples;re_idx++)
    {
      double c_re = c_re_min + (double)re_idx * ((c_re_max - c_re_min) / (double)n_samples);
      local_count += mandelbrot_point(c_re,c_im);
    }
  }
  td->count = local_count;
  printf("thread %2d: end at %.6f\n",td->thread_number,wall_time());
  return NULL;
}

int main(int argc,char **argv)
{
  int n_threads = (argc > 1) ? atoi(argv[1]) : 4;
  if(n_threads < 1)  n_threads = 1;
  if(n_threads > 64) n_threads = 64;

  pthread_t tid[n_threads];
  thread_data_t td[n_threads];

  double t0 = wall_time();
  int base = n_samples / n_threads;
  int rem  = n_samples % n_threads;
  int start = 0;
  for(int i = 0;i < n_threads;i++)
  {
    int width = base + (i < rem ? 1 : 0);
    td[i].thread_number = i;
    td[i].im_start = start;
    td[i].im_end   = start + width;
    start += width;
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
  printf("%ld -- %7.3f (n_threads=%d, horizontal stripes)\n",total,t1 - t0,n_threads);
  return 0;
}
