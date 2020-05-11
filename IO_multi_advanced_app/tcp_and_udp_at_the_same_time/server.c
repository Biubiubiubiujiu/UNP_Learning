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
#include <stdio.h>
#include <sys/epoll.h>

#define MAX_EVENT_NUNMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

int setnonbloking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonbloking(fd);
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

    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(udpfd > 0);
    ret = bind(udpfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUNMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);
    addfd(epollfd, udpfd);

    while(1){
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUNMBER, -1);
        if(number < 0) {
            printf("epoll failure\n");
            break;
        }
        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) {
                printf("new tcp client coming\n");
                struct sockaddr_in client_address;
                socklen_t client_length = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_length);
                addfd(epollfd, connfd);
            }
            else if(sockfd == udpfd){
                printf("new udp client coming\n");
                char buf[UDP_BUFFER_SIZE];
                memset(buf, 0, sizeof(buf));
                struct sockaddr_in client_address;
                socklen_t client_length = sizeof(client_address);
                ret = recvfrom(udpfd, buf, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_address, &client_length);
                if(ret > 0) {
                    sendto(udpfd, buf, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_address, client_length);
                }
            }
            else if(events[i].events & EPOLLIN) {
                printf("%d's data coming\n", sockfd);
                char buf[TCP_BUFFER_SIZE];
                while(1) {
                    memset(buf, 0, sizeof(buf));
                    ret = recv(sockfd, buf, TCP_BUFFER_SIZE - 1, 0);
                    if(ret < 0) {
                        if((errno == EAGAIN || errno == EWOULDBLOCK)){
                            printf("read later\n");
                            break;
                        }
                        printf("recv error: %s\n", strerror(errno));
                        close(events[i].data.fd);
                        break;
                    }
                    else if(ret == 0)
                        close(sockfd);
                    else{
                        send(sockfd, buf, ret, 0);
                        printf("recv data from tcp client %d:%s", events[i].data.fd, buf);
                    }
                }
            }
            else {
                printf("something else happened\n");
            }
        }
    }
    close(listenfd);
    return 0;
}

