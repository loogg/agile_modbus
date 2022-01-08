#ifndef __TCP_H
#define __TCP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int tcp_listen(int port, int nb_connection);
int tcp_accept(int s);
void tcp_close(int s);
int tcp_send(int s, const uint8_t *buf, int length);
int tcp_receive(int s, uint8_t *buf, int bufsz, int timeout);
int tcp_flush(int s);
int tcp_connect(const char *ip, int port);

#ifdef __cplusplus
}
#endif

#endif
