//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P4
//
// server/client example, split version: server code
//

#ifndef X_SERVER_H
#define X_SERVER_H

#include "x_common.h"

int setup_server(int port_number);
int get_connection(int listen_fd,char connection_ipv4_address[32]);
void alarm_signal_handler(int dummy);

#endif
