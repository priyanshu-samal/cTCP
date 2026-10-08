#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>

#define BUFFER_SIZE 4096

void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];

    while (1)
    {
        int n = recv(
            client_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (n == 0)
        {
            printf("Client disconnected\n");
            break;
        }

        if (n < 0)
        {
            perror("recv");
            break;
        }

        buffer[n] = '\0';

        printf(
            "Client [%d]: %s",
            client_fd,
            buffer
        );

        if (send(client_fd, buffer, n, 0) < 0)
        {
            perror("send");
            break;
        }
    }

    close(client_fd);
}

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in address;

    server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(
            server_fd,
            (struct sockaddr *)&address,
            sizeof(address)
        ) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Server listening on port 8080...\n");

    /*
     * Parent continuously accepts clients.
     */
    while (1)
    {
        client_fd = accept(
            server_fd,
            NULL,
            NULL
        );

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        printf(
            "New client connected: fd=%d\n",
            client_fd
        );

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            close(client_fd);
            continue;
        }

        /*
         * Child process
         */
        if (pid == 0)
        {
            close(server_fd);

            handle_client(client_fd);

            exit(0);
        }

        /*
         * Parent process
         */
        close(client_fd);

        printf(
            "Created child process: pid=%d\n",
            pid
        );
    }

    close(server_fd);

    return 0;
}