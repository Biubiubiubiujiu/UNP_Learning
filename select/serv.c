#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>

int main(int argc,char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int host = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(host);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock != -1);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t cli_len = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &cli_len);
    if(connfd < 0) {
        printf("accept errno: %d\n", errno);
    }
    else {
        char buf[4096];
        fd_set read_set;
        fd_set except_set;
        FD_ZERO(&read_set);
        FD_ZERO(&except_set);
        while(1) {
            memset(buf, '\0', sizeof(buf));
            FD_SET(connfd, &read_set);
            FD_SET(connfd, &except_set);
            ret = select(connfd + 1, &read_set, NULL, &except_set, NULL);
            if(ret <= 0){
                printf("selection failed\n");
                break;
            }
            if(FD_ISSET(connfd, &except_set)) {
                ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
                if(ret <= 0) {
                    printf("read oob <= 0\n");
                    break;
                }
                printf("get %d bytes of oob data: %s\n", ret, buf);
            }

            if(FD_ISSET(connfd, &read_set)) {
                ret = recv(connfd, buf, sizeof(buf) - 1, 0);
                if(ret <= 0) {
                    printf("read normal ret = %d\n", ret);
                    break;
                }
                printf("get %d bytes of normal data: %s\n", ret, buf);
            }
                    }
        close(connfd);
    }
    close(sock);
    return 0;
}

