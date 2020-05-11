#define _GUN_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    assert(argc == 2);
    const char* file = argv[1];
    
    int filefd = open(file, O_RDWR|O_CREAT|O_TRUNC, 0666);
    int pipe_std[2];
    int ret = pipe(pipe_std);
    assert(ret != -1);

    ret = splice(STDIN_FILENO, NULL, pipe_std[1], NULL, 32768, SPLICE_F_MORE|SPLICE_F_MOVE);
    assert(ret != -1);

    int pipe_file[2];
    ret = pipe(pipe_file);
    assert(ret != -1);

    ret = tee(pipe_std[0], pipe_file[1], 32768, SPLICE_F_NONBLOCK);
    if(ret == -1) {
        printf("error: %s\n",strerror(errno));
        printf("errno: %d\n", errno);
        exit(1);
    }
    printf("tee ret: %d\n", ret);

    
    splice(pipe_file[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE|SPLICE_F_MOVE);
    splice(pipe_std[0], NULL, STDOUT_FILENO, NULL, 32768, SPLICE_F_MORE|SPLICE_F_MOVE);

    close(pipe_std[0]);
    close(pipe_std[1]);
    close(pipe_file[0]);
    close(pipe_file[1]);
    close(filefd);

    return 0;
}

