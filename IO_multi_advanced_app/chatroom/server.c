#define _GNU_SOURCE 1
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <stdio.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

struct client_data{
    sockaddr_in address;
    char* write_buf;
    char buf[BUFFER_SIZE];
};

int setnonbloking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

int main(int argc, char* argv[])
{
    if(argc < 3) {
        printf("usage: %s ip_address port\n",basename(argv[0]));
        return 1;
    }
    char* ip = argv[1];
    int port = atoi(argv[2]);
    sockaddr_in address;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == -1) {
        printf("bind error: %s\n", strerror(errno));
        return -1;
    }

    ret = listen(listenfd, 5);
    if(ret == -1) {
        printf("listen error: %s\n", strerror(errno));
        return -1;
    }

    client_data* users = new client_data[FD_LIMIT];
    pollfd fds[USER_LIMIT + 1];
    
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;
    
    int USER_COUNT = 0;

    while (1) {
        ret = poll(fds, USER_COUNT + 1, -1);
        if(ret == -1) {
            printf("poll error: %s\n", strerror(errno));
            return 1;
        }

        for(int i = 0; i < USER_COUNT + 1; i++) {
            int sockfd = fds[i].fd;
            if(sockfd == listenfd && (fds[i].revents & POLLIN)) {
                struct sockaddr_in client_address;
                socklen_t client_length = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_length);
                if(connfd < 0) {
                    printf("connfd error: %s\n", strerror(errno));
                    return 1;
                }

                if(USER_COUNT >= USER_LIMIT) {
                    const char* info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                USER_COUNT++;
                fds[USER_COUNT].fd = connfd;
                fds[USER_COUNT].events = POLLIN | POLLERR | POLLRDHUP;
                fds[USER_COUNT].revents = 0;
                users[connfd].address = client_address;
                setnonbloking(connfd);
                printf("comes a new user, now have %d user\n", USER_COUNT);
            }
            else if(fds[i].revents & POLLERR) {
                printf("get an error from %d\n", sockfd);
                char errors[100];
                memset(errors, 0, sizeof(errors));
                socklen_t length = sizeof(errors);
                if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            }
            else if(fds[i].revents & POLLRDHUP) {
                users[sockfd] = users[fds[USER_COUNT].fd];
                fds[i] = fds[USER_COUNT];
                USER_COUNT--;
                i--;
                close(sockfd);
                printf("one client left\n");
            }
            else if(fds[i].revents & POLLIN) {
                int connfd = sockfd;
                memset(users[connfd].buf, 0, sizeof(users[connfd].buf));
                ret = recv(connfd, users[connfd].buf, sizeof(users[connfd].buf), 0);
                if(ret < 0) {
                    if(errno != EAGAIN) {
                        close(connfd);
                        users[connfd] = users[fds[USER_COUNT].fd];
                        fds[i] = fds[USER_COUNT];
                        USER_COUNT--;
                        i--;
                    }
                }
                else if(ret == 0){

                }
                else {
                    for(int j = 1; j <= USER_COUNT; j++) {
                        if(fds[j].fd == connfd) {
                            continue;
                        }
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            }
            else if(fds[i].revents & POLLOUT) {
                int connfd = sockfd;
                if(!users[connfd].write_buf)
                    continue;
                ret = send(connfd, users[connfd].write_buf, sizeof(users[connfd]), 0);
                if(ret < 0) {
                    printf("send error: %s\n", strerror(errno));
                    close(connfd);
                    continue;
                }
                users[connfd].write_buf = NULL;
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }
    delete []users;
    close(listenfd);
    return 0;
}

