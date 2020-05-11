#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

static bool stop = false;

static void handle_term(int sig) {
    stop = true;
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, handle_term);
    if(argc < 3){
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    sleep(20);
    struct sockaddr_in client;
    socklen_t clilen = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &clilen);
    if(connfd < 0) {
        printf("errno is: %d\n", errno);
    }
    else {
        char remote[clilen];
        printf("connectted with ip: %s and port : %d\n", 
               inet_ntop(AF_INET, &client.sin_addr.s_addr, remote, INET_ADDRSTRLEN), 
               ntohs(client.sin_port));
        //close(connfd);
    }

    close(sock);
    return 0;
}

