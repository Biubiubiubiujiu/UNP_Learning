#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock > 0);

    int ret = connect(sock, (struct sockaddr*)&serv, sizeof(serv));
    assert(ret != -1);

    int fd[2];
    ret = pipe(fd);
    assert(ret != -1);

    int fd2[2];
    ret = pipe(fd2);
    assert(ret != -1);

    while(1) {
        int ret = splice(STDIN_FILENO, NULL, fd[1], NULL, 4096, SPLICE_F_MORE);
        if(ret == -1)
            break;

        ret = splice(fd[0], NULL, sock, NULL, 4096, SPLICE_F_MORE);
        if(ret == -1)
            break;
        
        //ret = splice(sock, NULL, fd2[1], NULL, 4096, 0);
        if(ret == -1)
            break;
        
        //ret =  splice(fd2[0], NULL, STDOUT_FILENO, NULL, 4096, 0);
        if(ret == -1)
            break;
    }
    close(fd2[0]);
    close(fd2[1]);
    close(fd[0]);
    close(fd[1]);

    close(sock);
    return 0;
}

