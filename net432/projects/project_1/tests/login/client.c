#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>


int main(int argc, char * argv[]){

	char * server_hostname = "localhost";
	int server_port = 2000;
	char * client_username = "crake";

	int getaddrinfo(const char *node, // e.g. "www.example.com" or IP
	const char *service, // e.g. "http" or port number
	const struct addrinfo *hints,
	struct addrinfo **res);

	int ret;
	ret = getaddrinfo(server_hostname, server_port, )

	void freeaddrinfo(struct addrinfo *ai);
	int getaddrinfo(const char *restrict nodename,
    const char *restrict servname,
    const struct addrinfo *restrict hints,
    struct addrinfo **restrict res);


	return 0;
}