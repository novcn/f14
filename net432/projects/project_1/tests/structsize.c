#include <stdio.h>
#include <stdlib.h>

typedef struct one{
	int identifier;
	char * message;
	char * user;
}one;

typedef struct two{
	int identifier;
	char * message;
}two;

one * init(one * o){
	o = (one*)malloc(sizeof(*o));
}

int main(void){

	one * o;
	two * t;
	// o->message = "initialized";

	// two * t;
	// t->identifier = 0;

	printf("%d\n",sizeof(*o));

	printf("%d\n",sizeof(*t));

	return 0;
}



