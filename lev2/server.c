#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in address;
    char buffer[1024];

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    // Server address
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to port 8080
    if (bind(server_fd,
             (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind");
        return 1;
    }

    // Start listening
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Server listening on port 8080...\n");

    // Wait for client
    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0)
    {
        perror("accept");
        return 1;
    }

    printf("Client connected!\n");

    // Echo loop
    while (1)
    {
        int n = recv(client_fd, buffer, sizeof(buffer), 0);

        // Client closed connection
        if (n == 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        // Error
        if (n < 0)
        {
            perror("recv");
            break;
        }

        printf("Received %d bytes\n", n);

        // Echo the data back
        int sent = send(client_fd, buffer, n, 0);

        if (sent < 0)
        {
            perror("send");
            break;
        }

        printf("Sent %d bytes\n", sent);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
