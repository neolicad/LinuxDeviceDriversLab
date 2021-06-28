/*
 * jit.c -- the just-in-time module
 *
 * Copyright (C) 2001,2003 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001,2003 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: jit.c,v 1.16 2004/09/26 07:02:43 gregkh Exp $
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/preempt.h>

#include <asm/hardirq.h>
/*
 * This module is a silly one: it only embeds short code fragments
 * that show how time delays can be handled in the kernel.
 */

int delay = HZ; /* the default delay, expressed in jiffies */

module_param(delay, int, 0);

MODULE_AUTHOR("Alessandro Rubini");
MODULE_LICENSE("Dual BSD/GPL");

/* use these as data pointers, to implement four files in one function */
enum jit_files {
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO
};

/*
 * This function prints one line of data, after sleeping one second.
 * It can sleep in different ways, according to the data pointer
 */
int jit_fn(char *buf, void *data)
{
	unsigned long j0, j1; /* jiffies */
	wait_queue_head_t wait;
    int len;

    printk(KERN_ALERT "jit_fn data=%ld", (long) data);
	init_waitqueue_head (&wait);
	j0 = jiffies;
	j1 = j0 + delay;

	switch((long)data) {
		case JIT_BUSY:
			while (time_before(jiffies, j1))
				cpu_relax();
			break;
		case JIT_SCHED:
			while (time_before(jiffies, j1)) {
                printk(KERN_ALERT "jit_fn before schedule jiffies=%ld", jiffies);
				schedule();
                printk(KERN_ALERT "jit_fn after schedule jiffies=%ld", jiffies);
			}
			break;
		case JIT_QUEUE:
			wait_event_interruptible_timeout(wait, 0, delay);
			break;
		case JIT_SCHEDTO:
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout (delay);
			break;
	}
	j1 = jiffies; /* actual value after we delayed */

	len = sprintf(buf, "%9li %9li\n", j0, j1);

	return len;
}

static void *currentime_seq_start(struct seq_file *m, loff_t *pos) {
    if (*pos < 4) {
        (*pos)++;
        return pos;
    }
    return NULL;
}

/*
 * This file, on the other hand, returns the current time forever
 */
static int currentime_seq_show(struct seq_file *m, void *v) {
    struct timespec64 tv1;
	struct timespec64 tv2;
	unsigned long j1;
	u64 j2;

	/* get them four */
	j1 = jiffies;
	j2 = get_jiffies_64();
	ktime_get_real_ts64(&tv1);
    ktime_get_coarse_real_ts64(&tv2);

	/* print */
	seq_printf(m,"0x%08lx 0x%016Lx %10li.%06li\n"
		       "%40li.%09li\n",
		       j1, j2,
		       (long) tv1.tv_sec, (long) tv1.tv_nsec,
		       (long) tv2.tv_sec, (long) tv2.tv_nsec);
    return 0;
}

static void *currentime_seq_next(struct seq_file *m, void *v, loff_t *pos) {
    if (*pos < 4) {
        (*pos)++;
        return pos;
    }
    (*pos)++;
    return NULL;
}

static void currentime_seq_stop(struct seq_file *m, void *v) {
    /* do nothing */
}



static struct seq_operations currentime_seq_ops = {
    .start = currentime_seq_start,
    .stop = currentime_seq_stop,
    .next = currentime_seq_next,
    .show = currentime_seq_show
};

static int currentime_seq_open(struct inode *inode, struct file *filp) {
    return seq_open(filp, &currentime_seq_ops);
}

static struct proc_ops currentime_proc_ops = {
    .proc_open = currentime_seq_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};


static ssize_t jit_read(
        struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    void *data;
    char kbuf[100];
    int len, LINE_WIDTH=22;

    printk(KERN_ALERT "jit read, count = %ld", count);
    if (count != LINE_WIDTH) {
        return -EBADRQC;
    }
    data = PDE_DATA(file_inode(filp));
    len = jit_fn(kbuf, data);
    printk(KERN_ALERT "jit read, len=%d", len);
    if (len != LINE_WIDTH) {
        return -EHWPOISON;
    }
    copy_to_user(buf, kbuf, LINE_WIDTH);
    *f_pos += LINE_WIDTH;

    return LINE_WIDTH;
}

static struct proc_ops jit_proc_ops = {
    .proc_read = jit_read,
};

/*
 * The timer example follows
 */

int tdelay = 10;
module_param(tdelay, int, 0);

/* This data structure used as "data" for the timer and tasklet functions */
struct jit_data {
	struct timer_list timer;
	struct tasklet_struct tlet;
	int hi;
    wait_queue_head_t wait;
	unsigned long prevjiffies;
	unsigned char buf[1000];
    char *next;
	int loops;
};

#define JIT_ASYNC_LOOPS 5

void jit_timer_fn(struct timer_list *t)
{
	struct jit_data *data = from_timer(data, t, timer);
	unsigned long j = jiffies;
	data->next += sprintf(data->next, "%9li  %3li     %i    %6i   %i   %s\n",
			     j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
			     current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->timer.expires += tdelay;
		data->prevjiffies = j;
		add_timer(&data->timer);
	} else {
		wake_up_interruptible(&data->wait);
	}
}

static void *jitimer_seq_start(struct seq_file *m, loff_t *pos) {
 	struct jit_data *data;
	char *buf2;
	unsigned long j = jiffies;

    if (*pos) {
        return NULL;
    }

	data = kmalloc(sizeof(*data), GFP_KERNEL);
    if (!data) {
        /* TODO: how to return failure in seq_ops? */
        printk(KERN_ALERT "jitimer_seq_start: no memory to allocate jit_data");
		return NULL;
    }

    timer_setup(&data->timer, jit_timer_fn, 0);
	init_waitqueue_head (&data->wait);

	/* write the first lines in the buffer */
    buf2 = data->buf;
	buf2 += sprintf(buf2, "   time   delta  inirq    pid   cpu command\n");
	buf2 += sprintf(buf2, "%9li  %3li     %i    %6i   %i   %s\n",
			j, 0L, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm);

	/* fill the data for our timer function */
	data->prevjiffies = j;
	data->next = buf2;
	data->loops = JIT_ASYNC_LOOPS;
	
	/* register the timer */
	data->timer.expires = j + tdelay; /* parameter */
    add_timer(&data->timer);

	/* wait for the buffer to fill */
    wait_event_interruptible(data->wait, !data->loops);
    if (signal_pending(current)) {
        printk(KERN_ALERT "jitimer_seq_start: wait event interrupted");
        return NULL;
    }
    *data->next = '\0';
    (data->next)++;
    (*pos)++;
    return data;
}

static int jitimer_seq_show(struct seq_file *m, void *v) {
    struct jit_data *data = (struct jit_data *)v;
    seq_printf(m, data->buf);
    return 0;
}

static void *jitimer_seq_next(struct seq_file *m, void *v, loff_t *pos) {
    (*pos)++;
    return NULL;
}

static void jitimer_seq_stop(struct seq_file *m, void *v) {
    struct jit_data *data = (struct jit_data *)v;
    kfree(data);
}

static struct seq_operations jitimer_seq_ops = {
    .start = jitimer_seq_start,
    .stop = jitimer_seq_stop,
    .next = jitimer_seq_next,
    .show = jitimer_seq_show,
};

static int jitimer_seq_open(struct inode *inode, struct file *filp) {
    return seq_open(filp, &jitimer_seq_ops);
}

static struct proc_ops jitimer_proc_ops = {
    .proc_open = jitimer_seq_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release,
};


/*
void jit_tasklet_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *)arg;
	unsigned long j = jiffies;
	data->buf += sprintf(data->buf, "%9li  %3li     %i    %6i   %i   %s\n",
			     j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
			     current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->prevjiffies = j;
		if (data->hi)
			tasklet_hi_schedule(&data->tlet);
		else
			tasklet_schedule(&data->tlet);
	} else {
		wake_up_interruptible(&data->wait);
	}
}*/

/* the /proc function: allocate everything to allow concurrency */
/*int jit_tasklet(char *buf, char **start, off_t offset,
	      int len, int *eof, void *arg)
{
	struct jit_data *data;
	char *buf2 = buf;
	unsigned long j = jiffies;
	long hi = (long)arg;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	init_waitqueue_head (&data->wait); */

	/* write the first lines in the buffer */
	/*buf2 += sprintf(buf2, "   time   delta  inirq    pid   cpu command\n");
	buf2 += sprintf(buf2, "%9li  %3li     %i    %6i   %i   %s\n",
			j, 0L, in_interrupt() ? 1 : 0,
			current->pid, smp_processor_id(), current->comm); */

	/* fill the data for our tasklet function */
	/* data->prevjiffies = j;
	data->buf = buf2;
	data->loops = JIT_ASYNC_LOOPS; */
	
	/* register the tasklet */
	/*tasklet_init(&data->tlet, jit_tasklet_fn, (unsigned long)data);
	data->hi = hi;
	if (hi)
		tasklet_hi_schedule(&data->tlet);
	else
		tasklet_schedule(&data->tlet); */

	/* wait for the buffer to fill */
	/*wait_event_interruptible(data->wait, !data->loops);

	if (signal_pending(current))
		return -ERESTARTSYS;
	buf2 = data->buf;
	kfree(data);
	*eof = 1;
	return buf2 - buf;
} */



int __init jit_init(void)
{
    proc_create("currenttime", 0, NULL, &currentime_proc_ops);
    proc_create_data("jitbusy", 0, NULL, &jit_proc_ops, (void *)JIT_BUSY);
    proc_create_data("jitsched", 0, NULL, &jit_proc_ops, (void *)JIT_SCHED);
    proc_create_data("jitqueue", 0, NULL, &jit_proc_ops, (void *)JIT_QUEUE);
    proc_create_data(
            "jitschedto", 0, NULL, &jit_proc_ops, (void *)JIT_SCHEDTO);

    proc_create("jitimer", 0, NULL, &jitimer_proc_ops);
    
	/* create_proc_read_entry("jitimer", 0, NULL, jit_timer, NULL);
	create_proc_read_entry("jitasklet", 0, NULL, jit_tasklet, NULL);
	create_proc_read_entry("jitasklethi", 0, NULL, jit_tasklet, (void *)1); */

	return 0; /* success */
}

void __exit jit_cleanup(void)
{
	remove_proc_entry("currentime", NULL);
	remove_proc_entry("jitbusy", NULL);
	remove_proc_entry("jitsched", NULL);
	remove_proc_entry("jitqueue", NULL);
	remove_proc_entry("jitschedto", NULL);

	remove_proc_entry("jitimer", NULL);
	remove_proc_entry("jitasklet", NULL);
	remove_proc_entry("jitasklethi", NULL);
}

module_init(jit_init);
module_exit(jit_cleanup);
