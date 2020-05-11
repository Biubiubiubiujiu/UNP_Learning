#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock > 0);

    char buf[1024];
    while(1) {
        scanf("%s", buf);
        sendto(sock, buf, 1023,0,  (struct sockaddr*)&serv, sizeof(serv));
        bzero(buf, sizeof(buf));
        socklen_t len = sizeof(serv);
        recvfrom(sock, buf, 1023, 0, (struct sockaddr*)&serv, &len);
        printf("recv:%s", buf);
    }

    close(sock);
    return 0;
}

