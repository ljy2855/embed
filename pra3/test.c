#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

struct mystruct {
	int a;
	int b;
};

int main(void){
	struct mystruct my_st;
	my_st.a =2019;
	my_st.b =1630;
	
	syscall(378, &my_st);


	return 0;
}
