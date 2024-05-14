#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

struct my_data{
	int a,b,c;
};
struct new_data{
	int a,b,c,d;
	int id;
};

extern long newcall(struct my_data *block);
extern long mynew_function(struct new_data *block);

int calldev_init(void){
	struct new_data data;
	printk(KERN_INFO "Embedded SoftWare 2017\n");
	data.id = 1630;

	printk(KERN_INFO "sysc# = %ld\n", mynew_function(&data));

	printk(KERN_INFO "Thousands= %d / hundreds= %d / tens= %d / units= %d \n", data.a,data.b,data.c,data.d);
	
	return 0;
}

void calldev_exit( void ) { }
module_init ( calldev_init );
module_exit ( calldev_exit );
MODULE_LICENSE("Dual BSD/GPL");
