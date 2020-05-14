#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int timeout_connect(const char* ip, int port, int time_o) {
    struct sockaddr_in serv_address;
    serv_address.sin_family = AF_INET;
    serv_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_address.sin_addr);

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    struct timeval timeout;
    timeout.tv_sec = time_o;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    int ret = setsockopt(serv_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret != -1);

    int connfd = connect(serv_sock, (struct sockaddr*)&serv_address, sizeof(serv_address));
    if(connfd == -1) { // 超时失败。。。
        if(errno == EINPROGRESS) {
            printf("now time out\n");
            return -1;
        }
        printf("something else error happen: %s\n", strerror(errno));
        return -1;
    }
    printf("connect success\n");
    return 0;
}

int main(int argc, char* argv[])
{
    if(argc < 4) {
        printf("usage: %s ip port timeout\n", argv[0]);
        return 1;
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int time_out = atoi(argv[3]);
    int ret = timeout_connect(ip, port, time_out);
    return 0;
}

