#ifndef _DEMO_THREAD_H_INCLUDED_
#define _DEMO_THREAD_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif  /*__cplusplus*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

typedef intptr_t        int_t;
typedef uintptr_t       uint_t;

#define  T_OK          0
#define  T_ERROR      -1

int thread_mutex_create(pthread_mutex_t *mtx);  //创建互斥量
int thread_mutex_destroy(pthread_mutex_t *mtx); //销毁互斥量
int thread_mutex_lock(pthread_mutex_t *mtx);    //加锁
int thread_mutex_unlock(pthread_mutex_t *mtx);  //解锁


int thread_cond_create(pthread_cond_t *cond);   //创建条件变量
int thread_cond_destroy(pthread_cond_t *cond);  //销毁条件变量
int thread_cond_signal(pthread_cond_t *cond);   //唤醒一个等待特定条件变量的线程
int thread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx);  //等待条件变量的状态改变


#ifdef __cplusplus
}
#endif // __cplusplus


#endif /* _DEMO_THREAD_H_INCLUDED_ */






