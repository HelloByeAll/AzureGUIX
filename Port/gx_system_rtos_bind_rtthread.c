/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** GUIX Component                                                        */
/**                                                                       */
/**   System Management (System)                                          */
/**                                                                       */
/**************************************************************************/

//#define GX_SOURCE_CODE

/* Include necessary system files.  */

#include "gx_api.h"
#include "gx_system.h"
#include "gx_system_rtos_bind.h"


#ifndef GX_THREADX_BINDING

/* This is an example of a generic RTOS binding. We have #ifdefed this code out
   of the GUIX library build, so that the user attempting to port to a new RTOS
   will intentionally observe a series of linker errors. These functions need to
   be implemented in a way that works correctly with the target RTOS.
*/

#if defined(GUIX_BINDING_RT_THREAD)

#include "rtthread.h"

#ifndef OS_CFG_TICK_RATE_HZ
#define OS_CFG_TICK_RATE_HZ 1000
#endif

#define GX_TIMER_TASK_PRIORITY (GX_SYSTEM_THREAD_PRIORITY + 1)
#define GX_TIMER_TASK_STACK_SIZE 512

void guix_timer_task_entry(void *);

/* a few global status variables */
GX_BOOL guix_timer_event_pending;

/* define stack space for GUIX task */
char guix_task_stack[GX_THREAD_STACK_SIZE];

/* define stack space for timer task */
char guix_timer_task_stack[GX_TIMER_TASK_STACK_SIZE];

/* define semaphore for lock/unlock macros */
struct rt_mutex guix_system_lock_mutex;


struct rt_thread guix_tcb;
struct rt_thread guix_timer_tcb;

/* a custom event structure, to hold event and linked list */

typedef struct guix_linked_event_struct
{
    GX_EVENT event_data;                   /* the GUIX event structure */
    struct guix_linked_event_struct *next; /* add link member          */
} GUIX_LINKED_EVENT;

/* a custom fifo event queue structure */

typedef struct guix_event_queue_struct
{
    GUIX_LINKED_EVENT *first;    /* first (oldest) event in fifo */
    GUIX_LINKED_EVENT *last;     /* last (newest) event in fifo  */
    GUIX_LINKED_EVENT *free;     /* linked list of free events   */
    GUIX_LINKED_EVENT *free_end; /* end of the free list         */
    struct rt_semaphore count_sem; /* number of queued events      */
    struct rt_mutex lock;        /* lock to protect queue update */
} GUIX_EVENT_QUEUE;

/* an array of GX_EVENTs used to implement fifo queue */

GUIX_LINKED_EVENT guix_event_array[GX_MAX_QUEUE_EVENTS];
GUIX_EVENT_QUEUE guix_event_queue;

/* rtos initialize: perform any setup that needs to be done before the GUIX task runs here */
VOID gx_generic_rtos_initialize(VOID)
{
    int Loop;
    GUIX_LINKED_EVENT *current;

    guix_timer_event_pending = GX_FALSE;
    rt_mutex_init(&guix_system_lock_mutex, "gx_system_lock", RT_IPC_FLAG_FIFO);

    /* initialize a custom fifo queue structure */

    guix_event_queue.first = GX_NULL;
    guix_event_queue.last = GX_NULL;
    guix_event_queue.free = guix_event_array;

    current = guix_event_queue.free;

    /* link all the structures together */
    for (Loop = 0; Loop < GX_MAX_QUEUE_EVENTS - 1; Loop++)
    {
        current->next = (current + 1);
        current = current->next;
    }
    current->next = GX_NULL; /* terminate the list */
    guix_event_queue.free_end = current;

    /* create mutex for lock access to this queue */
    rt_mutex_init(&guix_event_queue.lock, "gx_queue_lock", RT_IPC_FLAG_FIFO);

    /* create counting semaphore to track queue entries */
    rt_sem_init(&guix_event_queue.count_sem, "gx_queue_count", 0, RT_IPC_FLAG_FIFO);
}

VOID (*gx_system_thread_entry)(ULONG);

// A small shell function to convert the void * arg expected by uC/OS to
// a ULONG parameter expected by GUIX thread entry
void gx_system_thread_entry_shell(void *parg)
{
    gx_system_thread_entry((ULONG)parg);
}

/* thread_start: start the GUIX thread running. */
UINT gx_generic_thread_start(VOID (*guix_thread_entry)(ULONG))
{

    /* save the GUIX system thread entry function pointer */
    gx_system_thread_entry = guix_thread_entry;

    /* create the main GUIX task */

    rt_thread_init(&guix_tcb, "guix_task",
                   gx_system_thread_entry_shell,
                   GX_NULL,
                   &guix_task_stack[0],
                   GX_THREAD_STACK_SIZE,
                   GX_SYSTEM_THREAD_PRIORITY,
                   20);

    /* create a simple task to generate timer events into GUIX */
    rt_thread_init(&guix_timer_tcb, "guix_timer_task",
                   guix_timer_task_entry,
                   GX_NULL,
                   &guix_timer_task_stack[0],
                   GX_TIMER_TASK_STACK_SIZE,
                   GX_TIMER_TASK_PRIORITY,
                   20);

    rt_thread_startup(&guix_tcb);
    rt_thread_startup(&guix_timer_tcb);
    /* suspend the timer task until needed */
    rt_thread_suspend(&guix_timer_tcb);
    return GX_SUCCESS;
}

/* event_post: push an event into the fifo event queue */
UINT gx_generic_event_post(GX_EVENT *event_ptr)
{
    GUIX_LINKED_EVENT *linked_event;

    /* lock down the guix event queue */
    rt_mutex_take(&guix_event_queue.lock, RT_WAITING_FOREVER);

    /* grab a free event slot */
    if (!guix_event_queue.free)
    {
        /* there are no free events, return failure */
        rt_mutex_release(&guix_event_queue.lock);
        return GX_FAILURE;
    }

    linked_event = guix_event_queue.free;
    guix_event_queue.free = guix_event_queue.free->next;

    if (!guix_event_queue.free)
    {
        guix_event_queue.free_end = GX_NULL;
    }

    /* copy the event data into this slot */
    linked_event->event_data = *event_ptr;
    linked_event->next = GX_NULL;

    /* insert this event into fifo queue */
    if (guix_event_queue.last)
    {
        guix_event_queue.last->next = linked_event;
    }
    else
    {
        guix_event_queue.first = linked_event;
    }
    guix_event_queue.last = linked_event;

    /* Unlock the guix queue */
    rt_mutex_release(&guix_event_queue.lock);

    /* increment event count */
    rt_sem_release(&guix_event_queue.count_sem);
    return GX_SUCCESS;
}

/* event_fold: update existing matching event, otherwise post new event */
UINT gx_generic_event_fold(GX_EVENT *event_ptr)
{
    GUIX_LINKED_EVENT *pTest;

    /* Lock down the guix queue */

    rt_mutex_take(&guix_event_queue.lock, RT_WAITING_FOREVER);

    // see if the same event is already in the queue:
    pTest = guix_event_queue.first;

    while (pTest)
    {
        if (pTest->event_data.gx_event_type == event_ptr->gx_event_type)
        {
            /* found matching event, update it and return */
            pTest->event_data.gx_event_payload.gx_event_ulongdata = event_ptr->gx_event_payload.gx_event_ulongdata;
            rt_mutex_release(&guix_event_queue.lock);
            return GX_SUCCESS;
        }
        pTest = pTest->next;
    }

    /* did not find a match, push new event */

    rt_mutex_release(&guix_event_queue.lock);
    gx_generic_event_post(event_ptr);
    return GX_SUCCESS;
}

/* event_pop: pop oldest event from fifo queue, block if wait and no events exist */
UINT gx_generic_event_pop(GX_EVENT *put_event, GX_BOOL wait)
{
    if (!wait)
    {
        if (guix_event_queue.first == GX_NULL)
        {
            /* the queue is empty, just return */
            return GX_FAILURE;
        }
    }

    /* wait for an event to arrive in queue */
    rt_sem_take(&guix_event_queue.count_sem, RT_WAITING_FOREVER);

    /* lock down the queue */
    rt_mutex_take(&guix_event_queue.lock, RT_WAITING_FOREVER);

    /* copy the event into destination */
    *put_event = guix_event_queue.first->event_data;

    /* link this event holder back into free list */
    if (guix_event_queue.free_end)
    {
        guix_event_queue.free_end->next = guix_event_queue.first;
    }
    else
    {
        guix_event_queue.free = guix_event_queue.first;
    }

    guix_event_queue.free_end = guix_event_queue.first;
    guix_event_queue.first = guix_event_queue.first->next;
    guix_event_queue.free_end->next = GX_NULL;

    if (!guix_event_queue.first)
    {
        guix_event_queue.last = GX_NULL;
    }

    /* check for popping out a timer event */
    if (put_event->gx_event_type == GX_EVENT_TIMER)
    {
        guix_timer_event_pending = GX_FALSE;
    }

    /* unlock the queue */
    rt_mutex_release(&guix_event_queue.lock);
    return GX_SUCCESS;
}

/* event_purge: delete events targetted to particular widget */
VOID gx_generic_event_purge(GX_WIDGET *target)
{
    GX_BOOL Purge;
    GUIX_LINKED_EVENT *pTest;

    /* Lock down the guix queue */

    rt_mutex_take(&guix_event_queue.lock, RT_WAITING_FOREVER);

    // look for events targetted to widget or widget children:
    pTest = guix_event_queue.first;

    while (pTest)
    {
        Purge = GX_FALSE;

        if (pTest->event_data.gx_event_target)
        {
            if (pTest->event_data.gx_event_target == target)
            {
                Purge = GX_TRUE;
            }
            else
            {
                gx_widget_child_detect(target, pTest->event_data.gx_event_target, &Purge);
            }
            if (Purge)
            {
                pTest->event_data.gx_event_target = GX_NULL;
                pTest->event_data.gx_event_type = 0;
            }
        }
        pTest = pTest->next;
    }

    rt_mutex_release(&guix_event_queue.lock);
}

/* start the RTOS timer */
VOID gx_generic_timer_start(VOID)
{
    rt_thread_resume(&guix_timer_tcb);
}

/* stop the RTOS timer */
VOID gx_generic_timer_stop(VOID)
{
    rt_thread_suspend(&guix_timer_tcb);
}

/* lock the system protection mutex */
VOID gx_generic_system_mutex_lock(VOID)
{
    rt_mutex_take(&guix_system_lock_mutex, RT_WAITING_FOREVER);
}

/* unlock system protection mutex */
VOID gx_generic_system_mutex_unlock(VOID)
{
    rt_mutex_release(&guix_system_lock_mutex);
}

/* return number of low-level system timer ticks. Used for pen speed caculations */
ULONG gx_generic_system_time_get(VOID)
{
    return ((ULONG)rt_tick_get());
}

/* thread_identify: return current thread identifier, cast as VOID * */
VOID *gx_generic_thread_identify(VOID)
{
    return rt_thread_self();
}

/* a simple task to drive the GUIX timer mechanism */
void guix_timer_task_entry(void *unused)
{
    int TickCount = OS_CFG_TICK_RATE_HZ / GX_TICKS_SECOND;

    if (TickCount <= 0)
    {
        TickCount = 1;
    }

    while (1)
    {
        rt_thread_mdelay(TickCount);

        /* prevent sending timer events faster than they can be processed */
        if (!guix_timer_event_pending)
        {
            guix_timer_event_pending = GX_TRUE;
            _gx_system_timer_expiration(0);
        }
    }
}

VOID gx_generic_time_delay(int ticks)
{
    rt_thread_mdelay(ticks);
}
#endif //(GUIX_BINDING_RT_THREAD)

#endif //(GX_THREADX_BINDING)
