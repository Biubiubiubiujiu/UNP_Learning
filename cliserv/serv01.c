#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
int main()
{
    pid_t childpid;
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket error\n");
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = 6666;
    if(bind(listenfd,(struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind error\n");
        exit(1);
    }
    if(listen(listenfd, 5) == -1) {
        printf("listen error\n");
        exit(1);
    }
    for( ; ; ) {
        clilen = sizeof(cliaddr);
        if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1) {
            printf("accept error\n");
            exit(1);
        }
        childpid = fork();
        if(childpid == 0) {
            close(listenfd);
            
            ssize_t n;
            char buf[4096];
            for( ; ; ) {
                while((n = read(connfd, buf, sizeof(buf)) > 0)) {
                    write(connfd, buf, n);
                }
                if(n < 0 && errno == EINTR)
                    continue;
                else break;
            }
            exit(0);
        }
        close(connfd);
    }
    return 0;
}

