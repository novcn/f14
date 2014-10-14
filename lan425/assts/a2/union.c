#include <stdio.h>

int main(void){

	union intstring {
		int i;
		char *s;
	} x;
	int z = 1;
	int y;	
	if(z){
		x.s = "yup";
	} else {
		x.i = 3;
	}

	y = (x.i) + 0;
	printf("%d\n", y );

}