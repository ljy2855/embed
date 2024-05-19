/* FPGA Timer Ioremap Control
FILE : fpga_timer_driver.c
AUTH : jenny_0729@naver.com*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/timer.h>

#define IOM_TIMER_MAJOR 242
#define IOM_TIMER_NAME "/dev/dev_driver"

#define SET_OPTION _IOW(IOM_TIMER_MAJOR, 0, unsigned long)
#define COMMAND 1

#define IOM_DOT_ADDRESS 0x08000210
#define IOM_LED_ADDRESS 0x08000016
#define IOM_FND_ADDRESS 0x08000004
#define IOM_TEXT_LCD_ADDRESS 0x08000090

#define MAX_BUFF 32
#define LINE_BUFF 16

unsigned char fpga_numbers[10][10] = {
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

unsigned char fpga_set_full[10] = {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f};
unsigned char fpga_set_blank[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static int timer_usage = 0;
static unsigned char *iom_fnd_addr, *iom_led_addr, *iom_dot_addr, *iom_text_lcd_addr;

static unsigned char stunum[8] = "20201573";
static unsigned char stuname[10] = "JihyeonKim";

struct timer_data
{
    struct timer_list timer;
    int interval;                    // Timer interval
    int count;                       // Timer update count
    unsigned char init[4];           // FND value
    int digit;                       // FND non-zero digit
    int number;                      // Number printed on FND, DOT
    int rotation;                    // Number rotation count (1~8)
    int lcd_idx1, lcd_idx2;          // LCD string start index for line1,2
    int lcd_flag1, lcd_flag2;        // LCD index increment/decrement
    unsigned char lcd[MAX_BUFF + 1]; // LCD string to be printed
};

static struct timer_data mydata;

static int iom_timer_open(struct inode *minode, struct file *mfile);
static int iom_timer_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
static int iom_timer_release(struct inode *minode, struct file *mfile);
static void iom_timer_handler(unsigned long arg);

static struct file_operations iom_timer_fops = {
    .open = iom_timer_open,
    .unlocked_ioctl = iom_timer_ioctl,
    .release = iom_timer_release,
};

static int iom_timer_open(struct inode *minode, struct file *mfile)
{
    printk("timer_open\n");
    if (timer_usage != 0)
    {
        return -EBUSY;
    }
    timer_usage = 1;
    return 0;
}

static int iom_timer_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    int init = ioctl_param % 10000;
    int i = 0;

    printk("timer_ioctl\n");

    switch (ioctl_num)
    {
    case SET_OPTION:
        // Initialize values for device control
        mydata.init[0] = init / 1000;
        mydata.init[1] = (init / 100) % 10;
        mydata.init[2] = (init / 10) % 10;
        mydata.init[3] = init % 10;

        while (i < 4)
        {
            if (mydata.init[i] != 0)
            {
                mydata.number = mydata.init[i];
                mydata.digit = i;
            }
            i++;
        }
        mydata.count = (ioctl_param / 10000) % 1000;
        mydata.interval = ioctl_param / 10000000;
        mydata.rotation = 1;
        mydata.lcd_idx1 = 0;
        mydata.lcd_idx2 = LINE_BUFF;
        mydata.lcd_flag1 = 1;
        mydata.lcd_flag2 = 1;
        for (i = 0; i < MAX_BUFF + 1; i++)
        {
            mydata.lcd[i] = ' ';
        }
        break;
    case COMMAND:
        // Update values for timer control
        mydata.timer.expires = jiffies + (mydata.interval * HZ / 10);
        mydata.timer.data = (unsigned long)&mydata;
        mydata.timer.function = iom_timer_handler;
        add_timer(&mydata.timer);
        break;
    }
    return 1;
}

static int iom_timer_release(struct inode *minode, struct file *mfile)
{
    printk("timer_release\n");
    timer_usage = 0;
    return 0;
}

static void iom_timer_handler(unsigned long arg)
{
    int i;
    unsigned short fnd_value = 0;
    unsigned short lcd_value = 0;
    struct timer_data *data = (struct timer_data *)arg;

    // Check count
    if (data->count == 0)
    {
        outw(0, (unsigned int)iom_fnd_addr);
        outw(0, (unsigned int)iom_led_addr);
        for (i = 0; i < 10; i++)
        {
            outw(0, (unsigned int)iom_dot_addr + i * 2);
        }
        for (i = 0; i < MAX_BUFF; i++)
        {
            lcd_value = ((' ' & 0xFF) << 8) | (' ' & 0xFF);
            outw(lcd_value, (unsigned int)iom_text_lcd_addr + i);
            i++;
        }
        return;
    }

    // FND write
    fnd_value = (data->init[0] << 12) | (data->init[1] << 8) | (data->init[2] << 4) | data->init[3];
    outw(fnd_value, (unsigned int)iom_fnd_addr);

    // LED write
    fnd_value = 0x80 >> (data->number - 1);
    outw(fnd_value, (unsigned int)iom_led_addr);

    // Dot write
    for (i = 0; i < 10; i++)
    {
        lcd_value = fpga_numbers[data->number][i] & 0x7F;
        outw(lcd_value, (unsigned int)iom_dot_addr + i * 2);
    }

    // Text LCD write
    for (i = 0; i < 8; i++)
        data->lcd[i + data->lcd_idx1] = stunum[i];
    for (i = 0; i < 10; i++)
        data->lcd[i + data->lcd_idx2] = stuname[i];
    for (i = 0; i < MAX_BUFF; i++)
    {
        lcd_value = ((data->lcd[i] & 0xFF) << 8) | (data->lcd[i + 1] & 0xFF);
        outw(lcd_value, (unsigned int)iom_text_lcd_addr + i);
        i++;
    }

    // Update values
    for (i = 0; i < MAX_BUFF; i++)
        data->lcd[i] = ' '; // Clear LCD
    data->count--;          // Decrease count
    data->number++;         // Increase number
    if (data->number == 9)
    {                     // If 8 was printed just before,
        data->number = 1; // Set number to 1
    }
    data->rotation++; // Increase rotation
    if (data->rotation == 9)
    {                                // If rotation is over at current digit
        data->rotation = 1;          // Set rotation count to 1
        data->init[data->digit] = 0; // Set previous digit number to 0
        data->digit++;               // Move current digit to the right
        if (data->digit > 3)
        {                    // If previous digit was the rightmost digit
            data->digit = 0; // Set current digit to leftmost digit
        }
    }
    data->init[data->digit] = data->number; // Update current digit value to number
    data->lcd_idx1 += data->lcd_flag1;      // Update lcd_idx1
    if (data->lcd_idx1 == 8)
    {                         // If line1 string met right end
        data->lcd_flag1 = -1; // Decrease lcd_idx1 from next interval
    }
    if (data->lcd_idx1 == 0)
    {                        // If line1 string met left end
        data->lcd_flag1 = 1; // Increase lcd_idx1 from next interval
    }
    data->lcd_idx2 += data->lcd_flag2; // Update lcd_idx2
    if (data->lcd_idx2 == 22)
    {                         // If line2 string met right end
        data->lcd_flag2 = -1; // Decrease lcd_idx2 from next interval
    }
    if (data->lcd_idx2 == 16)
    {                        // If line2 string met left end
        data->lcd_flag2 = 1; // Increase lcd_idx2 from next interval
    }

    mydata.timer.expires = jiffies + (mydata.interval * HZ / 10);
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = iom_timer_handler;

    add_timer(&mydata.timer);
}

static int __init iom_timer_init(void)
{
    int result;
    printk("timer_init\n");

    // Device register
    result = register_chrdev(IOM_TIMER_MAJOR, IOM_TIMER_NAME, &iom_timer_fops);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get any major\n");
        return result;
    }

    // Map FPGA device address
    iom_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
    iom_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
    iom_dot_addr = ioremap(IOM_DOT_ADDRESS, 0x10);
    iom_text_lcd_addr = ioremap(IOM_TEXT_LCD_ADDRESS, 0x32);

    // Initialize timer
    init_timer(&(mydata.timer));

    printk(KERN_ALERT "Init Module Success\n");
    printk(KERN_ALERT "Device : %s, major num : %d\n", IOM_TIMER_NAME, IOM_TIMER_MAJOR);

    return 0;
}

static void __exit iom_timer_exit(void)
{
    printk("timer_exit\n");

    // Delete timer
    timer_usage = 0;
    del_timer_sync(&mydata.timer);

    // Unmap device address
    iounmap(iom_text_lcd_addr);
    iounmap(iom_dot_addr);
    iounmap(iom_led_addr);
    iounmap(iom_fnd_addr);

    unregister_chrdev(IOM_TIMER_MAJOR, IOM_TIMER_NAME);
}

module_init(iom_timer_init);
module_exit(iom_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JIHYEON");
