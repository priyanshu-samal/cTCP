#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

int main(void)
{
    int sockfd;

    struct sockaddr_in server_address;

    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &server_address.sin_addr
    );

    if (connect(
            sockfd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) < 0)
    {
        perror("connect");
        return 1;
    }

    printf("Connected to server\n");

    while (1)
    {
        printf("> ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;

        int length = strlen(buffer);

        if (send(sockfd, buffer, length, 0) < 0)
        {
            perror("send");
            break;
        }

        int n = recv(
            sockfd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (n == 0)
        {
            printf("Server disconnected\n");
            break;
        }

        if (n < 0)
        {
            perror("recv");
            break;
        }

        buffer[n] = '\0';

        printf("Server: %s", buffer);
    }

    close(sockfd);

    return 0;
}