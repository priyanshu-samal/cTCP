#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void)
{
    int sockfd;

    struct sockaddr_in server_address;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);

    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    connect(
        sockfd,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    );

    send(sockfd, "Hello", 5, 0);

    char buffer[1024];

    int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

    buffer[n] = '\0';

    printf("Server said: %s\n", buffer);

    close(sockfd);

    return 0;
}
