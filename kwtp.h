/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*  Date:                                                      */
/*        2013-02-20                                           */
/*                                                             */
/*                                                             */
/*-===========================================================-*/

#ifndef __KWTP_H__
#define __KWTP_H__

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <asm/bug.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,15)
#define mutex semaphore
#define mutex_init(foo) init_MUTEX(foo)
#define mutex_lock(foo) down(foo)
#define mutex_unlock(foo) up(foo)
#else
#include <linux/mutex.h>
#endif

#define KW_THREAD_NAME 16
struct kw_thread_pool {
    struct list_head threads;
    struct mutex lock;
    int (*threadfn)(void *data);
    struct kw_thread *(*alloc)(void);
    void (*free)(struct kw_thread *kwt);
    char name[KW_THREAD_NAME];
    int threadnum;
};

struct kw_thread {
    struct kw_thread_pool *pool;
    struct list_head link;
    struct task_struct *task;
    int id; 
};

extern void kwtp_init(struct kw_thread_pool *kwtp,
        int (*threadfn)(void *data),
        struct kw_thread *(*alloc)(void),
        void (*free)(struct kw_thread *kwt),
        char *name);
extern int kwtp_run(struct kw_thread_pool *pool, int num);
#endif //__KWTP_H__
