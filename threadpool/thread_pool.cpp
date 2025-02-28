#include "thread_pool.h"


static void thread_pool_exit_handler(void *data);
static void *thread_pool_cycle(void *data);
static int_t thread_pool_init_default(thread_pool_t *tpp, char *name);

static uint_t       thread_pool_task_id;

static int debug = 0;

thread_pool_t* thread_pool_init()
{
    int             err;
    pthread_t       tid;    //线程的句柄
    uint_t          n;
    pthread_attr_t  attr;   //用来设置线程的属性
	thread_pool_t   *tp=NULL;

	tp = (thread_pool_t *)calloc( 1,sizeof(thread_pool_t));

	if(tp == NULL){
	    fprintf(stderr, "thread_pool_init: calloc failed!\n");
	}
	//初始化线程池中其他的数据变量
	thread_pool_init_default(tp, NULL);
	//初始化任务队列
    thread_pool_queue_init(&tp->queue);
	//创建互斥量
    if (thread_mutex_create(&tp->mtx) != T_OK) {
		free(tp);
        return NULL;
    }
	
	//创建条件变量
    if (thread_cond_create(&tp->cond) != T_OK) {
        (void) thread_mutex_destroy(&tp->mtx);
		free(tp);
        return NULL;
    }
    err = pthread_attr_init(&attr);
    if (err) {
        fprintf(stderr, "pthread_attr_init() failed, reason: %s\n",strerror(errno));
		free(tp);
        return NULL;
    }
	
	// 设置线程的属性为分离状态   （detached -> 分离 ）
    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err) {
        fprintf(stderr, "pthread_attr_setdetachstate() failed, reason: %s\n",strerror(errno));
		free(tp);
        return NULL;
    }
	
	//创建线程
    for (n = 0; n < tp->threads; n++) {
        err = pthread_create(&tid, &attr, thread_pool_cycle,  tp);
        if (err) {
            fprintf(stderr, "pthread_create() failed, reason: %s\n",strerror(errno));
			free(tp);
            return NULL;
        }
    }
	//销毁pthread_attr_t 变量
    (void) pthread_attr_destroy(&attr);

    return tp;
}


void thread_pool_destroy(thread_pool_t *tp)
{
    uint_t           n;
    thread_task_t    task;    
    volatile uint_t  lock;   //volatile 修饰 表示该变量的值可能会在任何时候被修改，因此需要特殊对待

    memset(&task,'\0', sizeof(thread_task_t));

    task.handler = thread_pool_exit_handler;
    task.ctx = (void *) &lock;

    for (n = 0; n < tp->threads; n++) {
        lock = 1;

        if (thread_task_post(tp, &task) != T_OK) {
            return;
        }

        while (lock) {
            sched_yield();  //让出 cpu 的使用权
        }

        //task.event.active = 0;
    }

    (void) thread_cond_destroy(&tp->cond);
    (void) thread_mutex_destroy(&tp->mtx);

	free(tp);
}

static void
thread_pool_exit_handler(void *data)   //线程销毁执行的任务
{
    uint_t *lock = ( uint_t *)data;

    *lock = 0;

    pthread_exit(0);   //结束线程
}

void 
thread_task_destroy(thread_task_t* task) {
    if (task) {
        free(task);
    }
}

thread_task_t *
thread_task_alloc( size_t size )
{
    thread_task_t  *task;

    task = (thread_task_t *)calloc(1,sizeof(thread_task_t) + size);
    if (task == NULL) {
        return NULL;
    }

    task->ctx = task + 1;

    return task;
}


//往线程池中投递任务
int_t
thread_task_post(thread_pool_t *tp, thread_task_t *task)
{
    if (thread_mutex_lock(&tp->mtx) != T_OK) {
        return T_ERROR;
    }

    if (tp->waiting >= tp->max_queue) {
        (void) thread_mutex_unlock(&tp->mtx);

        fprintf(stderr,"thread pool \"%s\" queue overflow: %ld tasks waiting\n",
                      tp->name, tp->waiting);
        return T_ERROR;
    }

    //task->event.active = 1;

    task->id = thread_pool_task_id++;
    task->next = NULL;
	
	//唤醒一个线程
    if (thread_cond_signal(&tp->cond) != T_OK) {
        (void) thread_mutex_unlock(&tp->mtx);
        return T_ERROR;
    }

    *tp->queue.last = task;   //将任务链到到任务队列的尾部
    tp->queue.last = &task->next;   //更新尾部节点的位置

    tp->waiting++;

    (void) thread_mutex_unlock(&tp->mtx);

    if(debug)fprintf(stderr,"task #%lu added to thread pool \"%s\"\n",
                   task->id, tp->name);

    return T_OK;
}


static void *
thread_pool_cycle( void *data )  //线程循环函数（回调）
{
    thread_pool_t *tp = (thread_pool_t *)data;

    int                 err ;
    thread_task_t       *task;

    if(debug)fprintf(stderr,"thread in pool \"%s\" started\n", tp->name);
  
    for ( ; ; ) {
        if (thread_mutex_lock(&tp->mtx) != T_OK) {
            return NULL;
        }
 
        tp->waiting--;

        while (tp->queue.first == NULL) {  //判断队列是否为空
            if (thread_cond_wait(&tp->cond, &tp->mtx)
                != T_OK)
            {
                (void) thread_mutex_unlock(&tp->mtx);
                return NULL;
            }
        }
		//移动头节点
        task = tp->queue.first;
        tp->queue.first = task->next;

        if (tp->queue.first == NULL) {  //判断是否达到尾部的下一个节点（NULL 空节点）
            tp->queue.last = &tp->queue.first;
        }
		
        if (thread_mutex_unlock(&tp->mtx) != T_OK) {
            return NULL;
        }
		
        if(debug) fprintf(stderr,"run task #%lu in thread pool \"%s\"\n",
                       task->id, tp->name);

        task->handler( task->ctx );   //执行任务 

        if(debug) fprintf(stderr,"complete task #%lu in thread pool \"%s\"\n",task->id, tp->name);

        thread_task_destroy(task);

        task->next = NULL;

        //notify 
    }
}

static int_t
thread_pool_init_default(thread_pool_t *tpp, char *name)
{
	if(tpp)
    {
        tpp->threads = DEFAULT_THREADS_NUM;
        tpp->max_queue = DEFAULT_QUEUE_NUM;
        //strdup 字符串拷贝 , 内部会调用malloc 进行内存分配,传入的参数不能为空
		tpp->name = strdup(name?name:"default");
        if(debug)fprintf(stderr,
                      "thread_pool_init, name: %s ,threads: %lu max_queue: %ld\n",
                      tpp->name, tpp->threads, tpp->max_queue);

        return T_OK;
    }

    return T_ERROR;
}




