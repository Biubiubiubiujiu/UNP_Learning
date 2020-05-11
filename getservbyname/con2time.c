#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    assert(argc == 2);
    char* host = argv[1];
    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);
    printf("host_name: %s\n", hostinfo->h_name);

    struct servent* servinfo0 = getservbyport(htons(80), "tcp");
    assert(servinfo0);
    printf("port 80 is %s\n", servinfo0->s_name);

    struct servent* servinfo1 = getservbyname("http", "tcp");
    assert(servinfo1);
    printf("http port is %d\n", ntohs(servinfo1->s_port));

    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    address.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd > 0);

    int res = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(res != -1);

    char buf[512];
    res = read(sockfd, buf, 512);
    assert(res > 0);
    buf[res] = 0;
    printf("the day time is: %s",buf);
    close(sockfd);

    return 0;
}

