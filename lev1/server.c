#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in address;

    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 5);

    printf("Server listening on port 8080...\n");

    client_fd = accept(server_fd, NULL, NULL);

    printf("Client connected!\n");

    int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    buffer[n] = '\0';

    printf("Client said: %s\n", buffer);

    send(client_fd, "Hello back!", 11, 0);

    close(client_fd);
    close(server_fd);

    return 0;
}
