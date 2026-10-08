#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in address;

    char buffer[BUFFER_SIZE];
    int buffer_used = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd,
             (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Server listening on 8080...\n");

    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0)
    {
        perror("accept");
        return 1;
    }

    printf("Client connected\n");

    while (1)
    {
        int n = recv(
            client_fd,
            buffer + buffer_used,
            BUFFER_SIZE - buffer_used - 1,
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

        buffer_used += n;

        buffer[buffer_used] = '\0';

        /*
         * Look for complete commands.
         */
        char *newline;

        while ((newline = strchr(buffer, '\n')) != NULL)
        {
            *newline = '\0';

            printf("Command: [%s]\n", buffer);

            if (strcmp(buffer, "PING") == 0)
            {
                send(client_fd, "PONG\n", 5, 0);
            }
            else
            {
                send(client_fd, "NOT_IMPLEMENTED\n", 16, 0);
            }

            /*
             * Remove processed command
             * from buffer.
             */
            int consumed = (newline - buffer) + 1;

            memmove(
                buffer,
                buffer + consumed,
                buffer_used - consumed
            );

            buffer_used -= consumed;

            buffer[buffer_used] = '\0';
        }
    }

    close(client_fd);
    close(server_fd);

    return 0;
}