#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

int main(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        size_t length = strlen(input);

        size_t sent = 0;
        while (sent < length) {
            ssize_t n = send(fd, input + sent, length - sent, 0);

            if (n <= 0) {
                perror("send");
                close(fd);
                return 1;
            }

            sent += (size_t)n;
        }

        /* Our server returns one newline-terminated response per command. */
        size_t used = 0;

        while (used < sizeof(response) - 1) {
            ssize_t n = recv(fd, response + used, 1, 0);

            if (n <= 0) {
                if (n < 0)
                    perror("recv");

                close(fd);
                return 1;
            }

            if (response[used++] == '\n')
                break;
        }

        response[used] = '\0';
        printf("%s", response);
    }

    close(fd);
    return 0;
}