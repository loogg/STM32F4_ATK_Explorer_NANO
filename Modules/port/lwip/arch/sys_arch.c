#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/stats.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/tcpip.h"
#include <rtthread.h>

#ifdef RT_USING_SMP
static struct rt_mutex _mutex = {0};
#else
static RT_DEFINE_SPINLOCK(_spinlock);
#endif

/*
 * Initialize the ethernetif layer and set network interface device up
 */
static void tcpip_init_done_callback(void *arg)
{
    rt_sem_release((rt_sem_t)arg);
}

/**
 * LwIP system initialization
 */
int lwip_system_init(void)
{
    rt_err_t rc;
    struct rt_semaphore done_sem;
    static rt_bool_t init_ok = RT_FALSE;

    if (init_ok)
    {
        rt_kprintf("lwip system already init.\n");
        return 0;
    }
#ifdef RT_USING_SMP
    rt_mutex_init(&_mutex, "sys_arch", RT_IPC_FLAG_FIFO);
#endif

    extern int eth_system_device_init_private(void);
    eth_system_device_init_private();

    /* set default netif to NULL */
    netif_default = RT_NULL;

    rc = rt_sem_init(&done_sem, "done", 0, RT_IPC_FLAG_FIFO);
    if (rc != RT_EOK)
    {
        LWIP_ASSERT("Failed to create semaphore", 0);

        return -1;
    }

    tcpip_init(tcpip_init_done_callback, (void *)&done_sem);

    /* waiting for initialization done */
    if (rt_sem_take(&done_sem, RT_WAITING_FOREVER) != RT_EOK)
    {
        rt_sem_detach(&done_sem);

        return -1;
    }
    rt_sem_detach(&done_sem);

    rt_kprintf("lwIP-%d.%d.%d initialized!\n", LWIP_VERSION_MAJOR, LWIP_VERSION_MINOR, LWIP_VERSION_REVISION);

    init_ok = RT_TRUE;

    return 0;
}
INIT_PREV_EXPORT(lwip_system_init);

void sys_init(void) { /* nothing on RT-Thread porting */ }

/* ====================== Sem ====================== */

/*
 * Create a new semaphore
 *
 * @return the operation status, ERR_OK on OK; others on error
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count) {
    static unsigned short counter = 0;
    char tname[RT_NAME_MAX];
    sys_sem_t tmpsem;

    RT_DEBUG_NOT_IN_INTERRUPT;

    rt_snprintf(tname, RT_NAME_MAX, "%s%d", SYS_LWIP_SEM_NAME, counter);
    counter++;

    tmpsem = rt_sem_create(tname, count, RT_IPC_FLAG_FIFO);
    if (tmpsem == RT_NULL) {
        SYS_STATS_INC(sem.err);
        return ERR_MEM;
    } else {
        *sem = tmpsem;
        SYS_STATS_INC_USED(sem);

        return ERR_OK;
    }
}

/*
 * Deallocate a semaphore
 */
void sys_sem_free(sys_sem_t *sem) {
    RT_DEBUG_NOT_IN_INTERRUPT;
    SYS_STATS_DEC(sem.used);
    rt_sem_delete(*sem);
}

/*
 * Signal a semaphore
 */
void sys_sem_signal(sys_sem_t *sem) { rt_sem_release(*sem); }

/*
 * Block the thread while waiting for the semaphore to be signaled
 *
 * @return If the timeout argument is non-zero, it will return the number of milliseconds
 *         spent waiting for the semaphore to be signaled; If the semaphore isn't signaled
 *         within the specified time, it will return SYS_ARCH_TIMEOUT; If the thread doesn't
 *         wait for the semaphore, it will return zero
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
    rt_err_t ret;
    s32_t t;
    u32_t tick;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* get the begin tick */
    tick = rt_tick_get();
    if (timeout == 0) {
        t = RT_WAITING_FOREVER;
    } else {
        /* convert msecond to os tick */
        if (timeout < (1000 / RT_TICK_PER_SECOND))
            t = 1;
        else
            t = timeout / (1000 / RT_TICK_PER_SECOND);
    }

    ret = rt_sem_take(*sem, t);

    if (ret == -RT_ETIMEOUT) {
        return SYS_ARCH_TIMEOUT;
    } else {
        if (ret == RT_EOK) ret = 1;
    }

    /* get elapse msecond */
    tick = rt_tick_get() - tick;

    /* convert tick to msecond */
    tick = tick * (1000 / RT_TICK_PER_SECOND);
    if (tick == 0) tick = 1;

    return tick;
}

#ifndef sys_sem_valid
/** Check if a semaphore is valid/allocated:
 *  return 1 for valid, 0 for invalid
 */
int sys_sem_valid(sys_sem_t *sem) {
    int ret = 0;

    if (*sem) ret = 1;

    return ret;
}
#endif

#ifndef sys_sem_set_invalid
/** Set a semaphore invalid so that sys_sem_valid returns 0
 */
void sys_sem_set_invalid(sys_sem_t *sem) { *sem = RT_NULL; }
#endif

/* ====================== Mutex ====================== */

/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex
 */
err_t sys_mutex_new(sys_mutex_t *mutex) {
    static unsigned short counter = 0;
    char tname[RT_NAME_MAX];
    sys_mutex_t tmpmutex;

    RT_DEBUG_NOT_IN_INTERRUPT;

    rt_snprintf(tname, RT_NAME_MAX, "%s%d", SYS_LWIP_MUTEX_NAME, counter);
    counter++;

    tmpmutex = rt_mutex_create(tname, RT_IPC_FLAG_PRIO);
    if (tmpmutex == RT_NULL) {
        SYS_STATS_INC(mutex.err);
        return ERR_MEM;
    } else {
        *mutex = tmpmutex;
        SYS_STATS_INC_USED(mutex);
        return ERR_OK;
    }
}

/** Lock a mutex
 * @param mutex the mutex to lock
 */
void sys_mutex_lock(sys_mutex_t *mutex) {
    RT_DEBUG_NOT_IN_INTERRUPT;
    rt_mutex_take(*mutex, RT_WAITING_FOREVER);
    return;
}

/** Unlock a mutex
 * @param mutex the mutex to unlock
 */
void sys_mutex_unlock(sys_mutex_t *mutex) { rt_mutex_release(*mutex); }

/** Delete a semaphore
 * @param mutex the mutex to delete
 */
void sys_mutex_free(sys_mutex_t *mutex) {
    RT_DEBUG_NOT_IN_INTERRUPT;
    SYS_STATS_DEC(mutex.used);
    rt_mutex_delete(*mutex);
}

#ifndef sys_mutex_valid
/** Check if a mutex is valid/allocated:
 *  return 1 for valid, 0 for invalid
 */
int sys_mutex_valid(sys_mutex_t *mutex) {
    int ret = 0;

    if (*mutex) ret = 1;

    return ret;
}
#endif

#ifndef sys_mutex_set_invalid
/** Set a mutex invalid so that sys_mutex_valid returns 0
 */
void sys_mutex_set_invalid(sys_mutex_t *mutex) { *mutex = RT_NULL; }
#endif

/* ====================== Mailbox ====================== */

/*
 * Create an empty mailbox for maximum "size" elements
 *
 * @return the operation status, ERR_OK on OK; others on error
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
    static unsigned short counter = 0;
    char tname[RT_NAME_MAX];
    sys_mbox_t tmpmbox;

    RT_DEBUG_NOT_IN_INTERRUPT;

    rt_snprintf(tname, RT_NAME_MAX, "%s%d", SYS_LWIP_MBOX_NAME, counter);
    counter++;

    tmpmbox = rt_mb_create(tname, size, RT_IPC_FLAG_FIFO);
    if (tmpmbox != RT_NULL) {
        *mbox = tmpmbox;
        SYS_STATS_INC_USED(mbox);

        return ERR_OK;
    }

    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
}

/*
 * Deallocate a mailbox
 */
void sys_mbox_free(sys_mbox_t *mbox) {
    RT_DEBUG_NOT_IN_INTERRUPT;
    SYS_STATS_DEC(mbox.used);
    rt_mb_delete(*mbox);
    return;
}

/** Post a message to an mbox - may not fail
 * -> blocks if full, only used from tasks not from ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
    RT_DEBUG_NOT_IN_INTERRUPT;
    rt_mb_send_wait(*mbox, (rt_ubase_t)msg, RT_WAITING_FOREVER);
    return;
}

/*
 * Try to post the "msg" to the mailbox
 *
 * @return return ERR_OK if the "msg" is posted, ERR_MEM if the mailbox is full
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
    if (rt_mb_send(*mbox, (rt_ubase_t)msg) == RT_EOK) {
        return ERR_OK;
    }

    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
}

#if (LWIP_VERSION_MAJOR * 100 + LWIP_VERSION_MINOR) >= 201 /* >= v2.1.0 */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg) { return sys_mbox_trypost(q, msg); }
#endif /* (LWIP_VERSION_MAJOR * 100 + LWIP_VERSION_MINOR) >= 201 */

/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return time (in milliseconds) waited for a message, may be 0 if not waited
           or SYS_ARCH_TIMEOUT on timeout
 *         The returned time has to be accurate to prevent timer jitter!
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
    rt_err_t ret;
    s32_t t;
    u32_t tick;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* get the begin tick */
    tick = rt_tick_get();

    if (timeout == 0) {
        t = RT_WAITING_FOREVER;
    } else {
        /* convirt msecond to os tick */
        if (timeout < (1000 / RT_TICK_PER_SECOND))
            t = 1;
        else
            t = timeout / (1000 / RT_TICK_PER_SECOND);
    }
    /*When the waiting msg is generated by the application through signaling mechanisms,
    only by using interruptible mode can the program be made runnable again*/
    ret = rt_mb_recv_interruptible(*mbox, (rt_ubase_t *)msg, t);
    if (ret != RT_EOK) {
        return SYS_ARCH_TIMEOUT;
    }

    /* get elapse msecond */
    tick = rt_tick_get() - tick;

    /* convert tick to msecond */
    tick = tick * (1000 / RT_TICK_PER_SECOND);
    if (tick == 0) tick = 1;

    return tick;
}

/**
 * @ingroup sys_mbox
 * This is similar to sys_arch_mbox_fetch, however if a message is not
 * present in the mailbox, it immediately returns with the code
 * SYS_MBOX_EMPTY. On success 0 is returned.
 * To allow for efficient implementations, this can be defined as a
 * function-like macro in sys_arch.h instead of a normal function. For
 * example, a naive implementation could be:
 * \#define sys_arch_mbox_tryfetch(mbox,msg) sys_arch_mbox_fetch(mbox,msg,1)
 * although this would introduce unnecessary delays.
 *
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {
    int ret;

    ret = rt_mb_recv(*mbox, (rt_ubase_t *)msg, 0);
    if (ret == -RT_ETIMEOUT) {
        return SYS_ARCH_TIMEOUT;
    } else {
        if (ret == RT_EOK) ret = 0;
    }

    return ret;
}

#ifndef sys_mbox_valid
/** Check if an mbox is valid/allocated:
 *  return 1 for valid, 0 for invalid
 */
int sys_mbox_valid(sys_mbox_t *mbox) {
    int ret = 0;

    if (*mbox) ret = 1;

    return ret;
}
#endif

#ifndef sys_mbox_set_invalid
/** Set an mbox invalid so that sys_mbox_valid returns 0
 */
void sys_mbox_set_invalid(sys_mbox_t *mbox) { *mbox = RT_NULL; }
#endif

/* ====================== System ====================== */

/*
 * Start a new thread named "name" with priority "prio" that will begin
 * its execution in the function "thread()". The "arg" argument will be
 * passed as an argument to the thread() function
 */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio) {
    rt_thread_t t;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* create thread */
    t = rt_thread_create(name, thread, arg, stacksize, prio, 20);
    RT_ASSERT(t != RT_NULL);

    /* startup thread */
    rt_thread_startup(t);

    return t;
}

#if SYS_LIGHTWEIGHT_PROT

sys_prot_t sys_arch_protect(void) {
#ifdef RT_USING_SMP
    rt_mutex_take(&_mutex, RT_WAITING_FOREVER);
    return 0;
#else
    rt_base_t level;
    level = rt_spin_lock_irqsave(&_spinlock);
    return level;
#endif
}

void sys_arch_unprotect(sys_prot_t pval) {
#ifdef RT_USING_SMP
    RT_UNUSED(pval);
    rt_mutex_release(&_mutex);
#else
    rt_spin_unlock_irqrestore(&_spinlock, pval);
#endif
}

#endif /* SYS_LIGHTWEIGHT_PROT */

void sys_arch_assert(const char *file, int line) {
    rt_kprintf("\nAssertion: %d in %s, thread %s\n", line, file, rt_thread_self()->parent.name);
    RT_ASSERT(0);
}

u32_t sys_jiffies(void) { return rt_tick_get(); }

u32_t sys_now(void) { return rt_tick_get_millisecond(); }

#if !defined(NO_SYS) || !NO_SYS

static rt_thread_t lwip_tcpip_thread = RT_NULL;

void sys_mark_tcpip_thread(void) { lwip_tcpip_thread = rt_thread_self(); }

void sys_check_core_locking(void) {
    RT_DEBUG_IN_THREAD_CONTEXT;

    if (lwip_tcpip_thread != RT_NULL) {
        rt_thread_t current_thread = rt_thread_self();

#if LWIP_TCPIP_CORE_LOCKING
        LWIP_ASSERT("Function called without core lock", lock_tcpip_core->owner == current_thread);
#else
        LWIP_ASSERT("Function called from wrong thread", current_thread == lwip_tcpip_thread);
#endif /* LWIP_TCPIP_CORE_LOCKING */
    }
}

#endif /* !NO_SYS || !NO_SYS */
