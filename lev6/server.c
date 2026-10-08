#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096


int make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
    {
        return -1;
    }

    return fcntl(
        fd,
        F_SETFL,
        flags | O_NONBLOCK
    );
}


int main(void)
{
    int server_fd;
    int epoll_fd;

    struct sockaddr_in address;

    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];


    /*
     * Create TCP socket
     */
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


    /*
     * Allow quick restart of server
     */
    int reuse = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );


    /*
     * Server address
     */
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;


    /*
     * Bind
     */
    if (bind(
            server_fd,
            (struct sockaddr *)&address,
            sizeof(address)
        ) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }


    /*
     * Listen
     */
    if (listen(server_fd, 128) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }


    /*
     * Make listening socket non-blocking
     */
    if (make_nonblocking(server_fd) < 0)
    {
        perror("nonblocking");
        close(server_fd);
        return 1;
    }


    /*
     * Create epoll instance
     */
    epoll_fd = epoll_create1(0);

    if (epoll_fd < 0)
    {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }


    /*
     * Add server socket to epoll
     */
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(
            epoll_fd,
            EPOLL_CTL_ADD,
            server_fd,
            &event
        ) < 0)
    {
        perror("epoll_ctl");
        close(epoll_fd);
        close(server_fd);
        return 1;
    }


    printf("Server listening on port %d\n", PORT);


    /*
     * Main event loop
     */
    while (1)
    {
        int n_events = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );


        if (n_events < 0)
        {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }


        /*
         * Process every ready socket
         */
        for (int i = 0; i < n_events; i++)
        {
            int fd = events[i].data.fd;


            /*
             * New client connection
             */
            if (fd == server_fd)
            {
                while (1)
                {
                    int client_fd = accept(
                        server_fd,
                        NULL,
                        NULL
                    );

                    if (client_fd < 0)
                    {
                        if (
                            errno == EAGAIN ||
                            errno == EWOULDBLOCK
                        )
                        {
                            break;
                        }

                        perror("accept");
                        break;
                    }


                    /*
                     * Make client non-blocking
                     */
                    if (make_nonblocking(client_fd) < 0)
                    {
                        perror("fcntl");
                        close(client_fd);
                        continue;
                    }


                    /*
                     * Add client to epoll
                     */
                    event.events = EPOLLIN;
                    event.data.fd = client_fd;

                    if (epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_ADD,
                            client_fd,
                            &event
                        ) < 0)
                    {
                        perror("epoll_ctl");
                        close(client_fd);
                        continue;
                    }


                    printf(
                        "Client connected: fd=%d\n",
                        client_fd
                    );
                }
            }


            /*
             * Existing client has data
             */
            else
            {
                char buffer[BUFFER_SIZE];


                int bytes = recv(
                    fd,
                    buffer,
                    sizeof(buffer),
                    0
                );


                /*
                 * Client disconnected
                 */
                if (bytes == 0)
                {
                    printf(
                        "Client disconnected: fd=%d\n",
                        fd
                    );

                    epoll_ctl(
                        epoll_fd,
                        EPOLL_CTL_DEL,
                        fd,
                        NULL
                    );

                    close(fd);
                }


                /*
                 * Error
                 */
                else if (bytes < 0)
                {
                    if (
                        errno != EAGAIN &&
                        errno != EWOULDBLOCK
                    )
                    {
                        perror("recv");

                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_DEL,
                            fd,
                            NULL
                        );

                        close(fd);
                    }
                }


                /*
                 * Data received
                 */
                else
                {
                    printf(
                        "Received %d bytes from fd=%d\n",
                        bytes,
                        fd
                    );


                    /*
                     * Echo data back
                     */
                    int sent = send(
                        fd,
                        buffer,
                        bytes,
                        0
                    );


                    if (sent < 0)
                    {
                        perror("send");
                    }
                }
            }
        }
    }


    close(epoll_fd);
    close(server_fd);

    return 0;
}