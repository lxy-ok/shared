#ifndef _THREAD_POOL_H_INCLUDED_
#define _THREAD_POOL_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif  /*__cplusplus*/

#include "thread.h"

#define DEFAULT_THREADS_NUM 4
#define DEFAULT_QUEUE_NUM  65535

typedef unsigned long         atomic_uint_t;
typedef struct thread_task_s  thread_task_t;
typedef struct thread_pool_s  thread_pool_t;

struct thread_task_s {
    thread_task_t       *next;
    uint_t               id;
    void                *ctx;
    void               (*handler)(void *data);
};

typedef struct {
    thread_task_t        *first;
    thread_task_t        **last;
} thread_pool_queue_t;

#define thread_pool_queue_init(q)                                         \
    (q)->first = NULL;                                                    \
    (q)->last = &(q)->first


struct thread_pool_s {
    pthread_mutex_t        mtx;
    thread_pool_queue_t   queue;
    int_t                 waiting;
    pthread_cond_t         cond;

    char                  *name;
    uint_t                threads;
    int_t                 max_queue;
};

thread_task_t *thread_task_alloc(size_t size);
void thread_task_destroy(thread_task_t* task);

int_t thread_task_post(thread_pool_t *tp, thread_task_t *task);
thread_pool_t* thread_pool_init();

void thread_pool_destroy(thread_pool_t *tp);


#ifdef __cplusplus
}
#endif // __cplusplus



#endif /* _THREAD_POOL_H_INCLUDED_ */
