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

int main(int argc,char* argv[])
{
    assert(argc == 4);
    const char* ip = argv[1];
    int host = atoi(argv[2]);
    const char* filename = argv[3];

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
        int file_fd = open(filename, O_RDONLY);
        assert(file_fd > 0);
        
        struct stat file_stat;
        int ret = fstat(file_fd, &file_stat);
        assert(ret != -1);

        ret = sendfile(connfd, file_fd, NULL, file_stat.st_size);
        assert(ret != -1);

        close(connfd);
    }
    close(sock);
    return 0;
}

