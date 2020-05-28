#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int a = 0;
int b = 0;
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;

void* another(void* arg) {
    pthread_mutex_lock(&mutex_b);
    printf("in child thread, got mutex b, waiting mutex a\n");
    sleep(5);
    b++;
    pthread_mutex_lock(&mutex_a);
    b += a++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    pthread_t pid;
    
    pthread_mutex_init(&mutex_a, NULL);
    pthread_mutex_init(&mutex_b, NULL);
    pthread_create(&pid, NULL, another, NULL);

    pthread_mutex_lock(&mutex_a);
    printf("in parent thread, got mutex a, waiting mutex b\n");
    sleep(5);
    a++;
    pthread_mutex_lock(&mutex_b);
    a += b++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);

    //主线程退出后进程会退出，导致其他线程也销毁掉，所以要用join等待其他线程
    pthread_join(pid, NULL);

    return 0;
}

