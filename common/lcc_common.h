#ifndef __LCC_COMMON_H__
#define __LCC_COMMON_H__

#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>

#define		debug(fmt, ...)			printf((fmt), ##__VA_ARGS__)
#define CHECK(cond, fmt, ...)       do{\
     if (!(cond)) {\
         printf("%s line: %d fun: %s\n\t"\
             "\033[1;31;40m check \"%s\" failed, \033[0m", __FILE__, __LINE__, __FUNCTION__, #cond);\
         printf(fmt, ##__VA_ARGS__); fflush(stdout);\
         raise(9);\
     }}while(0)

static pthread_mutex_t s_mutex_lock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()      pthread_mutex_lock(&s_mutex_lock)
#define UNLOCK()    pthread_mutex_unlock(&s_mutex_lock)

/** 统计当前存活线程的数目 */
static int s_thread_cnt = 0;
#define     threads_cnt_inc()   do{LOCK();s_thread_cnt++;UNLOCK();}while(0)
#define     threads_cnt_dec()   do{LOCK();s_thread_cnt--;UNLOCK();}(0)
static int threads_cnt()   
{
    LOCK();
    int ret = s_thread_cnt;
    UNLOCK();
    return ret;
}

static long get_cur_ms()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return (long)(tv.tv_sec*1000 + tv.tv_usec/1000);
}

#define lcc_tick_start() long __lcc_start_ms=get_cur_ms()
#define lcc_tick_end() debug("\t lcc_tick: %s spend %ld ms\n", __FUNCTION__, get_cur_ms()-__lcc_start_ms);

static long get_cur_us()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return (long)(tv.tv_sec*1000000UL + tv.tv_usec);
}

/** 用于判断线程是否已经创建完毕，防止地址传的参数被修改 */
static int s_create_thread_flag = 0;
static void create_thread_start(int sock)
{
    struct timeval timeout={3,0};//3s
    if (0!=setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))) {
        debug("set sock %d opt failed, %m\n", sock);
    }
    if (0!=setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))) {
        debug("set sock %d opt failed, %m\n", sock);
    }
    //debug("create thread for sock %d\n", sock);
    LOCK();
    s_create_thread_flag = 0;
    UNLOCK();
}

static void thread_created(int sock)
{
    LOCK();
    s_create_thread_flag = 1;
    UNLOCK();
    //debug("thread for sock %d already created\n", sock);
}
static int is_thread_created()
{
    LOCK();
    int ret = s_create_thread_flag;
    UNLOCK();
    return ret;
}

#endif
