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

typedef enum {
    SOCKET_TCP,
    SOCKET_UDP,
} SocketType;

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    SocketType type;
} Socket;

Socket *socket_create(SocketType type);
bool socket_bind(Socket *socket, int port);
bool socket_listen(Socket *socket, int backlog);
Socket *socket_accept(Socket *socket);
bool socket_connect(Socket *socket, const char *ipv4, int port);
ssize_t socket_send(Socket *socket, const void *buffer, size_t size);
ssize_t socket_receive(Socket *socket, void *buffer, size_t size);
void socket_close(Socket *socket);
const char *socket_get_error(void);

#endif // DN_H_

#ifdef DN_IMPLEMENTATION

Socket *socket_create(SocketType type)
{
    Socket *sock = malloc(sizeof(*sock));
    if (sock == NULL) {
        return NULL;
    }

    int socket_type;
    switch (type) {
        case SOCKET_TCP: socket_type = SOCK_STREAM; break;
        case SOCKET_UDP: socket_type = SOCK_DGRAM; break;
        default: {
            free(sock);
            errno = EINVAL;
            return NULL;
        } break;
    }

    sock->sockfd = socket(AF_INET, socket_type, 0);
    if (sock->sockfd < 0) {
        free(sock);
        return NULL;
    }

    memset(&sock->addr, 0, sizeof(sock->addr));
    sock->addr.sin_family = AF_INET;

    return sock;
}

bool socket_bind(Socket *socket, int port)
{
    socket->addr.sin_port = htons(port);
    socket->addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket->sockfd, (struct sockaddr*)&socket->addr, sizeof(socket->addr)) < 0) {
        return false;
    }

    return true;
}

bool socket_listen(Socket *socket, int backlog)
{
    if (listen(socket->sockfd, backlog) < 0) {
        return false;
    }

    return true;
}

Socket *socket_accept(Socket *socket)
{
    Socket *client_socket = socket_create(socket->type);
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

bool socket_connect(Socket *socket, const char *ipv4, int port)
{
    socket->addr.sin_port = htons(port);
    inet_pton(AF_INET, ipv4, &socket->addr.sin_addr);

    if (connect(socket->sockfd, (struct sockaddr*)&socket->addr, sizeof(socket->addr)) < 0) {
        return false;
    }

    return true;
}

ssize_t socket_send(Socket *socket, const void *buffer, size_t size)
{
    return send(socket->sockfd, buffer, size, 0);
}

ssize_t socket_receive(Socket *socket, void *buffer, size_t size)
{
    return recv(socket->sockfd, buffer, size, 0);
}

void socket_close(Socket *socket)
{
    close(socket->sockfd);
    free(socket);
}

const char *socket_get_error(void)
{
    return strerror(errno);
}

#endif // DN_IMPLEMENTATION
