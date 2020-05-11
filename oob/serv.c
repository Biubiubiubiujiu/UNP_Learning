#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

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

    int connfd = accept(sock, (struct sockaddr*)&client, &clilen);
    if(connfd < 0) {
        printf("connection failed\n");
    }
    else {
        char buf[4096];

        bzero(buf, sizeof(buf));
        int res = recv(connfd, buf, sizeof(buf),0);
        printf("got %d bytes of data '%s'\n", res, buf);

        bzero(buf, sizeof(buf));
        res = recv(connfd, buf, sizeof(buf),MSG_OOB);
        printf("got %d bytes of oob data '%s'\n", res, buf);

        bzero(buf, sizeof(buf));
        res = recv(connfd, buf, sizeof(buf),0);
        printf("got %d bytes of data '%s'\n", res, buf);
        
        close(connfd);
    }
    close(sock);
    return 0;
}

