//macros on port ranges

#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

int main(void){

	printf("%d", IPPORT_RESERVED);//ports less than are reserved for su use
	printf("\n%d", IPPORT_USERRESERVED);//reserved for explicit use (never allocated auto)

	return 0;
}