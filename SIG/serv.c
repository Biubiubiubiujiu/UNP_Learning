/*
 * 统一事件源
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
static int pipefd[2];

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

// 信号处理函数
void sig_handler(int sig) {
    printf("sig_handler...\n");
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);// 为啥传入1个字节
    errno = save_errno;
}

void addsig(int sig) {
    printf("add sig:%d\n", sig);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    //__sigset_t oset;
    //sigprocmask(SIG_BLOCK, NULL, &oset);
    //printf("old set: ");
    //for(int i = 0; i < sizeof(oset.__val) ; i++) {
    //    printf("%d ", oset.__val[i]);
    //}
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[])
{
    if(argc <= 2) {
        printf("usage: %s ip_address port_numer\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);

    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);
    bool stop_server = false;

    while(!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        printf("epoll number: %d\n", number);
        if(number == -1) {
            printf("epoll err: %s\n", strerror(errno));
        }
        if(number == -1 && errno != EINTR) {
            printf("epoll failuer\n");
            return 1;
        }

        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_len = sizeof(client_len);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_len);
                addfd(epollfd, connfd);
            }
            else if((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                printf("SIG coming\n");
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                printf("recv %d bytes data: %d\n", ret, *(int*)signals);
                if(ret <= 0){
                    continue;
                }
                else {
                    for(int i = 0; i < ret; i++) {
                        switch(signals[i]) {
                        case SIGCHLD:
                        case SIGHUP:
                            printf("SIGCHLD or SIGHUP\n");
                            continue;
                        case SIGTERM:
                        case SIGINT:
                            printf("SIGTERM or SIGINT\n");
                            stop_server = true;
                        }
                    }
                }
            }
        }
    }
    printf("close fds\n");
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}

