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

#define TIMER_MAJOR 242
#define TIMER_DEVICE_NAME "/dev/dev_driver"

#define IOCTL_SET_OPTION _IOW(TIMER_MAJOR, 0, unsigned long)
#define IOCTL_COMMAND 1

#define FPGA_DOT_BASE_ADDR 0x08000210
#define FPGA_LED_BASE_ADDR 0x08000016
#define FPGA_FND_BASE_ADDR 0x08000004
#define FPGA_LCD_BASE_ADDR 0x08000090
#define FPGA_RST_BASE_ADDR 0x08000000

#define MAX_LCD_BUFFER 32
#define MAX_LCD_LINE_LENGTH 16

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

static int device_open_count = 0;
static unsigned char *fpga_fnd_addr, *fpga_led_addr, *fpga_dot_addr, *fpga_lcd_addr, *fpga_rst_addr;

static unsigned char name[3] = "LJY";

struct timer_data
{
    struct timer_list timer;
    int interval;
    int count;
    unsigned char init_values[4];
    int active_digit;
    int current_number;
    int rotation_count;
    int finish_count;
    int lcd_index;
};

static struct timer_data timer_data;

static int fpga_timer_open(struct inode *inode, struct file *file);
static int fpga_timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int fpga_timer_release(struct inode *inode, struct file *file);
static void fpga_timer_handler(unsigned long data);

static struct file_operations fpga_timer_fops = {
    .open = fpga_timer_open,
    .unlocked_ioctl = fpga_timer_ioctl,
    .release = fpga_timer_release,
};

static int fpga_timer_open(struct inode *inode, struct file *file)
{
    printk("Timer device opened\n");
    if (device_open_count != 0)
    {
        return -EBUSY;
    }
    device_open_count = 1;
    return 0;
}

static int fpga_timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk("Handling IOCTL\n");

    int i;
    int initial_value;
    switch (cmd)
    {
    case IOCTL_SET_OPTION:
        // Extract and set initial values for FND device control

        initial_value = arg & 0x3FFF; // Extract the lower 14 bits

        for (i = 3; i >= 0; --i)
        {
            timer_data.init_values[i] = (initial_value % 10); // convert integer to each digit
            initial_value /= 10;
            if (timer_data.init_values[i] != 0)
                timer_data.active_digit = i;
        }
        timer_data.count = (arg >> 14) & 0x7F;    // Extract the next 7 bits
        timer_data.interval = (arg >> 21) & 0x7F; // Extract the next 7 bits
        timer_data.current_number = timer_data.init_values[timer_data.active_digit];
        timer_data.rotation_count = 1;
        timer_data.finish_count = 3;

        break;

    case IOCTL_COMMAND:
        // Configure and start the timer
        timer_data.timer.expires = jiffies + (timer_data.interval * HZ / 10);
        timer_data.timer.data = (unsigned long)&timer_data;
        timer_data.timer.function = fpga_timer_handler;

        while (inw((unsigned int)fpga_rst_addr) != 0)
            ; // wait for reset switch
        add_timer(&timer_data.timer);

        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static int fpga_timer_release(struct inode *inode, struct file *file)
{
    printk("Timer device released\n");
    device_open_count = 0;
    return 0;
}

// helpers

// Writes to the FND device
void write_fnd(unsigned char *values)
{

    __u16 value = 0;
    value += values[0] << 12;
    value += values[1] << 8;
    value += values[2] << 4;
    value += values[3];
    outw(value, (unsigned int)fpga_fnd_addr);
}

// Writes to the LED device
void write_led(int number)
{

    outw((__u16)0x80 >> (number - 1), (unsigned int)fpga_led_addr);
}

// Writes to the DOT device
void write_dot(int number)
{
    int i;
    for (i = 0; i < 10; i++)
        outw((__u16)(fpga_number_patterns[number][i] & 0x7F), (unsigned int)fpga_dot_addr + i * 2);
}

// Updates the text on the LCD display
void update_lcd_display(struct timer_data *td)
{
    char line1[17] = {
        0,
    },
         line2[17] = {
             0,
         };
    int i, temp = td->lcd_index;
    __u16 value;

    memset(line1, ' ', MAX_LCD_LINE_LENGTH);
    memset(line2, ' ', MAX_LCD_LINE_LENGTH);

    snprintf(line1, sizeof(line1), "20191630%8d", td->count);
    printk("%s\n", line1);
    for (i = 0; i < 3; i++)
    {
        line2[temp] = name[i];
        temp = (temp + 1) % MAX_LCD_LINE_LENGTH;
    }
    td->lcd_index = (td->lcd_index + 1) % MAX_LCD_LINE_LENGTH;
    for (i = 0; i < MAX_LCD_LINE_LENGTH; i++)
    {
        value = ((line1[i] & 0xFF) << 8) | (line1[i + 1] & 0xFF);
        outw(value, (unsigned int)fpga_lcd_addr + i);
        i++;
    }
    for (i = 0; i < MAX_LCD_LINE_LENGTH; i++)
    {
        value = ((line2[i] & 0xFF) << 8) | (line2[i + 1] & 0xFF);
        outw(value, (unsigned int)fpga_lcd_addr + i + MAX_LCD_LINE_LENGTH);
        i++;
    }
}

// Manages the rotation and active digit logic
void update_digit_and_rotation(struct timer_data *td)
{
    // clear LCD buffer

    if (td->rotation_count == 1)
    {
        td->init_values[td->active_digit] = 0;
        td->active_digit = (td->active_digit + 1) % 4;
    }
    td->init_values[td->active_digit] = td->current_number;
}

// Resets all FPGA device states
void clear_fpga_devices(void)
{
    int i;
    outw(0, (unsigned int)fpga_fnd_addr);
    outw(0, (unsigned int)fpga_led_addr);
    for (i = 0; i < 10; i++)
        outw(0, (unsigned int)fpga_dot_addr + i * 2);
    for (i = 0; i < MAX_LCD_BUFFER; i++)
    {
        outw((__u16)((' ' & 0xFF) << 8) | (' ' & 0xFF), (unsigned int)fpga_lcd_addr + i);
        i++;
    }
}

// finish timer handler that display lcd
static void finish_handler(unsigned long data)
{
    struct timer_data *td = (struct timer_data *)data;
    char buffer[MAX_LCD_BUFFER + 1];
    int i;
    __u16 value;
    if (td->finish_count <= 0)
    {
        // Stop the timer and reset all devices
        clear_fpga_devices();
        return;
    }

    snprintf(buffer, sizeof(buffer), "Time's up!     0Shutdown in %1d...", td->finish_count);
    for (i = 0; i < MAX_LCD_BUFFER; i++)
    {
        value = ((buffer[i] & 0xFF) << 8) | (buffer[i + 1] & 0xFF);
        outw(value, (unsigned int)fpga_lcd_addr + i);
        i++;
    }

    td->finish_count--;
    timer_data.timer.expires = jiffies + HZ;
    timer_data.timer.data = (unsigned long)&timer_data;
    timer_data.timer.function = finish_handler;
    add_timer(&timer_data.timer);
}

// timer handler update device state
static void fpga_timer_handler(unsigned long data)
{
    struct timer_data *td = (struct timer_data *)data;
    int i;

    printk("Timer handler invoked\n");

    if (td->count <= 0)
    {
        // Stop the timer and reset devices
        outw(0, (unsigned int)fpga_fnd_addr);
        outw(0, (unsigned int)fpga_led_addr);
        for (i = 0; i < 10; i++)
            outw(0, (unsigned int)fpga_dot_addr + i * 2);

        // call finish handler timer
        timer_data.timer.expires = jiffies + HZ;
        timer_data.timer.data = (unsigned long)&timer_data;
        timer_data.timer.function = finish_handler;
        add_timer(&timer_data.timer);
        return;
    }

    // Update FND display based on the current number
    write_fnd(td->init_values);

    // Update LED pattern
    write_led(td->current_number);

    // Update DOT matrix
    write_dot(td->current_number);

    // Update LCD display
    update_lcd_display(td);

    // Prepare for the next timer interval
    td->count--;
    td->current_number = (td->current_number % 8) + 1; // Rotate through 1-8
    td->rotation_count = (td->rotation_count % 8) + 1; // Rotate index through 1-9
    update_digit_and_rotation(td);

    // Rearm the timer
    timer_data.timer.expires = jiffies + (timer_data.interval * HZ / 10);
    timer_data.timer.data = (unsigned long)&timer_data;
    timer_data.timer.function = fpga_timer_handler;
    add_timer(&timer_data.timer);
}

static int __init fpga_timer_init(void)
{
    int result;
    printk("Initializing the FPGA Timer Driver\n");

    result = register_chrdev(TIMER_MAJOR, TIMER_DEVICE_NAME, &fpga_timer_fops);
    if (result < 0)
    {
        printk(KERN_WARNING "Failed to obtain major number\n");
        return result;
    }

    fpga_fnd_addr = ioremap(FPGA_FND_BASE_ADDR, 0x4);
    fpga_led_addr = ioremap(FPGA_LED_BASE_ADDR, 0x1);
    fpga_dot_addr = ioremap(FPGA_DOT_BASE_ADDR, 0x10);
    fpga_lcd_addr = ioremap(FPGA_LCD_BASE_ADDR, 0x32);
    fpga_rst_addr = ioremap(FPGA_RST_BASE_ADDR, 0x1);

    init_timer(&(timer_data.timer));

    printk(KERN_ALERT "Driver loaded successfully\n");
    printk(KERN_ALERT "Device: %s, Major Num: %d\n", TIMER_DEVICE_NAME, TIMER_MAJOR);

    return 0;
}

static void __exit fpga_timer_exit(void)
{
    printk("Exiting FPGA Timer Driver\n");

    del_timer_sync(&timer_data.timer);
    iounmap(fpga_lcd_addr);
    iounmap(fpga_dot_addr);
    iounmap(fpga_led_addr);
    iounmap(fpga_fnd_addr);
    iounmap(fpga_rst_addr);

    unregister_chrdev(TIMER_MAJOR, TIMER_DEVICE_NAME);
}

module_init(fpga_timer_init);
module_exit(fpga_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LeeJinYong");
