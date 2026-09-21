//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P2, section 7
//
// Mandelbrot set count, parallel version 4: producer/consumer with a single producer
// the producer pushes block indices (482x482 blocks, 289 total) onto a bounded queue;
// consumer threads pop a block, process it, and accumulate their own partial count
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

//
// bounded queue of block indices (single producer, multiple consumers)
//

#define queue_size  32

typedef struct
{
  pthread_mutex_t lock;
  pthread_cond_t  not_full;
  pthread_cond_t  not_empty;
  int  n_items;
  int  read_pos,write_pos;
  int  data[queue_size];
  int  producer_done; // set to 1 once the producer placed the last item
}
queue_t;

static queue_t q =
{
  .lock       = PTHREAD_MUTEX_INITIALIZER,
  .not_full   = PTHREAD_COND_INITIALIZER,
  .not_empty  = PTHREAD_COND_INITIALIZER,
  .n_items    = 0,
  .read_pos   = 0,
  .write_pos  = 0,
  .producer_done = 0
};

static void *producer_thread(void *arg)
{
  (void)arg;
  for(int block = 0;block < total_blocks;block++)
  {
    pthread_mutex_lock(&q.lock);
    while(q.n_items == queue_size)
      pthread_cond_wait(&q.not_full,&q.lock);
    q.data[q.write_pos] = block;
    q.write_pos = (q.write_pos + 1 < queue_size) ? q.write_pos + 1 : 0;
    q.n_items++;
    pthread_cond_signal(&q.not_empty); // wake one consumer
    pthread_mutex_unlock(&q.lock);
  }
  pthread_mutex_lock(&q.lock);
  q.producer_done = 1;
  pthread_cond_broadcast(&q.not_empty); // wake every consumer so they notice there is no more work
  pthread_mutex_unlock(&q.lock);
  printf("producer   : end at %.6f\n",wall_time());
  return NULL;
}

typedef struct
{
  int thread_number;
  long count;
  int blocks_done;
}
thread_data_t;

static void *consumer_thread(void *arg)
{
  thread_data_t *td = (thread_data_t *)arg;
  long local_count = 0;
  int blocks_done = 0;
  for(;;)
  {
    int block;
    pthread_mutex_lock(&q.lock);
    while(q.n_items == 0 && !q.producer_done)
      pthread_cond_wait(&q.not_empty,&q.lock);
    if(q.n_items == 0 && q.producer_done)
    { // no more work, and the queue is empty: done
      pthread_mutex_unlock(&q.lock);
      break;
    }
    block = q.data[q.read_pos];
    q.read_pos = (q.read_pos + 1 < queue_size) ? q.read_pos + 1 : 0;
    q.n_items--;
    pthread_cond_signal(&q.not_full); // wake the producer, if it was waiting
    pthread_mutex_unlock(&q.lock);

    blocks_done++;
    int block_row = block / n_blocks_per_dim;
    int block_col = block % n_blocks_per_dim;
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
  int n_consumers = (argc > 1) ? atoi(argv[1]) : 4;
  if(n_consumers < 1)  n_consumers = 1;
  if(n_consumers > 64) n_consumers = 64;

  pthread_t producer_tid;
  pthread_t consumer_tid[n_consumers];
  thread_data_t td[n_consumers];

  q.n_items = q.read_pos = q.write_pos = q.producer_done = 0;
  double t0 = wall_time();

  for(int i = 0;i < n_consumers;i++)
  {
    td[i].thread_number = i;
    if(pthread_create(&consumer_tid[i],NULL,consumer_thread,&td[i]) != 0)
    {
      fprintf(stderr,"pthread_create() failed for consumer idx=%d\n",i);
      exit(1);
    }
  }
  if(pthread_create(&producer_tid,NULL,producer_thread,NULL) != 0)
  {
    fprintf(stderr,"pthread_create() failed for producer\n");
    exit(1);
  }

  pthread_join(producer_tid,NULL);
  long total = 0;
  for(int i = 0;i < n_consumers;i++)
  {
    if(pthread_join(consumer_tid[i],NULL) != 0)
    {
      fprintf(stderr,"pthread_join() failed for consumer idx=%d\n",i);
      exit(1);
    }
    total += td[i].count;
  }
  double t1 = wall_time();
  printf("%ld -- %7.3f (n_consumers=%d, producer/consumer, %d blocks of %dx%d)\n",
         total,t1 - t0,n_consumers,total_blocks,block_size,block_size);
  return 0;
}
