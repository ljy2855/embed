#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

/*
#include <linux/ide.h>
#include <linux/sched.h>
*/

#define IOM_FND_ADDRESS 0x08000004 // pysical address

static int inter_major = 242, inter_minor = 0;
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;

#define MY_WORK_QUEUE_NAME "WQsched.c"
static struct workqueue_struct *my_wq;  // work queue for HOME
static struct workqueue_struct *my_wq2; // work queue for VOL+

static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void *dev_id, struct pt_regs *reg); // handler for HOME
irqreturn_t inter_handler2(int irq, void *dev_id, struct pt_regs *reg); // handler for BACK
irqreturn_t inter_handler3(int irq, void *dev_id, struct pt_regs *reg); // handler for VOL+
irqreturn_t inter_handler4(int irq, void *dev_id, struct pt_regs *reg); // handler for VOL-

static int inter_usage = 0;
static int my_wq_init = 0;  // init flag for my_wq
static int my_wq2_init = 0; // init flag for my_wq2

static unsigned char *iom_fnd_addr;

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

static struct file_operations inter_fops =
    {
        .open = inter_open,
        .write = inter_write,
        .release = inter_release,
};

static struct struct_mydata
{
    struct timer_list timer;
    unsigned char fnd[4];
    int elapsed; // elapsed time (s)
    int running; // 1 if HOME pressed (flag)
    int stopped; // 1 if VDWN pressed (flag)
    int stp_cnt; // inc if VDWN pressed til' 30, then stop app by waking up
};

struct struct_mydata mydata;

static void my_wq_function(struct work_struct *work)
{
    // fnd write
    unsigned short int _si_value;
    _si_value = (mydata.fnd[0] << 12) | (mydata.fnd[1] << 8) | (mydata.fnd[2] << 4) | (mydata.fnd[3]);
    outw(_si_value, (unsigned int)iom_fnd_addr);
    return;
}

static void my_wq_function2(struct work_struct *work)
{
    unsigned short int _si_value;
    // reset data
    mydata.elapsed = 0;
    mydata.fnd[0] = 0;
    mydata.fnd[1] = 0;
    mydata.fnd[2] = 0;
    mydata.fnd[3] = 0;
    mydata.stp_cnt = 0;
    // fnd write
    _si_value = (mydata.fnd[0] << 12) | (mydata.fnd[1] << 8) | (mydata.fnd[2] << 4) | (mydata.fnd[3]);
    outw(_si_value, (unsigned int)iom_fnd_addr);
    return;
}

irqreturn_t inter_handler1(int irq, void *dev_id, struct pt_regs *reg)
{
    int value = gpio_get_value(IMX_GPIO_NR(1, 11));
    printk(KERN_ALERT "interrupt1!!! = %x\n", value);
    if (value == 0)         // HOME FALLING
        mydata.running = 1; // set running flag to 1 -> inc elapsed time
    return IRQ_HANDLED;
}

irqreturn_t inter_handler2(int irq, void *dev_id, struct pt_regs *reg)
{
    int value = gpio_get_value(IMX_GPIO_NR(1, 12));
    printk(KERN_ALERT "interrupt2!!! = %x\n", value);
    if (value == 0)         // BACK FALLING
        mydata.running = 0; // set running flag to 0 -> X inc elapsed time
    return IRQ_HANDLED;
}

irqreturn_t inter_handler3(int irq, void *dev_id, struct pt_regs *reg)
{
    static struct work_struct work;
    // top half
    int value = gpio_get_value(IMX_GPIO_NR(2, 15));
    printk(KERN_ALERT "interrupt3!!! = %x\n", value);
    if (value == 0)
    {
        // bottom half
        flush_workqueue(my_wq);
        if (!my_wq2_init)
        {
            INIT_WORK(&work, my_wq_function2);
            my_wq2_init = 1;
        }
        else
            PREPARE_WORK(&work, my_wq_function2);
        queue_work(my_wq2, &work);
    }

    return IRQ_HANDLED;
}

irqreturn_t inter_handler4(int irq, void *dev_id, struct pt_regs *reg)
{
    int value = gpio_get_value(IMX_GPIO_NR(5, 14));
    printk(KERN_ALERT "interrupt4!!! = %x\n", value);
    if (value == 0)         // if VOL- FALLING
        mydata.stopped = 1; // set stop flag to 1 -> inc stop count
    else
        mydata.stopped = 0; // set stop flag to 1 -> X inc stop count
    return IRQ_HANDLED;
}

static void timer_handler(unsigned long arg)
{
    struct struct_mydata *data = (struct struct_mydata *)arg;
    static struct work_struct work;

    if (data->running)
    {
        data->elapsed++;
        if (data->elapsed % 10 == 0)
        { // for every 1s
            // top half
            data->fnd[0] = ((data->elapsed / 10) / 60) / 10;
            data->fnd[1] = ((data->elapsed / 10) / 60) % 10;
            data->fnd[2] = ((data->elapsed / 10) % 60) / 10;
            data->fnd[3] = ((data->elapsed / 10) % 60) % 10;
            // bottom half
            if (!my_wq_init)
            {
                INIT_WORK(&work, my_wq_function);
                my_wq_init = 1;
            }
            else
            {
                PREPARE_WORK(&work, my_wq_function);
            }
            queue_work(my_wq, &work);
        }
    }

    if (data->stopped)
    {
        data->stp_cnt++;
        if (data->stp_cnt == 30)
        {                                     // VDWN pressed for more than 3s
            __wake_up(&wq_write, 1, 1, NULL); // wake application program up
            printk("wake up\n");
            return;
        }
    }

    // add timer
    mydata.timer.expires = jiffies + (HZ / 10);
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = timer_handler;

    add_timer(&mydata.timer);
}

static int inter_open(struct inode *minode, struct file *mfile)
{
    int ret;
    int irq;

    printk(KERN_ALERT "Open Module\n");

    if (inter_usage != 0)
        return -EBUSY;
    inter_usage = 1;

    // int1 - HOME
    gpio_direction_input(IMX_GPIO_NR(1, 11));
    irq = gpio_to_irq(IMX_GPIO_NR(1, 11));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

    // int2 - BACK
    gpio_direction_input(IMX_GPIO_NR(1, 12));
    irq = gpio_to_irq(IMX_GPIO_NR(1, 12));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

    // int3 - VOL+
    gpio_direction_input(IMX_GPIO_NR(2, 15));
    irq = gpio_to_irq(IMX_GPIO_NR(2, 15));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

    // int4 - VOL-
    gpio_direction_input(IMX_GPIO_NR(5, 14));
    irq = gpio_to_irq(IMX_GPIO_NR(5, 14));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING, "voldown", 0);

    return 0;
}

static int inter_release(struct inode *minode, struct file *mfile)
{
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
    free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
    free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

    inter_usage = 0;

    printk(KERN_ALERT "Release Module\n");
    return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{

    printk("inter_write\n");

    // init mydata values
    mydata.elapsed = 0;
    mydata.running = 0;
    mydata.stopped = 0;
    mydata.stp_cnt = 0;

    // initial timer addition
    mydata.timer.expires = jiffies + (HZ / 10);
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = timer_handler;
    add_timer(&mydata.timer);

    // sleep application
    printk("sleep on\n");
    interruptible_sleep_on(&wq_write);

    // after application woken up
    printk("woken up\n");
    // delete timer
    del_timer(&mydata.timer);
    // fnd write 0
    outw(0, (unsigned int)iom_fnd_addr);

    return 0;
}

static int inter_register_cdev(void)
{
    int error;
    if (inter_major)
    {
        inter_dev = MKDEV(inter_major, inter_minor);
        error = register_chrdev_region(inter_dev, 1, "stopwatch");
    }
    else
    {
        error = alloc_chrdev_region(&inter_dev, inter_minor, 1, "stopwatch");
        inter_major = MAJOR(inter_dev);
    }
    if (error < 0)
    {
        printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
        return result;
    }
    printk(KERN_ALERT "major number = %d\n", inter_major);
    cdev_init(&inter_cdev, &inter_fops);
    inter_cdev.owner = THIS_MODULE;
    inter_cdev.ops = &inter_fops;
    error = cdev_add(&inter_cdev, inter_dev, 1);
    if (error)
    {
        printk(KERN_NOTICE "inter Register Error %d\n", error);
    }
    return 0;
}

static int __init inter_init(void)
{
    int result;
    if ((result = inter_register_cdev()) < 0)
        return result;

    iom_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
    init_timer(&(mydata.timer));

    init_waitqueue_head(&wq_write);
    my_wq = create_workqueue(MY_WORK_QUEUE_NAME);
    my_wq2 = create_workqueue(MY_WORK_QUEUE_NAME);

    printk(KERN_ALERT "Init Module Success \n");
    printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");
    return 0;
}

static void __exit inter_exit(void)
{
    destroy_workqueue(my_wq);
    destroy_workqueue(my_wq2);

    del_timer_sync(&mydata.timer);
    iounmap(iom_fnd_addr);

    cdev_del(&inter_cdev);
    unregister_chrdev_region(inter_dev, 1);
    printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);

MODULE_LICENSE("GPL");
