#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static const int CONTROL_LEN = CMSG_LEN(sizeof(int));

void send_fd(int fd, int fd_to_send) {
    struct iovec iov[1];
    struct msghdr msg;
    char buf[1]; // 书里这里写0，被坑了

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    cmsghdr cm;
    cm.cmsg_len = CONTROL_LEN;
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(&cm) = fd_to_send;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    int ret = sendmsg(fd, &msg, 0);
    if(ret == -1) {
        printf("sendmsg error: %s\n", strerror(errno));
        return;
    }
}

int recv_fd(int fd) {
    struct iovec iov[1];
    struct msghdr msg;
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg(fd, &msg, 0);

    int fd_to_read = *(int *)CMSG_DATA(&cm);
    return fd_to_read;
}

int main(int argc, char* argv[])
{
    int pipefd[2];
    int fd_to_pass = 0;

    int ret = socketpair(PF_UNIX, SOCK_DGRAM, 0, pipefd);
    assert(ret != -1);

    pid_t pid = fork();
    assert(pid >= 0);

    if(pid == 0) {
        printf("run child now\n");
        close(pipefd[0]);
        fd_to_pass = open("test.txt", O_RDWR, 0666);
        send_fd(pipefd[1], (fd_to_pass > 0 ? fd_to_pass : 0));
        close(fd_to_pass);
        exit(0);
     }

    //printf("wait\n");
    //wait();
    //printf("wait back\n");
    close(pipefd[1]);
    printf("recv_fd called\n");
    fd_to_pass = recv_fd(pipefd[0]);
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    read(fd_to_pass, buf, 1024);
    printf("I got fd %d and data %s\n", fd_to_pass, buf);
    close(fd_to_pass);
    return 0;
}

