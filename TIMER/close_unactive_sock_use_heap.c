#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include "heap_timer.h"

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 1

static int pipefd[2];
static time_heap timer_hep(1);
static int epollfd = 0;

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

void timer_handler() {
    //printf("timer_handler\n");
    timer_hep.tick();
    alarm(TIMESLOT);
}

void cb_func(client_data* user_data) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    printf("close fd %d\n", user_data->sockfd);
}

int main(int argc, char* argv[])
{
    if(argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    //printf("epoll_creat: %s\n", strerror(errno));
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    addsig(SIGALRM);
    addsig(SIGTERM);

    bool stop_server = false;

    client_data* users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(TIMESLOT);

    while(!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(number < 0 && errno != EINTR) {
            printf("epollfd: %d\n", epollfd);
            printf("epoll failure: %s\n", strerror(errno));
            break;
        }
        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_length = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_length);
                assert(connfd != -1);

                addfd(epollfd, connfd);
                users[connfd].address = client_address;
                users[connfd].sockfd = connfd;

                heap_timer* timer = new heap_timer(3 * TIMESLOT);
                timer->cb_func = cb_func;
                timer->user_data = &users[connfd];
                timer_hep.add_timer(timer);

                users[connfd].timer = timer;
            }
            else if(sockfd == pipefd[0] && events[i].events & EPOLLIN) {
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if(ret == -1) {
                    printf("recv signal error: %s\n", strerror(errno));
                    continue;
                }
                else if(ret == 0) {
                    continue;
                }
                else {
                    for(int i = 0; i < ret; i++) {
                        switch(signals[i]) {
                        case SIGALRM:
                            timeout = true;
                            break;
                        case SIGTERM:
                            stop_server = true;
                        }
                    }
                }
            }
            else if(events[i].events & EPOLLIN) {
                memset(users[sockfd].buf, 0, sizeof(users[sockfd].buf));
                ret = recv(sockfd, users[sockfd].buf, sizeof(users[sockfd].buf), 0);
                printf("get %d bytes of data %s from %d\n", ret, users[sockfd].buf, sockfd);

                heap_timer* timer = users[sockfd].timer;
                if(ret < 0) {
                    if(errno != EAGAIN) {
                        cb_func(&users[sockfd]); // 关闭连接
                        if(timer)
                            timer_hep.del_timer(timer);
                    }
                }
                else if(ret == 0) {
                    cb_func(&users[sockfd]);
                    if(timer)
                        timer_hep.del_timer(timer);
                }
                else {
                    if(timer) {
                        printf("adjust timer once\n");
                        heap_timer* new_timer = new heap_timer(3 * TIMESLOT);
                        new_timer->cb_func = timer->cb_func;
                        //printf("new timer\' cb func :%x\n",new_timer->cb_func);
                        new_timer->user_data = timer->user_data;
                        users[sockfd].timer = new_timer;
                        timer_hep.del_timer(timer); // 并没有真正地删除，只是将回调函数置空
                        timer_hep.add_timer(new_timer);
                    }
                }
            }
            else {
                printf("something else happen\n");
            }
        }
        if(timeout) {
            timer_handler();
            timeout = false;
        }
    }

    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;
    return 0;
}

