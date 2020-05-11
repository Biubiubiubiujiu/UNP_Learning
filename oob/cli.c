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

    if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        printf("connection failed\n");
    }
    else {
        const char* oob_data = "abc";
        const char* normal_data = "123";
        send(sock, normal_data, strlen(normal_data), 0);
        send(sock, oob_data, strlen(oob_data), MSG_OOB);
        send(sock, normal_data, strlen(normal_data), 0);
    }
    close(sock);
    return 0;
}

