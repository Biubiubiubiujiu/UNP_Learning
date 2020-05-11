#include <stdio.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    assert(argc == 3);

    char* IP = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd > 0);

    int result = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(result != -1);

    result = listen(sockfd, 5);
    assert(result != -1);

    struct sockaddr_in sockclient;
    socklen_t socklen = sizeof(sockclient);
    int connfd = accept(sockfd, (struct sockaddr*)&sockclient, &socklen);
    if(connfd < 0){
        printf("errno is: %d\n", errno);
    }
    else {
        //close(STDOUT_FILENO);
        //dup(connfd);
        dup2(connfd, 1);
        printf("abcdabcd\n");
        close(connfd);
    }
    return 0;
}

