#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define DN_IMPLEMENTATION
#include "dn.h"

#define PORT 6969
#define POOL_CAPACITY 16
#define BUFFER_CAPACITY 4096
#define USERNAME_CAPACITY 16

Socket *socket_pool[POOL_CAPACITY];
pthread_mutex_t pool_lock;

bool add_client(Socket *sock)
{
    bool added = false;
    pthread_mutex_lock(&pool_lock);
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        if (socket_pool[i] == NULL) {
            socket_pool[i] = sock;
            added = true;
            break;
        }
    }
    pthread_mutex_unlock(&pool_lock);

    return added;
}

bool remove_client(Socket *sock) {
    bool removed = false;
    pthread_mutex_lock(&pool_lock);
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        if (socket_pool[i] == sock) {
            socket_pool[i] = NULL;
            removed = true;
            break;
        }
    }
    pthread_mutex_unlock(&pool_lock);

    return removed;
}

void broadcast(const Socket *from, const char *msg, size_t msg_len)
{
    pthread_mutex_lock(&pool_lock);
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        Socket *r = socket_pool[i];
        if (r != NULL && r != from) {
            socket_send(r, msg, msg_len);
        }
    }
    pthread_mutex_unlock(&pool_lock);
}

void *handle_client(void *user_data)
{
    Socket *client = (Socket*) user_data;
    ssize_t received = 0;

    char buffer[BUFFER_CAPACITY];
    memset(buffer, 0, sizeof(buffer));

    char username[USERNAME_CAPACITY];
    size_t username_length = 0;
    memset(username, 0, sizeof(username));

    const char *username_prompt = "username:";
    size_t prompt_length = strlen(username_prompt);
    memcpy(buffer, "username: ", prompt_length);
    socket_send(client, buffer, prompt_length);
    received = socket_receive(client, username, sizeof(username));
    if (received < 0) {
        goto disconnect;
    }
    username_length = received;

    printf("INFO: Client login with username `%.*s`\n", (int)username_length,
            username);
    received = snprintf(buffer, sizeof(buffer), "[Server] `%.*s` joined the chat",
                        (int)username_length, username);
    broadcast(client, buffer, received);

    memcpy(buffer, username, username_length);
    buffer[username_length] = ':';
    buffer[username_length + 1] = ' ';
    size_t prefix_len = username_length + 2;

    size_t read_len = sizeof(buffer) - prefix_len;
    char *read_buf = buffer + prefix_len;

    while ((received = socket_receive(client, read_buf, read_len)) > 0) {
        printf("INFO: %.*s: %.*s\n", (int)username_length, username,
                (int)received, read_buf);
        broadcast(client, buffer, received + prefix_len);
    }

disconnect:
    socket_close(client);
    remove_client(client);

    printf("INFO: Client `%.*s` disconnected\n", (int)username_length,
            username);
    received = snprintf(buffer, sizeof(buffer), "[Server] `%.*s` left the chat",
            (int)username_length, username);
    broadcast(client, buffer, received);

    return NULL;
}

int main(void)
{
    Socket *server = socket_create(SOCKET_TCP);
    if (server == NULL) {
        fprintf(stderr, "ERROR: Could not create socket: %s\n",
                socket_get_error());
        return EXIT_FAILURE;
    }

    printf("INFO: Created socket\n");

    if (!socket_bind(server, PORT)) {
        fprintf(stderr, "ERROR: Could not bind socket: %s\n",
                socket_get_error());
        socket_close(server);
        return EXIT_FAILURE;
    }

    printf("INFO: Bind socket\n");

    if (!socket_listen(server, 10)) {
        fprintf(stderr, "ERROR: Could not listen on socket: %s\n",
                socket_get_error());
        socket_close(server);
        return EXIT_FAILURE;
    }

    printf("INFO: Listen socket\n");

    while (true) {
        Socket *client = socket_accept(server);
        if (client == NULL) {
            fprintf(stderr, "ERROR: Could not accept client %s\n",
                    socket_get_error());
            socket_close(client);
            continue;
        }

        if (!add_client(client)) {
            fprintf(stderr, "ERROR: Socket pool buffer is full (%d)\n",
                    POOL_CAPACITY);
            socket_close(client);
            continue;
        }

        // printf("INFO: Pool:\n");
        // pthread_mutex_lock(&pool_lock);
        // for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        //     printf("  %ld: %p\n", i, socket_pool[i]);
        // }
        // pthread_mutex_unlock(&pool_lock);
        // printf("INFO: End pool:\n");

        pthread_t thread;
        if (pthread_create(&thread, NULL, &handle_client, client) != 0) {
            fprintf(stderr, "ERROR: Could not create thread\n");
            socket_close(client);
            continue;
        }
        pthread_detach(thread);

        printf("INFO: New client connected\n");
    }

    socket_close(server);
    printf("INFO: Closed socket\n");

    return EXIT_SUCCESS;
}
