#include "tcp.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "tcp"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

int tcp_listen(int port, int nb_connection)
{
    int new_s;
    int enable;
    int flags;
    struct sockaddr_in addr;

    flags = SOCK_STREAM;

#ifdef SOCK_CLOEXEC
    flags |= SOCK_CLOEXEC;
#endif

    new_s = socket(PF_INET, flags, IPPROTO_TCP);
    if (new_s == -1) {
        return -1;
    }

    enable = 1;
    if (setsockopt(new_s, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&enable, sizeof(enable)) == -1) {
        close(new_s);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    /* If the modbus port is < to 1024, we need the setuid root. */
    addr.sin_port = htons(port);
    /* Listen any addresses */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(new_s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(new_s);
        return -1;
    }

    if (listen(new_s, nb_connection) == -1) {
        close(new_s);
        return -1;
    }

    flags = fcntl(new_s, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(new_s, F_SETFL, flags);

    flags = fcntl(new_s, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(new_s, F_SETFD, flags);

    return new_s;
}

int tcp_accept(int s)
{
    struct sockaddr_in addr;
    socklen_t addrlen;
    int new_s;

    addrlen = sizeof(addr);
#ifdef HAVE_ACCEPT4
    /* Inherit socket flags and use accept4 call */
    new_s = accept4(s, (struct sockaddr *)&addr, &addrlen, SOCK_CLOEXEC);
#else
    new_s = accept(s, (struct sockaddr *)&addr, &addrlen);
#endif

    if (new_s == -1) {
        return -1;
    }

    LOG_I("The client connection from %s is accepted", inet_ntoa(addr.sin_addr));

    return new_s;
}

void tcp_close(int s)
{
    if (s != -1) {
        shutdown(s, SHUT_RDWR);
        close(s);
    }
}

int tcp_send(int s, const uint8_t *buf, int length)
{
    return send(s, buf, length, MSG_NOSIGNAL);
}

int tcp_receive(int s, uint8_t *buf, int bufsz, int timeout)
{
    int len = 0;
    int rc = 0;
    fd_set readset, exceptset;
    struct timeval tv;

    while (bufsz > 0) {
        FD_ZERO(&readset);
        FD_ZERO(&exceptset);
        FD_SET(s, &readset);
        FD_SET(s, &exceptset);

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        rc = select(s + 1, &readset, NULL, &exceptset, &tv);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
        }

        if (rc <= 0)
            break;

        if (FD_ISSET(s, &exceptset)) {
            rc = -1;
            break;
        }

        rc = recv(s, buf + len, bufsz, MSG_DONTWAIT);
        if (rc < 0)
            break;
        if (rc == 0) {
            if (len == 0)
                rc = -1;
            break;
        }

        len += rc;
        bufsz -= rc;

        timeout = 50;
    }

    if (rc >= 0)
        rc = len;

    return rc;
}

int tcp_flush(int s)
{
    int rc;

    do {
        uint8_t devnull[100];

        if (s == -1)
            break;

        rc = recv(s, devnull, sizeof(devnull), MSG_DONTWAIT);

    } while (rc == 100);

    return 0;
}

int tcp_connect(const char *ip, int port)
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
        return -1;

    int option = 1;
    int rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const void *)&option, sizeof(int));
    if (rc == -1) {
        close(s);
        s = -1;
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    rc = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv, sizeof(struct timeval));
    if (rc == -1) {
        close(s);
        s = -1;
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    rc = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        close(s);
        s = -1;
        return -1;
    }

    return s;
}
