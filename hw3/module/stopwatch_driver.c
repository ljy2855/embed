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
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#define DRIVER_NAME "stopwatch"
#define FPGA_DOT_BASE_ADDR 0x08000210
#define FPGA_FND_BASE_ADDR 0x08000004

static int inter_major=242, inter_minor=0;
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;
static ktime_t start_time, end_time;
static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);
static void fpga_timer_handler(unsigned long data);
static void write_device(struct work_struct *work);


static struct stopwatch{
    struct timer_list timer;
    unsigned char fnd[4];
    int is_stop; // time stop
    int is_halt; // stopwatch terminate
    int ticker; // 0.1s count
};
static struct stopwatch stopwatch_data;


unsigned char fpga_number_patterns[10][10] = {
    {0x3e, 0x7f, 0x63, 0x73, 0x73, 0x6f, 0x67, 0x63, 0x7f, 0x3e}, // 0
    {0x0c, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e}, // 1
    {0x7e, 0x7f, 0x03, 0x03, 0x3f, 0x7e, 0x60, 0x60, 0x7f, 0x7f}, // 2
    {0xfe, 0x7f, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7e}, // 3
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7f, 0x7f, 0x06, 0x06}, // 4
    {0x7f, 0x7f, 0x60, 0x60, 0x7e, 0x7f, 0x03, 0x03, 0x7f, 0x7e}, // 5
    {0x60, 0x60, 0x60, 0x60, 0x7e, 0x7f, 0x63, 0x63, 0x7f, 0x3e}, // 6
    {0x7f, 0x7f, 0x63, 0x63, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03}, // 7
    {0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x3e}, // 8
    {0x3e, 0x7f, 0x63, 0x63, 0x7f, 0x3f, 0x03, 0x03, 0x03, 0x03}  // 9
};

unsigned char fpga_full_pattern[10] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};
unsigned char fpga_blank_pattern[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char *fpga_fnd_addr, *fpga_dot_addr;


static struct workqueue_struct *wq;
wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);
DECLARE_WORK(write_work,write_device);
static struct file_operations inter_fops =
{
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};


static void write_device(struct work_struct *work){
    __u16 value = 0;
    int i, number;
    value += stopwatch_data.fnd[0] << 12;
    value += stopwatch_data.fnd[1] << 8;
    value += stopwatch_data.fnd[2] << 4;
    value += stopwatch_data.fnd[3];
    outw(value, (unsigned int)fpga_fnd_addr);

    
    number = stopwatch_data.ticker % 10;
    for (i = 0; i < 10; i++)
        outw((__u16)(fpga_number_patterns[number][i] & 0x7F), (unsigned int)fpga_dot_addr + i * 2);
}

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "Start StopWatch\n");
    if(gpio_get_value(IMX_GPIO_NR(1, 11)) == 0)
        stopwatch_data.is_stop = 0;

	return IRQ_HANDLED;
}

irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
    if(gpio_get_value(IMX_GPIO_NR(1, 12)) == 0)
        stopwatch_data.is_stop = 1;
    return IRQ_HANDLED;
}

irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

    if(gpio_get_value(IMX_GPIO_NR(2, 15)) == 0){
        stopwatch_data.fnd[0] = 0;
        stopwatch_data.fnd[1] = 0;
        stopwatch_data.fnd[2] = 0;
        stopwatch_data.fnd[3] = 0;
        stopwatch_data.ticker = 0;
        stopwatch_data.is_stop = 1;

        queue_work(wq,&write_work);
    }
    return IRQ_HANDLED;
}

irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
    if(gpio_get_value(IMX_GPIO_NR(5, 14)) ==0 ){
        //press button
        start_time = ktime_get();
        printk("VOL DOWN PRESSED\n");
    }else{
        printk("VOL DOWN KEY UP\n");
        end_time = ktime_get();

        s64 time_diff = ktime_to_ms(ktime_sub(end_time,start_time));
        if(time_diff >= 3000)
            stopwatch_data.is_halt = 1;
    }
    return IRQ_HANDLED;
}

static void fpga_timer_handler(unsigned long data){
    struct stopwatch *cur_data = (struct stopwatch *)data;
    static struct work_struct work;

    if(cur_data->is_halt){
        __wake_up(&wq_write, 1, 1, NULL); 
        printk("wake up\n");
        return;
    }

    if(!cur_data->is_stop){
        cur_data->ticker++;
        cur_data->fnd[0] = ((cur_data->ticker / 10) / 60) / 10;
        cur_data->fnd[1] = ((cur_data->ticker / 10) / 60) % 10;
        cur_data->fnd[2] = ((cur_data->ticker / 10) % 60) / 10;
        cur_data->fnd[3] = ((cur_data->ticker / 10) % 60) % 10;
        queue_work(wq,&write_work);
        // write_device(NULL);
    }   
    
    stopwatch_data.timer.expires = jiffies + (HZ /10);
    stopwatch_data.timer.data = (unsigned long)&stopwatch_data;
    stopwatch_data.timer.function = fpga_timer_handler;
    add_timer(&stopwatch_data.timer);
}

static int inter_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");

	// HOME
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	// BACK
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

	// VOLUP
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	// VOLDOWN
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);

	return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){

    printk("inter_write\n");

    // init mydata values
    stopwatch_data.fnd[0] = 0;
    stopwatch_data.fnd[1] = 0;
    stopwatch_data.fnd[2] = 0;
    stopwatch_data.fnd[3] = 0;
    stopwatch_data.ticker = 0;
    stopwatch_data.is_stop = 1;
    stopwatch_data.is_halt = 0;
    write_device(NULL);

    
    
    stopwatch_data.timer.expires = jiffies + (HZ / 10);
    stopwatch_data.timer.data = (unsigned long)&stopwatch_data;
    stopwatch_data.timer.function = fpga_timer_handler;
    add_timer(&stopwatch_data.timer);

    // sleep application
    printk("sleep on\n");
    interruptible_sleep_on(&wq_write);
    
    stopwatch_data.fnd[0] = 0;
    stopwatch_data.fnd[1] = 0;
    stopwatch_data.fnd[2] = 0;
    stopwatch_data.fnd[3] = 0;
    stopwatch_data.ticker = 0;
    stopwatch_data.is_stop = 1;
    stopwatch_data.is_halt = 0;
    write_device(NULL);

	return 0;
}

static int inter_register_cdev(void)
{
	int error;
	if(inter_major) {
		inter_dev = MKDEV(inter_major, inter_minor);
		error = register_chrdev_region(inter_dev,1,DRIVER_NAME);
	}else{
		error = alloc_chrdev_region(&inter_dev,inter_minor,1,DRIVER_NAME);
		inter_major = MAJOR(inter_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", inter_major);
	cdev_init(&inter_cdev, &inter_fops);
	inter_cdev.owner = THIS_MODULE;
	inter_cdev.ops = &inter_fops;
	error = cdev_add(&inter_cdev, inter_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}

static int __init inter_init(void) {
	int result;
    // init intr
	if((result = inter_register_cdev()) < 0 )
		return result;

    // init timer
    init_timer(&(stopwatch_data.timer));

    // init device
    fpga_fnd_addr = ioremap(FPGA_FND_BASE_ADDR, 0x4);
    fpga_dot_addr = ioremap(FPGA_DOT_BASE_ADDR, 0x10);

    // init workqueue
    wq = create_workqueue("wq");
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");
	return 0;
}

static void __exit inter_exit(void) {
	cdev_del(&inter_cdev);
    del_timer_sync(&stopwatch_data.timer);
    destroy_workqueue(wq);
    iounmap(fpga_dot_addr);
    iounmap(fpga_fnd_addr);
	unregister_chrdev_region(inter_dev, 1);
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);
MODULE_LICENSE("GPL");
