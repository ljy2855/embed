#include <linux/kernel.h>
#include <linux/uaccess.h>

struct mystruct {
	int a;
	int b;
};

asmlinkage int sys_newcall3(struct mystruct *dd) {
	struct mystruct my_st;
	copy_from_user(&my_st, dd, sizeof(my_st));

	int a = my_st.a;
	int b = my_st.b;
	printk("sys_newcalld %d %d, sum : %d\n", a,b,a+b);

	return 23;
}
