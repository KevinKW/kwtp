/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*  Date:                                                      */
/*        2013-02-20                                           */
/*                                                             */
/*                                                             */
/*-===========================================================-*/

#include <kwtp.h>

#ifdef USE_KLOG
#define msg(logModule, logLevel, ...) klog(logModule, logLevel, __VA_ARGS__)
#else
#define msg(logModule, logLevel, ...) printk(__VA_ARGS__)
#endif


#ifdef PRINT_FUNCTION
#define kerr(fmt, args...) msg(LOG_ALWAYS, LOG_ERR, KERN_ERR "KWTP: (%s) " fmt, __FUNCTION__, ## args)
#else
#define kerr(fmt, args...) msg(LOG_ALWAYS, LOG_ERR, KERN_ERR "LDFS: " fmt, ## args)
#endif

void kwtp_init(struct kw_thread_pool *kwtp,
        int (*threadfn)(void *data),
        struct kw_thread *(*alloc)(void),
        void (*free)(struct kw_thread *kwt),
        char *name)
{
    INIT_LIST_HEAD(&kwtp->threads);
    mutex_init(&kwtp->lock);
    kwtp->threadfn = threadfn;
    kwtp->alloc = alloc;
    kwtp->free = free;
    snprintf(kwtp->name, KW_THREAD_NAME, "%s", name);
    kwtp->threadnum = 0;
}
EXPORT_SYMBOL(kwtp_init);

static struct kw_thread *kwt_alloc(struct kw_thread_pool *kwtp)
{
    struct kw_thread *kwt;

    if (kwtp->alloc) {
        kwt = kwtp->alloc();
    } else {
        kwt = kmalloc(sizeof(*kwt), GFP_KERNEL);
    }
    return kwt;
}

static void kwt_free(struct kw_thread_pool *kwtp, struct kw_thread *kwt)
{
    if (kwtp->free) {
        kwtp->free(kwt);
    } else {
        kfree(kwt);
    }
}

static void kwtp_stop(struct kw_thread_pool *kwtp, int num)
{
    struct kw_thread *kwt, *next;

    kwtp->threadnum -= num;
    list_for_each_entry_safe(kwt, next, &kwtp->threads, link) {
        if (kwt->id >= kwtp->threadnum) {
            list_del(&kwt->link);
            kthread_stop(kwt->task);
            kwt_free(kwtp, kwt);
        }
    }
}

static int kwtp_start(struct kw_thread_pool *kwtp, int num)
{
    struct kw_thread *kwt;
    int i;

    i = kwtp->threadnum;
    kwtp->threadnum += num;
    for (; i < kwtp->threadnum; i++) {
        kwt = kwt_alloc(kwtp);
        if (unlikely(!kwt)) {
            kerr("Create kthread %s:%d failed\n", kwtp->name, i);
            kwtp_stop(kwtp, num);
            return -ENOMEM;
        }
        INIT_LIST_HEAD(&kwt->link);
        kwt->task = kthread_create(kwtp->threadfn, kwt, "%s:%d", kwtp->name, i);
        if (unlikely(IS_ERR(kwt->task))) {
            kerr("Create kthread %s:%d failed\n", kwtp->name, i);
            kwt_free(kwtp, kwt);
            kwtp_stop(kwtp, num);
            return -ENOMEM;
        }
        kwt->id = i;
        list_add_tail(&kwt->link, &kwtp->threads);
        wake_up_process(kwt->task);
    }

    return 0;
}

int kwtp_run(struct kw_thread_pool *kwtp, int num)
{
    int ret = 0;

    mutex_lock(&kwtp->lock);
    if (kwtp->threadnum < num) {
        ret = kwtp_start(kwtp, (num - kwtp->threadnum));
    } else if (kwtp->threadnum > num) {
        kwtp_stop(kwtp, (kwtp->threadnum - num));
    }
    mutex_unlock(&kwtp->lock);

    return ret;
}
EXPORT_SYMBOL(kwtp_run);
