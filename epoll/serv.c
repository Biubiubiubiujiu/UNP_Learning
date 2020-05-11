#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epfd, int fd, bool enable_et) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(enable_et)
        event.events |= EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void lt(epoll_event* events, int cnt, int epfd, int listenfd) {
    char buffer[BUFFER_SIZE];
    printf("lt count: %d\n", cnt);
    for(int i = 0; i < cnt; i++ ) {
        printf("i: %d\n", i);
        int fd = events[i].data.fd;
        if(fd == listenfd) {
            sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            int client_fd = accept(listenfd, (struct sockaddr*)&client_address, &client_len);
            addfd(epfd, client_fd, false);
        }
        else if(events[i].events & EPOLLIN){
            printf("event trigger once\n");
            int ret = recv(fd, buffer, sizeof(buffer), 0);
            if(ret <= 0) {
                close(fd);
                continue;
            }
            buffer[ret] = '\0';
            printf("get %d bytes of content: %s\n", ret, buffer);
        }
        else{
            printf("something else happened\n");
        }
    }
}


void et(epoll_event* events, int cnt, int epfd, int listenfd) {
    char buffer[BUFFER_SIZE];
    printf("et cnt %d\n", cnt);
    for(int i = 0; i < cnt; i++) {
        int fd = events[i].data.fd;
        if(fd == listenfd) {
            sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            int ret = accept(listenfd, (struct sockaddr*)&client_address, &client_len);
            addfd(epfd, ret, true);
        }
        else if(events[i].events & EPOLLIN) {
            printf("event trigger once\n");
            while(1) {
                memset(buffer, '\0', sizeof(buffer));
                int ret = recv(fd, buffer, sizeof(buffer), 0);
                if(ret < 0) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read laster\n");
                        break;
                    }
                    close(fd);
                    break;
                }
                else if(ret == 0) {
                    close(fd);
                }
                else{
                    printf("get %d bytes of content: %s\n", ret, buffer);
                }
            }
        }
        else {
            printf("something else happened\n");
        }
    }
}

int main(int argc,char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int host = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(host);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epfd = epoll_create(5);
    assert(epfd != -1);
    addfd(epfd, listenfd, true);

    while(1) {
        int ret = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0){
            printf("epoll failure\n");
            break;
        }
        lt(events, ret, epfd, listenfd);
        //et(events, ret, epfd, listenfd);
    }

    close(listenfd);
    return 0;
}

