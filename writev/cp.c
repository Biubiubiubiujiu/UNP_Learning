HTTP/1.1 200 OK
Content-Length: 910

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
    assert(argc == 4);
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock > 0);

    int ret = connect(sock, (struct sockaddr*)&serv, sizeof(serv));
    assert(ret != -1);

    int fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    assert(fd != -1);

    ret = dup2(fd, 1);
    assert(ret > 0);

    char buf[4096];
    while (read(sock, buf, 4096) > 0) {
        printf("%s", buf);
    }
    close(fd);
    close(sock);
    return 0;
}

