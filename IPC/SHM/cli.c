#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char* argv[])
{
    assert(argc == 3);
    int port = atoi(argv[2]);
    const char* ip = argv[1];

    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    if(connfd == -1) {
        printf("socket error: %s\n", strerror(errno));
        return 1;
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int ret = connect(connfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    //const char* msg = "hello";
    //ret = send(connfd, msg, sizeof(msg), 0);
    //printf("send %d byte data to server: %s\n", sizeof(msg), msg);

    int epollfd = epoll_create(5);
    addfd(epollfd, connfd);
    addfd(epollfd, STDIN_FILENO);
    epoll_event events[5];

    int stop_server = 0;
    while(!stop_server) {
        int number = epoll_wait(epollfd, events, 5, -1);
        
        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if(sockfd == connfd) {
                printf("connfd comming\n");
                char buf[4096];
                bzero(buf, sizeof(buf));
                ret = recv(connfd, buf, sizeof(buf), 0);
                if(ret < 0) {
                    printf("ercv error: %s\n", strerror(errno));
                    break;
                }
                if(ret == 0) {
                    stop_server = true;
                    printf("server had closed the connection\n");
                    continue;
                }
                printf("recv %d bytes data: %s", ret, buf);
            }
            else if(sockfd == STDIN_FILENO) {
                printf("STDIN coming\n");
                int pipefd[2];
                ret = pipe(pipefd);
                assert(ret != -1);
                ret = splice(STDIN_FILENO, 0, pipefd[1], 0, 4096, SPLICE_F_MORE);
                printf("splice copy data count: %d\n", ret);
                assert(ret != -1);
                ret = splice(pipefd[0], 0, connfd, 0, 4096, 0);
                printf("splice copy data count: %d\n", ret);
                assert(ret != -1);
                printf("send success\n");
                close(pipefd[1]);
                close(pipefd[0]);
            }
            else {
                printf("I don't konw what happen know\n");
            }
        }
    }
    close(connfd);
    close(epollfd);

    return 0;
}

