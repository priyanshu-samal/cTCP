#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "hashmap.h"
#include "protocol.h"

#define PORT 8080
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    size_t used;
} Client;

static HashMap map;

static int send_all(int fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(fd, data + sent, length - sent, 0);

        if (n > 0) {
            sent += (size_t)n;
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}

static int execute_command(int fd, char *line)
{
    Command cmd;
    char response[BUFFER_SIZE];

    if (parse_command(line, &cmd) != 0) {
        return send_all(fd, "ERR invalid command\n", 20);
    }

    if (strcmp(cmd.argv[0], "PING") == 0 && cmd.argc == 1) {
        return send_all(fd, "PONG\n", 5);
    }

    if (strcmp(cmd.argv[0], "SET") == 0 && cmd.argc == 3) {
        hashmap_set(&map, cmd.argv[1], cmd.argv[2]);
        return send_all(fd, "OK\n", 3);
    }

    if (strcmp(cmd.argv[0], "GET") == 0 && cmd.argc == 2) {
        char *value = hashmap_get(&map, cmd.argv[1]);

        if (value == NULL)
            return send_all(fd, "(nil)\n", 6);

        size_t length = strlen(value);
        int result = send_all(fd, value, length);
        free(value);

        if (result < 0)
            return -1;

        return send_all(fd, "\n", 1);
    }

    if (strcmp(cmd.argv[0], "DEL") == 0 && cmd.argc == 2) {
        int deleted = hashmap_delete(&map, cmd.argv[1]);
        int length = snprintf(response, sizeof(response), "%d\n", deleted);

        return send_all(fd, response, (size_t)length);
    }

    if (strcmp(cmd.argv[0], "EXISTS") == 0 && cmd.argc == 2) {
        int exists = hashmap_exists(&map, cmd.argv[1]);
        int length = snprintf(response, sizeof(response), "%d\n", exists);

        return send_all(fd, response, (size_t)length);
    }

    return send_all(fd, "ERR unknown command or wrong arguments\n", 39);
}

static int make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(server_fd, 128) < 0) {
        perror("bind/listen");
        close(server_fd);
        return 1;
    }

    if (make_nonblocking(server_fd) < 0) {
        perror("fcntl");
        close(server_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    struct epoll_event event = {0};
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        perror("epoll_ctl");
        close(epoll_fd);
        close(server_fd);
        return 1;
    }

    hashmap_init(&map);

    /* This first version tracks client buffers by file descriptor. */
    Client *clients[65536] = {0};

    printf("CTRedis listening on port %d\n", PORT);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (count < 0) {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < count; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                while (1) {
                    int client_fd = accept(server_fd, NULL, NULL);

                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;

                        if (errno == EINTR)
                            continue;

                        perror("accept");
                        break;
                    }

                    if (client_fd >= 65536 ||
                        make_nonblocking(client_fd) < 0) {
                        close(client_fd);
                        continue;
                    }

                    Client *client = calloc(1, sizeof(Client));

                    if (client == NULL) {
                        close(client_fd);
                        continue;
                    }

                    client->fd = client_fd;
                    clients[client_fd] = client;

                    event.events = EPOLLIN | EPOLLRDHUP;
                    event.data.fd = client_fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                        perror("epoll_ctl client");
                        clients[client_fd] = NULL;
                        free(client);
                        close(client_fd);
                        continue;
                    }

                    printf("Client connected: fd=%d\n", client_fd);
                }
            } else {
                Client *client = (fd >= 0 && fd < 65536) ? clients[fd] : NULL;

                if (client == NULL)
                    continue;

                int close_client = 0;

                while (1) {
                    if (client->used == BUFFER_SIZE - 1) {
                        close_client = 1;
                        break;
                    }

                    ssize_t n = recv(fd,
                                     client->buffer + client->used,
                                     BUFFER_SIZE - 1 - client->used,
                                     0);

                    if (n > 0) {
                        client->used += (size_t)n;
                        client->buffer[client->used] = '\0';

                        char *newline;

                        while ((newline = memchr(client->buffer, '\n', client->used)) != NULL) {
                            size_t consumed = (size_t)(newline - client->buffer) + 1;
                            *newline = '\0';

                            if (execute_command(fd, client->buffer) < 0) {
                                close_client = 1;
                                break;
                            }

                            memmove(client->buffer,
                                    client->buffer + consumed,
                                    client->used - consumed);

                            client->used -= consumed;
                            client->buffer[client->used] = '\0';
                        }

                        if (close_client)
                            break;

                        continue;
                    }

                    if (n == 0) {
                        close_client = 1;
                        break;
                    }

                    if (errno == EINTR)
                        continue;

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;

                    perror("recv");
                    close_client = 1;
                    break;
                }

                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                    close_client = 1;

                if (close_client) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    free(client);
                    clients[fd] = NULL;
                    printf("Client disconnected: fd=%d\n", fd);
                }
            }
        }
    }

    hashmap_destroy(&map);
    close(epoll_fd);
    close(server_fd);

    return 0;
}