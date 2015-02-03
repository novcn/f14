//SERVER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define MYPORT "4950" // THIS WILL BE RM
#define MAXBUFLEN 100 //RM THIS TOO

#include <signal.h>
#include "duckchat.h"
int end = false;

void alarm_handler(){
  end = true;
}


//RM THIS TOO
// get sockaddr, IPv4 or IPv6:

int main(int argc, char * argv[]){
  int fd_sock;
  struct addrinfo hints, *result, *_result;
  int rv;
  int numbytes;
  struct sockaddr_storage client_addr;
  char buf[MAXBUFLEN];
  socklen_t addr_len;
  char s[INET6_ADDRSTRLEN];
  signal(SIGINT, alarm_handler);


  if(argc != 3){
    errno = EINVAL; //invalid arguements
    fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
    exit(EXIT_FAILURE);
  } 

  char * hostname = argv[1];
  char * port = argv[2];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; // set to AF_INET to force IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(hostname, port, &hints, &result)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  _result = result;
  while(_result != NULL){
    if( (fd_sock = socket(_result->ai_family, _result->ai_socktype, _result->ai_protocol)) == -1 ){
      perror("Socket Failed..\n");
      _result = _result->ai_next;
      continue;
    }
    if( bind(fd_sock, _result->ai_addr, _result->ai_addrlen) == -1 ){
      close(fd_sock);
      _result = _result->ai_next;
      perror("Bind Failed..\n");
      continue;
    }
    break;


  }

  if (_result == NULL) {
    fprintf(stderr, "Total socket bind failure\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);

  printf("Waiting for client to connect...\n");


  addr_len = sizeof client_addr;


  if ((numbytes = recvfrom(fd_sock, buf, MAXBUFLEN-1 , 0,
   (struct sockaddr *) &client_addr, &addr_len)) == -1) {
    perror("error recvfrom:");
    exit(EXIT_FAILURE);
  }

  printf("recieved packet\n");
  printf("listener: packet is %d bytes long\n", numbytes);
  buf[numbytes] = '\0';
  printf("listener: packet contains \"%s\"\n", buf);


    //look at packet and copy first 32 bits 
    //have a switch or if/else structure based on the packets request_type
    //we may need to do some casting from a standard request to request_<TYPE>
    //I don't really understand it full, ADAM knows,
    //this is how we cast from standard to one of type login:
    //struct request_login * req = (struct request_login *) req;
    //then based on the type we will harvest it for it's data e.g. in request_login
    //we would get the username field next and then call a function to add the user
    //to the userlist and printf the status message. This is all similar to line 286...
    //in the client.c




  while(!end){}//this is for testing, catches control c to make sure close(socket) is called
  close(fd_sock);

  return 0;
}