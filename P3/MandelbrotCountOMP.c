//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P3, section 6
//
// Mandelbrot set count, OpenMP version
//
// usage: ./MandelbrotCountOMP [n_threads] [schedule] [chunk_size]
//   schedule: "static" (default) or "dynamic"
//   chunk_size: number of re_idx values per chunk handed to a thread (default 1)
//
// with schedule(runtime), the actual policy is decided at run time via
// omp_set_schedule(), so we can compare static vs dynamic without recompiling
// -- this is the OpenMP equivalent of the two approaches explored in AAD_P2:
//    static "vertical stripes" (section 7.1) vs the dynamic "critical region"
//    work-stealing scheme (section 7.3)
//

#include <omp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc,char **argv)
{
  int n_threads = (argc > 1) ? atoi(argv[1]) : 4;
  if(n_threads < 1)  n_threads = 1;
  if(n_threads > 64) n_threads = 64;

  const char *schedule_name = (argc > 2) ? argv[2] : "static";
  int chunk_size = (argc > 3) ? atoi(argv[3]) : 1;
  if(chunk_size < 1) chunk_size = 1;

  omp_sched_t kind = (strcmp(schedule_name,"dynamic") == 0) ? omp_sched_dynamic : omp_sched_static;
  omp_set_schedule(kind,chunk_size);
  omp_set_num_threads(n_threads);

  int count = 0;
  double t0 = wall_time();
# pragma omp parallel for schedule(runtime) reduction(+:count)
  for(int re_idx = 0;re_idx < n_samples;re_idx++)
  {
    double c_re = c_re_min + (double)re_idx * ((c_re_max - c_re_min) / (double)n_samples);
    int local_count = 0;
    for(int im_idx = 0;im_idx < n_samples;im_idx++)
    {
      double c_im = c_im_min + (double)im_idx * ((c_im_max - c_im_min) / (double)n_samples);
      local_count += mandelbrot_point(c_re,c_im);
    }
    count += local_count;
    if(re_idx % 512 == 0) // avoid flooding the output; just a handful of progress lines per thread
      printf("thread %2d: re_idx=%4d done at %.6f\n",omp_get_thread_num(),re_idx,wall_time());
  }
  double t1 = wall_time();
  printf("%d -- %7.3f (n_threads=%d, schedule=%s, chunk=%d)\n",
         count,t1 - t0,n_threads,schedule_name,chunk_size);
  return 0;
}
