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
#define TIMER_DEVICE_NAME "/dev/fpga_timer"

#define IOCTL_SET_OPTION _IOW(TIMER_MAJOR, 0, unsigned long)
#define IOCTL_COMMAND 1

#define FPGA_DOT_BASE_ADDR 0x08000210
#define FPGA_LED_BASE_ADDR 0x08000016
#define FPGA_FND_BASE_ADDR 0x08000004
#define FPGA_LCD_BASE_ADDR 0x08000090

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

unsigned char fpga_full_pattern[10] = {0x7f, ...};
unsigned char fpga_blank_pattern[10] = {0x00, ...};

static int device_open_count = 0;
static unsigned char *fpga_fnd_addr, *fpga_led_addr, *fpga_dot_addr, *fpga_lcd_addr;

static unsigned char student_number[8] = "20191630";
static unsigned char student_name[10] = "LeeJinYong";

struct timer_data
{
    struct timer_list timer;
    int interval;
    int count;
    unsigned char init_values[4];
    int active_digit;
    int current_number;
    int rotation_count;
    int lcd_index_line1, lcd_index_line2;
    int lcd_index_increment1, lcd_index_increment2;
    unsigned char lcd_buffer[MAX_LCD_BUFFER + 1];
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
    switch (cmd)
    {
    case IOCTL_SET_OPTION:
        // Extract and set initial values for FND device control
        for (i = 0; i < 4; i++)
        {
            timer_data.init_values[i] = (arg >> (12 - 3 * i)) & 0xF;
        }

        timer_data.active_digit = 0;            // Start with the first digit
        timer_data.count = (arg >> 20) & 0xFFF; // Extract count
        timer_data.interval = arg >> 32;        // Extract interval in ms
        timer_data.current_number = timer_data.init_values[timer_data.active_digit];
        timer_data.rotation_count = 1;

        timer_data.lcd_index_line1 = 0;
        timer_data.lcd_index_line2 = MAX_LCD_LINE_LENGTH;
        timer_data.lcd_index_increment1 = 1;
        timer_data.lcd_index_increment2 = -1;
        memset(timer_data.lcd_buffer, ' ', MAX_LCD_BUFFER);
        break;

    case IOCTL_COMMAND:
        // Configure and start the timer
        mod_timer(&timer_data.timer, jiffies + msecs_to_jiffies(timer_data.interval));
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static int fpga_timer_release(struct inode *inode, struct *file) // TODO 코드를 소중히 하자 (by 괴도 루팡)
{
    printk("Timer device released\n");
    device_open_count = 0;
    return 0;
}

// helpers
void clear_fpga_devices(void)
{
    // Resets all FPGA device states
}

void write_fnd(unsigned char *values)
{
    // Writes to the FND device
}

void write_led(int number)
{
    // Writes to the LED device
}

void write_dot(int number)
{
    // Writes to the DOT device
}

void update_lcd_display(struct timer_data *td)
{
    // Updates the text on the LCD display
}

void update_digit_and_rotation(struct timer_data *td)
{
    // Manages the rotation and active digit logic
}

static void fpga_timer_handler(unsigned long data)
{
    struct timer_data *td = (struct timer_data *)data;
    int i;

    printk("Timer handler invoked\n");

    if (td->count <= 0)
    {
        // Stop the timer and reset all devices
        clear_fpga_devices();
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
    td->current_number = (td->current_number % 9) + 1; // Rotate through 1-9
    td->rotation_count++;
    update_digit_and_rotation(td);

    // Rearm the timer
    mod_timer(&td->timer, jiffies + msecs_to_jiffies(td->interval));
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

    unregister_chrdev(TIMER_MAJOR, TIMER_DEVICE_NAME);
}

module_init(fpga_timer_init);
module_exit(fpga_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LeeJinYong");