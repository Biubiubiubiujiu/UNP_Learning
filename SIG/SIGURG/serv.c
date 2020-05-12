#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define BUF_SIZE 1024

static int connfd;

void sig_urg(int sig) {
    int save_errno = errno;
    char buffer[BUF_SIZE];
    memset(buffer, '\0', BUF_SIZE);
    int ret = recv(connfd, buffer, BUF_SIZE - 1, MSG_OOB);
    printf("recv oob error: %s\n", strerror(errno));
    printf("got %d bytes of oob data '%s'\n", ret, buffer);
    errno = save_errno;
}

void addsig(int sig, void (*sig_handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[])
{
    if(argc < 2) {
        printf("usage: %s ip_address portnum\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock > 0);

    int res = bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));
    assert(res != -1);

    res = listen(sock, 5);
    assert(res != -1);

    struct sockaddr_in client;
    socklen_t clilen = sizeof(client);

    connfd = accept(sock, (struct sockaddr*)&client, &clilen);
    if(connfd < 0) {
        printf("connection failed\n");
    }
    else {
        addsig(SIGURG, sig_urg);
        fcntl(connfd, F_SETOWN, getpid());
        char buf[4096];
        while(1) {
            memset(buf, 0, sizeof(buf));
            int ret = recv(connfd, buf, BUF_SIZE - 1, 0);
            if(ret < 0) {
                printf("recv error: %d\n", strerror(errno));
                break;
            }
            if(ret == 0 ) {
                printf("EOF\n");
                break;
            }
            printf("get %d bytes normal data '%s'\n", ret, buf);
        }

        close(connfd);
    }
    close(sock);
    return 0;
}

