#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    char *IP = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, IP, &servaddr.sin_addr);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd > 0);

    int result = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    assert(result != -1);

    char buf[512];
    read(sockfd, buf, sizeof(buf));
    printf(buf);
    close(sockfd);
    return 0;
}

