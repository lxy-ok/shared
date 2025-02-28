#include "thread.h"

int
thread_mutex_create(pthread_mutex_t *mtx)
{
    int            err;
    pthread_mutexattr_t  attr;   //互斥量的属性

    err = pthread_mutexattr_init(&attr);  //初始化一个互斥锁属性的对象
    if (err != 0) {
        fprintf(stderr, "pthread_mutexattr_init() failed, reason: %s\n",strerror(errno));
        return T_ERROR;
    }
	//PTHREAD_MUTEX_ERRORCHECK 检错锁。
	//当同一个线程试图多次获取同一个这种类型的锁时，会返回错误码 EDEADLK，而不是像普通锁那样默认允许重入。
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);   //设置互斥锁的类型
    if (err != 0) {
		fprintf(stderr, "pthread_mutexattr_settype(PTHREAD_MUTEX_ERRORCHECK) failed, reason: %s\n",strerror(errno));
        return T_ERROR;
    }

    err = pthread_mutex_init(mtx, &attr);   //初始化互斥量 , 并设置属性
    if (err != 0) {
        fprintf(stderr,"pthread_mutex_init() failed, reason: %s\n",strerror(errno));
        return T_ERROR;
    }

    err = pthread_mutexattr_destroy(&attr);   //销毁互斥量的属性
    if (err != 0) {
		fprintf(stderr,"pthread_mutexattr_destroy() failed, reason: %s\n",strerror(errno));
    }

    return T_OK;
}


int
thread_mutex_destroy(pthread_mutex_t *mtx)
{
    int  err;

    err = pthread_mutex_destroy(mtx);
    if (err != 0) {
        fprintf(stderr,"pthread_mutex_destroy() failed, reason: %s\n",strerror(errno));
        return T_ERROR;
    }

    return T_OK;
}



int
thread_mutex_lock(pthread_mutex_t *mtx)
{
    int  err;

    err = pthread_mutex_lock(mtx);
    if (err == 0) {
        return T_OK;
    }
	fprintf(stderr,"pthread_mutex_lock() failed, reason: %s\n",strerror(errno));

    return T_ERROR;
}


int
thread_mutex_unlock(pthread_mutex_t *mtx)
{
    int  err;

    err = pthread_mutex_unlock(mtx);

#if 0
    ngx_time_update();
#endif

    if (err == 0) {
        return T_OK;
    }
	
	fprintf(stderr,"pthread_mutex_unlock() failed, reason: %s\n",strerror(errno));
    return T_ERROR;
}
