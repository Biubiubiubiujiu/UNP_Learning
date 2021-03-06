#pragma once
#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class tw_timer;

struct client_data{
    sockaddr_in address;
    int sockfd;
    char* buf[BUFFER_SIZE];
    tw_timer* timer;
};

class tw_timer
{
public:
    tw_timer(int rot, int ts, client_data* usr_data, void(*_cb_func)(client_data*)):
        next(NULL), prev(NULL), rotation(rot),time_slot(ts), user_data(usr_data),
        cb_func(_cb_func){}
    ~tw_timer() {}
public:
    int rotation;   // 记录定时器在时间轮转多少圈后生效
    int time_slot;  // 在时间轮上的哪个槽
    void (*cb_func)(client_data*);
    client_data* user_data;
    tw_timer* next;
    tw_timer* prev;
};

class time_wheel {
public:
    time_wheel(): cur_slot(0) {
        for(int i = 0 ; i < N; i++) {
            slots[i] = NULL;
        }
    }
    ~time_wheel() {
        for(int i = 0; i < N; i++) {
            tw_timer* tmp = slots[i];
            while(tmp) {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }
    tw_timer* add_timer(int timeout, client_data* usr_data, void (*cb_func)(client_data*)) {
        if(timeout < 0) {
            return NULL;
        }
        int ticks = 0;
        if(timeout < SI) {
            ticks = 1;
        }
        else {
            ticks = timeout / SI;
        }
        int rotation = ticks / N;
        int ts = (cur_slot + ticks % N) % N;
        tw_timer *timer = new tw_timer(rotation, ts, usr_data, cb_func);
        printf("add timer: rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
        if(slots[ts] == NULL) {
            slots[ts] = timer;
        }
        else {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    void del_timer(tw_timer* timer) {
        if(!timer)
            return;
        int ts = timer->time_slot;
        if(slots[ts] == timer) {
            slots[ts] = timer->next;
            if(slots[ts]){
                slots[ts]->prev = NULL;
            }
            delete timer;
        }
        else {
            timer->prev->next = timer->next;
            if(timer->next) {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }
    void tick() {
        tw_timer* tmp = slots[cur_slot];
        printf("current slot is %d \n", cur_slot);
        while(tmp) {
            printf("tick the timer once\n");
            if(tmp->rotation > 0) {
                tmp->rotation--;
                tmp = tmp->next;
            }
            else {
                printf("tmp->rotation = %d\n", tmp->rotation);
                tmp->cb_func(tmp->user_data);
                //printf("call back function return now\n");
                //printf("current slot is %d \n", cur_slot);
                //if(!slots[cur_slot]) {
                //    printf("slots[cur_slot] is NULL\n");
                //}
                if(tmp == slots[cur_slot]) {
                    printf("delete slots[%d] header\n", cur_slot);
                    slots[cur_slot] = tmp->next;
                    if(slots[cur_slot]){
                        slots[cur_slot]->prev = NULL;
                    }
                    delete tmp;
                    tmp = slots[cur_slot];
                }
                else {
                    printf("delete one middle timer in slots[%d]\n", cur_slot);
                    //if(tmp == NULL) {
                    //    printf("but the timer already\n");
                    //}
                    tmp->prev->next = tmp->next;
                    if(tmp->next) {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;
    }
private:
    static const int N = 60; // 时间轮上槽的数目
    static const int SI = 1; // 每1s转动一次 
    tw_timer* slots[N]; 
    int cur_slot;   //时间轮当前槽
};

#endif

