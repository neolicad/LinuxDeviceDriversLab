#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/irq.h>

MODULE_AUTHOR("neolicad");
MODULE_LICENSE("Dual BSD/GPL");

static void *nr_irqs_seq_start(struct seq_file *m, loff_t *pos) {
    if (*pos == 0) {
        (*pos)++;
        return pos;
    } 
    return NULL;
}

static void *nr_irqs_seq_next(struct seq_file *m, void *v, loff_t *pos) {
    (*pos)++;
    return NULL;
}

static int nr_irqs_seq_show(struct seq_file *m, void *v) {
    seq_printf(m, "proc nr_irqs: NR_IRQS = %d\n", NR_IRQS);    
    return 0;
}

static void nr_irqs_seq_stop(struct seq_file *m, void *v) {
    // Do nothing
}

static struct seq_operations nr_irqs_seq_ops = {
    .start = nr_irqs_seq_start,
    .next = nr_irqs_seq_next,
    .show = nr_irqs_seq_show,
    .stop = nr_irqs_seq_stop,
};

int proc_init(void) {
    proc_create_seq("nr_irqs", 0, NULL, &nr_irqs_seq_ops);
    return 0;
}

void proc_cleanup(void) {
    remove_proc_entry("nr_irqs", NULL);
}

module_init(proc_init);
module_exit(proc_cleanup);
