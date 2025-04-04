#ifndef DN_H_
#define DN_H_

#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
} TcpSocket;

TcpSocket *tcp_socket_create(void);
bool tcp_socket_bind(TcpSocket *socket, int port);
bool tcp_socket_listen(TcpSocket *socket, int backlog);
TcpSocket *tcp_socket_accept(TcpSocket *socket);
bool tcp_socket_connect(TcpSocket *socket, const char *ipv4, int port);
ssize_t tcp_socket_send(TcpSocket *socket, const void *buffer, size_t size);
ssize_t tcp_socket_receive(TcpSocket *socket, void *buffer, size_t size);
void tcp_socket_close(TcpSocket *socket);
const char *tcp_get_error(void);

#endif // DN_H_

#ifdef DN_IMPLEMENTATION

TcpSocket *tcp_socket_create(void)
{
    TcpSocket *sock = malloc(sizeof(*sock));
    if (sock == NULL) {
        return NULL;
    }

    sock->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock->sockfd < 0) {
        free(sock);
        return NULL;
    }

    memset(&sock->addr, 0, sizeof(sock->addr));
    sock->addr.sin_family = AF_INET;

    return sock;
}

bool tcp_socket_bind(TcpSocket *socket, int port)
{
    socket->addr.sin_port = htons(port);
    socket->addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket->sockfd, (struct sockaddr*)&socket->addr, sizeof(socket->addr)) < 0) {
        return false;
    }

    return true;
}

bool tcp_socket_listen(TcpSocket *socket, int backlog)
{
    if (listen(socket->sockfd, backlog) < 0) {
        return false;
    }

    return true;
}

TcpSocket *tcp_socket_accept(TcpSocket *socket)
{
    TcpSocket *client_socket = tcp_socket_create();
    if (client_socket == NULL) {
        return NULL;
    }

    socklen_t addr_len = sizeof(client_socket->addr);
    client_socket->sockfd = accept(socket->sockfd, (struct sockaddr*)&client_socket->addr, &addr_len);
    if (client_socket->sockfd < 0) {
        free(client_socket);
        return NULL;
    }

    return client_socket;
}

bool tcp_socket_connect(TcpSocket *socket, const char *ipv4, int port)
{
    socket->addr.sin_port = htons(port);
    inet_pton(AF_INET, ipv4, &socket->addr.sin_addr);

    if (connect(socket->sockfd, (struct sockaddr*)&socket->addr, sizeof(socket->addr)) < 0) {
        return false;
    }

    return true;
}

ssize_t tcp_socket_send(TcpSocket *socket, const void *buffer, size_t size)
{
    return send(socket->sockfd, buffer, size, 0);
}

ssize_t tcp_socket_receive(TcpSocket *socket, void *buffer, size_t size)
{
    return recv(socket->sockfd, buffer, size, 0);
}

void tcp_socket_close(TcpSocket *socket)
{
    close(socket->sockfd);
    free(socket);
}

const char *tcp_get_error(void)
{
    return strerror(errno);
}

#endif // DN_IMPLEMENTATION
