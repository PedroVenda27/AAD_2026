//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P4, section 4
//
// Mandelbrot set count, distributed version (server/clients over TCP/IP sockets): client code
//
// usage: ./mandel_client server-address port_number [n_threads]
//
// the client asks the server for a block of rows, counts the Mandelbrot points in those rows using
// n_threads OpenMP threads (default 1, i.e., sequential; AAD_P3 inside each machine), and sends the count back
//

#include <omp.h>
#include <time.h>
#include "x_common.h"

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


//
// the work: count the points of the rows re_first <= re_idx < re_last (same computation as MandelbrotCount.c)
//

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

static unsigned int mandelbrot_rows(int re_first,int re_last)
{
  unsigned int count = 0u;

# pragma omp parallel for schedule(dynamic) reduction(+:count)
  for(int re_idx = re_first;re_idx < re_last;re_idx++)
  {
    double c_re = c_re_min + (double)re_idx * ((c_re_max - c_re_min) / (double)n_samples);
    for(int im_idx = 0;im_idx < n_samples;im_idx++)
    {
      double c_im = c_im_min + (double)im_idx * ((c_im_max - c_im_min) / (double)n_samples);
      count += mandelbrot_point(c_re,c_im);
    }
  }
  return count;
}


//
// connect to the server; unlike x_client.c, a failure is not fatal: it returns -1 (the server may have already finished)
//

static int connect_to_server(char *ip_address,int port_number)
{
  struct sockaddr_in peer_address;
  int connection_fd;
  in_addr_t address;

  address = inet_addr(ip_address);
  if(address == (in_addr_t)-1)
  {
    fprintf(stderr,"connect_to_server(): bad IPv4 address %s\n",ip_address);
    exit(1);
  }
  connection_fd = socket(AF_INET,SOCK_STREAM,0);
  if(connection_fd == -1)
  {
    perror("connect_to_server(): socket");
    exit(1);
  }
  peer_address.sin_family = AF_INET;
  peer_address.sin_addr.s_addr = address;
  peer_address.sin_port = htons(port_number);
  if(connect(connection_fd,(struct sockaddr *)&peer_address,(socklen_t)sizeof(struct sockaddr)) == -1)
  {
    close(connection_fd);
    return -1;
  }
  return connection_fd;
}


//
// the main client code
//

int main(int argc,char **argv)
{
  unsigned int work_idx,work_id,re_first,re_last,count,valid,n_done;
  double t_start,t_work;
  char *server_address;
  int port_number,n_threads;

  //
  // command line stuff
  //
  if(argc < 3 || argc > 4)
  {
usage:
    fprintf(stderr,"usage: %s server-address port_number [n_threads]  # 49152 <= port_number <= 65535\n",argv[0]);
    exit(1);
  }
  server_address = argv[1];
  port_number = atoi(argv[2]);
  if(port_number < 49152 || port_number > 65535)
    goto usage;
  n_threads = (argc > 3) ? atoi(argv[3]) : 1;
  if(n_threads < 1 || n_threads > 64)
    goto usage;
  omp_set_num_threads(n_threads);
  //
  // talk with the server (on a loop, until there is no more work to be done)
  //
  work_idx = work_id = re_first = re_last = count = valid = n_done = 0u;
  t_start = wall_time();
  t_work = 0.0;
  for(;;)
  {
    int connection_fd;
    char *report;
    message_t m;

    connection_fd = connect_to_server(server_address,port_number);
    if(connection_fd < 0)
    {
      if(n_done == 0u)
      {
        perror("connect_to_server(): connect");
        exit(1);
      }
      printf("the server is gone (all work done by now)\n");
      break;
    }
    // request more work or send work completion data
    report = NULL;
    if(valid == 0u)
      m.message_size = 0u;
    else
    {
      m.message_size = 3u;
      m.data[0] = work_idx;
      m.data[1] = work_id;
      m.data[2] = count;
    }
    if(send_message(connection_fd,&m) < 0)
    {
      report = "client->server communication error";
      goto terminate_connection;
    }
    // get a new work item: data[0]=index, data[1]=work_id, data[2]=re_first, data[3]=re_last; size 0 means no more work
    if(receive_message(connection_fd,&m) < 0)
    {
      report = "server->client communication error";
      goto terminate_connection;
    }
    switch(m.message_size)
    {
      case 0: // no more work
        valid = 0u;
        break;
      case 4: // a new work item
        if(m.data[2] >= m.data[3] || m.data[3] > n_samples)
        {
          report = "bad work item";
          valid = 0u;
          break;
        }
        valid = 1u;
        work_idx = m.data[0];
        work_id = m.data[1];
        re_first = m.data[2];
        re_last = m.data[3];
        break;
      default:
        report = "unexpected message size";
        valid = 0u;
        break;
    }
terminate_connection:
    close_socket(connection_fd);
    if(report != NULL)
      printf("%s\n",report);
    // if requested (or on error), exit
    if(valid == 0u)
      break;
    // do the requested work
    double t0 = wall_time();
    count = mandelbrot_rows((int)re_first,(int)re_last);
    t_work += wall_time() - t0;
    n_done++;
  }
  //
  // finish
  //
  printf("work items done: %u, compute time %.3f s, total time %.3f s (n_threads=%d)\n",n_done,t_work,wall_time() - t_start,n_threads);
  return 0;
}
