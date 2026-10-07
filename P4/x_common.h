//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P4
//
// server/client example, split version: code shared by the server and the client
//

#ifndef X_COMMON_H
#define X_COMMON_H

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


//
// all messages exchanged between the server and each client are always an array of unsigned integers
//

#define max_message_size 7u

typedef struct
{
  unsigned int message_size;
  unsigned int data[max_message_size];
}
message_t;


//
// write to and read from a socket (return 0 when all is well and a negative value on error)
//

int write_data(int socket_fd,void *data,size_t data_size);
int read_data(int socket_fd,void *data,size_t data_size);


//
// close a socket
//

void close_socket(int socket_fd);


//
// send and receive messages (takes care of the byte order)
//

int send_message(int socket_fd,message_t *m);
int receive_message(int socket_fd,message_t *m);

#endif
