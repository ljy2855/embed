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

long newcall(struct my_data *data){
	int a, b;

	a=data->a;
	b=data->b;

	data->a=a+b;
	data->b=a-b;
	data->c=a%b;

	return 313;
}

long mynew_function(struct new_data *data) {
	int temp;
	temp = data->id;

	data->a = temp/1000;
	data->b = (temp/100)%10;
	data->c = (temp/10)%10;
	data->d = temp%10;

	return 314;
}

int funcdev_init( void ) {
	return 0;
}
void funcdev_exit( void ) { }

EXPORT_SYMBOL( newcall );
EXPORT_SYMBOL( mynew_function );
module_init ( funcdev_init );
module_exit ( funcdev_exit );
MODULE_LICENSE("Dual BSD/GPL");
