//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P4, section 4
//
// Mandelbrot set count, distributed version (server/clients over TCP/IP sockets): server code
//
// usage: ./mandel_server port_number [rows_per_item]
//
// the 8193 values of re_idx are split into work items of rows_per_item consecutive rows (default 64)
// each client asks for a work item, computes the count of its rows, and sends it back (dynamic load balancing,
// the distributed equivalent of the "critical region" version of AAD_P2 and of schedule(dynamic) of AAD_P3)
//
// messages (same protocol style as server_clients.c):
//   client -> server: size 0                                  "give me work to do"
//                     size 3: [idx, work_id, count]            "here go the results of work item idx"
//   server -> client: size 4: [idx, work_id, re_first, re_last] "please do rows re_first <= re_idx < re_last"
//                     size 0                                   "no more work, please exit"
//
// fault tolerance: when there are no more work items to be done, work items that were assigned but whose
// results did not arrive yet are assigned again (oldest first), so a dead or slow client does not stall everything
//

#include <time.h>
#include <string.h>
#include "x_common.h"

#define n_samples  8193

static double wall_time(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  return (double)ts.tv_sec + 1.0e-9 * (double)ts.tv_nsec;
}


//
// set up the server socket to wait for connections; errors are fatal
//

static int setup_server(int port_number)
{
  struct sockaddr_in peer_address;
  int listen_fd,yes = 1;

  listen_fd = socket(AF_INET,SOCK_STREAM,0);
  if(listen_fd == -1)
  {
    perror("setup_server(): socket");
    exit(1);
  }
  (void)setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)); // allow an immediate restart on the same port
  peer_address.sin_family = AF_INET;
  peer_address.sin_addr.s_addr = htonl(INADDR_ANY);
  peer_address.sin_port = htons(port_number);
  if(bind(listen_fd,(struct sockaddr *)&peer_address,sizeof(peer_address)) == -1)
  {
    perror("setup_server(): bind");
    exit(1);
  }
  if(listen(listen_fd,16) == -1) // larger backlog than the example, because many clients may arrive at the same time
  {
    perror("setup_server(): listen");
    exit(1);
  }
  return listen_fd;
}


//
// return a new socket from a listening socket; errors are ignored
//

static int get_connection(int listen_fd,char connection_ipv4_address[32])
{
  struct sockaddr_in peer_address;
  socklen_t peer_length;
  int connection_fd;

  do
  {
    peer_length = (socklen_t)sizeof(peer_address);
    connection_fd = accept(listen_fd,(struct sockaddr *)&peer_address,&peer_length);
  }
  while(connection_fd < 0);
  if(connection_ipv4_address != NULL)
    sprintf(connection_ipv4_address,"%.30s",inet_ntoa(peer_address.sin_addr));
  return connection_fd;
}


//
// the server shuts down by itself after 15 minutes
//

static void alarm_signal_handler(int dummy)
{
  exit(1);
}


//
// the main server code
//

typedef enum { to_be_done,assigned,done } work_item_status_t;

typedef struct
{
  work_item_status_t status;
  unsigned int work_id;
  unsigned int re_first,re_last; // rows re_first <= re_idx < re_last
  unsigned int count;
  double assign_time;            // last time the work item was assigned (used to pick the oldest one to reassign)
  char done_by[32];              // IPv4 address of the client that sent the result
}
work_item_t;

int main(int argc,char **argv)
{
  unsigned int n_items,done_items,i,rows_per_item,n_reassigned,total_count;
  work_item_t *work_items;
  double t_start,t_end;
  int port_number;
  int listen_fd;

  //
  // command line stuff
  //
  if(argc < 2 || argc > 3)
  {
usage:
    fprintf(stderr,"usage: %s port_number [rows_per_item]  # 49152 <= port_number <= 65535, 1 <= rows_per_item <= %d\n",argv[0],n_samples);
    exit(1);
  }
  port_number = atoi(argv[1]);
  if(port_number < 49152 || port_number > 65535)
    goto usage;
  rows_per_item = (argc > 2) ? (unsigned int)atoi(argv[2]) : 64u;
  if(rows_per_item < 1u || rows_per_item > n_samples)
    goto usage;
  //
  // initialize the work items
  //
  n_items = (n_samples + rows_per_item - 1u) / rows_per_item;
  work_items = (work_item_t *)calloc((size_t)n_items,sizeof(work_item_t));
  if(work_items == NULL)
  {
    fprintf(stderr,"out of memory\n");
    exit(1);
  }
  srand((unsigned int)time(NULL));
  for(i = 0u;i < n_items;i++)
  {
    work_items[i].status = to_be_done;
    work_items[i].work_id = ((unsigned int)rand() << 16) ^ (unsigned int)rand() ^ i; // hard to guess, so stale/bogus results are rejected
    work_items[i].re_first = i * rows_per_item;
    work_items[i].re_last = (i + 1u) * rows_per_item;
    if(work_items[i].re_last > n_samples)
      work_items[i].re_last = n_samples;
  }
  done_items = n_reassigned = 0u;
  printf("%u work items of up to %u rows each\n",n_items,rows_per_item);
  //
  // arm the alarm
  //
  (void)signal(SIGALRM,alarm_signal_handler);
  (void)alarm(15u * 60u);
  //
  // talk with clients until all work is done
  //
  listen_fd = setup_server(port_number);
  t_start = -1.0;
  while(done_items < n_items)
  {
    char connection_ipv4_address[32];
    int connection_fd;
    int got_result;
    char *report;
    message_t m;

    connection_fd = get_connection(listen_fd,connection_ipv4_address);
    if(t_start < 0.0)
      t_start = wall_time(); // the clock starts when the first client shows up
    report = NULL;
    got_result = 0;
    // the client sends a message to either request more work or to send work completion data
    if(receive_message(connection_fd,&m) < 0)
    {
      report = "client->server communication error";
      goto terminate_connection;
    }
    switch(m.message_size)
    {
      case 0: // no done work, the client is just asking for some work to do
        break;
      case 3: // work completion data: data[0]=index, data[1]=work_id, data[2]=count
        if(m.data[0] >= n_items || m.data[1] != work_items[m.data[0]].work_id || work_items[m.data[0]].status == to_be_done)
        {
          report = "unexpected client message";
          goto terminate_connection;
        }
        if(work_items[m.data[0]].status == done)
        { // a reassigned work item was done twice; the results must agree
          if(work_items[m.data[0]].count != m.data[2])
            printf("  WARNING: work item %u done twice with different results (%u and %u)\n",m.data[0],work_items[m.data[0]].count,m.data[2]);
          break;
        }
        work_items[m.data[0]].count = m.data[2];
        work_items[m.data[0]].status = done;
        sprintf(work_items[m.data[0]].done_by,"%.30s",connection_ipv4_address);
        done_items++;
        got_result = 1;
        break;
      default:
        report = "unexpected message size";
        goto terminate_connection;
    }
    // give a work item to the client: first one not yet assigned; if none, the oldest assigned one
    for(i = 0u;i < n_items && work_items[i].status != to_be_done;i++)
      ;
    if(i == n_items)
    {
      unsigned int j;

      for(j = 0u;j < n_items;j++)
        if(work_items[j].status == assigned && (i == n_items || work_items[j].assign_time < work_items[i].assign_time))
          i = j;
      if(i < n_items)
        n_reassigned++;
    }
    if(i == n_items)
      m.message_size = 0u; // no more work
    else
    {
      m.message_size = 4u;
      m.data[0] = i;
      m.data[1] = work_items[i].work_id;
      m.data[2] = work_items[i].re_first;
      m.data[3] = work_items[i].re_last;
    }
    if(send_message(connection_fd,&m) < 0)
    {
      report = "server->client communication error";
      goto terminate_connection;
    }
    if(i < n_items)
    {
      work_items[i].status = assigned;
      work_items[i].assign_time = wall_time();
    }
terminate_connection:
    close_socket(connection_fd);
    if(report != NULL)
      printf("  %s: %s\n",connection_ipv4_address,report);
    else if(got_result != 0 && done_items % 16u == 0u)
      printf("  %u/%u work items done\n",done_items,n_items);
  }
  t_end = wall_time();
  close_socket(listen_fd);
  //
  // finish: sum the partial counts and report which clients did the work
  //
  total_count = 0u;
  for(i = 0u;i < n_items;i++)
    total_count += work_items[i].count;
  printf("work items done per client:\n");
  for(i = 0u;i < n_items;i++)
  {
    unsigned int j,k;

    for(j = 0u;j < i && strcmp(work_items[j].done_by,work_items[i].done_by) != 0;j++)
      ;
    if(j < i)
      continue; // already reported
    for(k = 0u,j = i;j < n_items;j++)
      if(strcmp(work_items[j].done_by,work_items[i].done_by) == 0)
        k++;
    printf("  %-16s %u\n",work_items[i].done_by,k);
  }
  printf("%u -- %7.3f (rows_per_item=%u, work_items=%u, reassigned=%u)\n",total_count,t_end - t_start,rows_per_item,n_items,n_reassigned);
  free(work_items);
  return 0;
}
