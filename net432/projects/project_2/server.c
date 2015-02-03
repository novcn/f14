//SERVER
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "server.h"
#include <fcntl.h>

#define CLI_MAX 128
#include <signal.h>
#include "duckchat.h"
int end = false;
channel * first_c = NULL;
super_user * first_su = NULL;
server * s = NULL;
const char * default_c = "Common";
const char * LEFT_CHANNEL = "nXbPRqYfJ6fLi91";//represents a channel the server unsubscribed to
int user_count = 0;
int channel_count = 0;
int fd_sock;
socklen_t addr_len;

const int JOIN_TIMER = 60;
volatile sig_atomic_t timer_flag = false;

//For one minute time intervals (and implicit two minute)
/////////////////////////////////////////////////////////
void sig_proc(){
  timer_flag = true;
}

//Initialize User Struct
//////////////////////////
user * init_user(user * u){
  u = (user*)malloc(sizeof(*u));
  u->next_u = NULL;
  u->port = -1;
  return u;
}

//Initialize Super User Struct
//This is a 1 dimensional list to keep track of all users
/////////////////////////////////////////////////////////
super_user * init_super(super_user * su){
  su = (super_user*)malloc(sizeof(*su));
  su->next_su = NULL;
  su->port = -1;
  return su;
}

//Initialize Channel Struct
////////////////////////////////////
channel * init_channel(channel * c){
  c = (channel*)malloc(sizeof(*c));
  c->next_c = NULL;
  c->first_u = NULL;
  c->user_count = 0;
  c->serv_subbed = false;
  return c;
} 

//Initialize Server Struct
/////////////////////////////////
server * init_server(server * s){
  s = (server*)malloc(sizeof(*s));
  s->next_s = NULL;
  s->uid_idx = 0;
  s->uids[0] = 0;
  s->channel_count = 0; 
  s->channels = (char**)malloc(sizeof(char*) * 16);
  s->channels[0] = (char*)malloc(sizeof(char*));
  memset((char*)s->channels[0], '\0', sizeof(char*));
  return s;
}

//Free super user structs
///////////////////////
void free_super(super_user * su){
  free(su);
}

//Free a channel struct
///////////////////////////////
void free_channel(channel * c){
  free(c);
}

//Free a user struct
/////////////////////////
void free_user(user * u){
  free(u);
}

//Free a server struct
/////////////////////////////
void free_server(server * s){
  free(s);
}

//Free all the structs
////////////////////////////
void free_all(){
  super_user * su = first_su;
  super_user * temp_su;
  while(su){
    temp_su = su;
    su = su->next_su;
    free_super(temp_su);
  }
  channel * c = first_c;
  user * u = NULL;
  channel * temp_c = NULL;
  user * temp_u = NULL;
  while(c){
    u = c->first_u;
    while(u){
      temp_u = u;
      u = u->next_u;
      free_user(temp_u);
    }
    temp_c = c;
    c = c->next_c;
    free_channel(temp_c);
  }
}

void alarm_handler(){
  end = true;
  free_all();
  close(fd_sock);
}

// Get 64 bit uid
//////////////////////////////////////////////////
int get_uid(){
  int f_des = open("/dev/urandom", O_RDONLY);
  int uid;
  if( (read(f_des, (char*)&uid, sizeof(uid))) < 0){
    printf("%s:%u ", s->hostname, s->port);
    fflush(stdout);
    perror("Read error dev/urandom");
  }
  close(f_des);
  return uid;
}

// Build a tree of servers using argv
/////////////////////////////////////
void build_tree(char * argv[]){
  int k = 1;
  struct hostent *h;
  s = init_server(s);
  server * cur_s = s;
  server * next = NULL;
  while(argv[k]){
    /* convert hostname to ip address */
    if((h=gethostbyname(argv[k++])) == NULL) 
      {  herror("gethostbyname"); exit(1); }
    char * ip_addr = inet_ntoa(*((struct in_addr *)h->h_addr));
    *((char*)mempcpy(cur_s->hostname, ip_addr, INET_ADDRSTRLEN)) = '\0';
    cur_s->port = (unsigned short)atoi(argv[k++]);
    // cur_s->port = argv[k++];
    // *((char*)mempcpy(cur_s->port, argv[k++], sizeof(argv[k-1]))) = '\0';
    if(argv[k]){
      next = init_server(next);
      cur_s->next_s = next;
      cur_s = next; 
    }
  }
}

// Sets up the hints and binding and socket file descriptor 
// 
// @flag, 1 for setup (file descriptor and bind) not to be done any other time
///////////////////////////////////////////////////////////////////////////
struct addrinfo * resolve_address(char * hostname, char * port, int flag){
  int rv;
  struct addrinfo hints, *result;

  memset((char *)&hints, '\0', sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // set to AF_INET to force IPv4
  hints.ai_socktype = SOCK_DGRAM;

  if ((rv = getaddrinfo(hostname, port, &hints, &result)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    fflush(stdout);
    exit(EXIT_FAILURE);
  }

  if(flag == 1){
    if( (fd_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1 ){
      printf("%s:%u ", s->hostname, s->port);
      fflush(stdout);
      perror("Socket Failed..\n");
      result = result->ai_next;
    }

    if( bind(fd_sock, result->ai_addr, result->ai_addrlen) == -1 ){
      close(fd_sock);
      result = result->ai_next;
      printf("%s:%u ", s->hostname, s->port);
      fflush(stdout);
      perror("Bind Failed..\n");
    }
  }

  if (result == NULL) {
    fprintf(stderr, "Total socket bind failure\n");
    fflush(stdout);
    exit(EXIT_FAILURE);
  }
  // freeaddrinfo(result);.--------&_&
  return result;
}

// Add a channel c1 -> ... -> cn -> NULL
//
// @channel_name, the channel we are adding 
////////////////////////////////////////////
int add_channel(char * channel_name){
  channel * c = first_c;
  int len = 0;
  if( (len = strlen(channel_name)) == 0){
    printf("Cannot create empty named channel!\n");
    fflush(stdout);
    return 0;
  }
  if(c == NULL){
    first_c = init_channel(first_c);
    *((char*)mempcpy(first_c->channel_name, channel_name, len)) = '\0';
    return 1;
  }
  while(c->next_c != NULL){
    c = c->next_c;
  }
  c->next_c = init_channel(c->next_c);
  *((char*)mempcpy(c->next_c->channel_name, channel_name, len)) = '\0'; 
  return 1;
}

// Loops through the channels to check if a channel exists
// 
// @channel_name, the channel to check existance of
// @return, 1 if successful 0 otherwise
///////////////////////////////////////////////////////////////////////
int channel_exists(char * channel_name){
  channel * c = first_c;
  if(c != NULL){
    if(strcmp(c->channel_name, channel_name) == 0){
      return 1;
    }
    while(c->next_c){
      if(strcmp(c->next_c->channel_name, channel_name) == 0){
        return 1;
      }
      c = c->next_c;
    }
  }
  return 0;
}

// Finds a channel and returns the number of users on the channel
// 
// 
///////////////////////////////////////////////////////////////////
int users_on_channel(char * channel_name){
  channel * c = first_c;
  if(c != NULL){
    if(strcmp(c->channel_name, channel_name) == 0){
      return c->user_count;
    }
    while(c->next_c){
      if(strcmp(c->next_c->channel_name, channel_name) == 0){
        return c->next_c->user_count;
      }
      c = c->next_c;
    }
  }
  return 0;
}
// Loops through the super user list to check if a user exists
// 
// @username, the user to check existance of 
// @return, 1 if successful 0 otherwise
/////////////////////////////////////////////////////////
int user_exists(char * username){
  super_user * su = first_su;
  if(su != NULL){
    if(strcmp(su->username, username) == 0){
      return 1;
    }
    while(su->next_su){
      if(strcmp(su->next_su->username, username) == 0){
        return 1;
      }
      su = su->next_su;
    }
  }
  return 0;
}

// Subscribes a user to a channel
// 
// @u the user to add; @channel_name the channel to add the user to
///////////////////////////////////////////////////////////////////////
void add_user(user * u, channel * c){
  user * cur_u = c->first_u;
  if(c->first_u == NULL){
    c->first_u = u; 
  }else{
    while(cur_u->next_u){
      cur_u = cur_u->next_u;
    }
    cur_u->next_u = u;
  }
  c->user_count++;
}

// Adds a user to the super list 
// 
// @su, the user to add
/////////////////////////////////
void add_super(super_user * su){
  if(first_su == NULL){
    first_su = su;
  }
  super_user * cur = first_su;
  while(cur->next_su != NULL){
    cur = cur->next_su;
  }
  cur->next_su = init_super(cur->next_su);
  cur->next_su = su;
  su->next_su = NULL;
}

// Finds a su_user (by ip & port) in the super user list, returns user
// 
// @sock the socket containing ip and port info
// @user the returned user, data coppied from super user list
////////////////////////////////////////////////////////////////////
user * get_user(struct sockaddr_in * sock){
  super_user * su = NULL;
  su = init_super(su);
  su = first_su;
  user * u = NULL;
  u = init_user(u);
  int len = 0;
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, ip, INET_ADDRSTRLEN);    
  int port = ntohs(sock->sin_port);

  if(su->port == port && (strcmp(su->ip, ip) == 0 )){
    u->port = port;
    len = strlen(su->username);
    *((char*)mempcpy(u->username, su->username, len)) = '\0';
    len = strlen(su->ip);
    *((char*)mempcpy(u->ip, su->ip, len)) = '\0'; 
    u->sock = su->sock;
  } else {
    while(su->next_su != NULL){
      if(su->next_su->port == port && (strcmp(su->ip, ip) == 0)){
        u->port = port;
        len = strlen(su->next_su->username);
        *((char*)mempcpy(u->username, su->next_su->username, len)) = '\0';
        len = strlen(ip);
        *((char*)mempcpy(u->ip, ip, len)) = '\0'; 
        u->sock = su->next_su->sock;        
      }
      su = su->next_su;
    }
  }
  if(u == NULL){
    printf("Error finding user from super list!\n");
    fflush(stdout);
  } 
  return u;
}

// Finds a channel and then calls add_user
// 
// @u, the user to add to the channel; @channel_name chan to add user to
// @return, 1 if succesfull 0 if otherwise
////////////////////////////////////////////////////////////////////////
int find_channel(user * u, char * channel_name){
  channel * c = first_c;
  if(!c){
    return 0;
  }else if(strcmp(c->channel_name, channel_name) == 0){
    add_user(u, c);
    return 1;
  }
  while(c->next_c != NULL){
    if(strcmp(c->next_c->channel_name, channel_name) == 0){
      add_user(u, c->next_c);
      return 1;
    }
    c = c->next_c;
  }
  return 0;
}

// Remove and free a channel
// 
// @c not the channel we are removing, but the one before it
/////////////////////////////////////////////////////////////
void remove_channel(channel * c){
  channel * temp_c;
  temp_c = c->next_c;
  if(temp_c != NULL){
    c->next_c = c->next_c->next_c;
  }
  free_channel(temp_c);
  return;
}

// Broadcast say message to every user in the channel
// 
// @c the channel to broadcast to; @username the name of the user
// broadcasting the message; @text_say the content of the message
/////////////////////////////////////////////////////////////////
void broadcast_say(channel * c, char * username, char * text_say){
  user * u = c->first_u;
  struct text_say t;
  t.txt_type = TXT_SAY;
  int len = strlen(c->channel_name);
  *((char*)mempcpy(t.txt_channel, c->channel_name, len)) = '\0';
  len = strlen(username);
  *((char*)mempcpy(t.txt_username, username, len)) = '\0';
  len = strlen(text_say);
  *((char*)mempcpy(t.txt_text, text_say, len)) = '\0';
  if(sendto(fd_sock, &t, sizeof(struct text_say), 0, (struct sockaddr *) &u->sock, addr_len) == -1) {
    printf("%s:%u ", s->hostname, s->port);
    fflush(stdout);
    perror("Send text say: ");
    exit(EXIT_FAILURE);
  } 
  while(u->next_u != NULL){
    if(sendto(fd_sock, &t, sizeof(struct text_say), 0, (struct sockaddr *) &u->next_u->sock, addr_len) == -1) {
      printf("%s:%u ", s->hostname, s->port);
      fflush(stdout);
      perror("Send text say: ");
      exit(EXIT_FAILURE);
    } 
    u = u->next_u;
  }
}

// Checks the uids in the server array to see if there are 
// duplicates
// this works fine for int but not double type see 
// ./ext/double_rand_failure.c for more info
///////////////////////////////////////////////////////////
int detect_loop(int uid){
  int k = 0;
  while(s->uids[k]){
    if(uid == s->uids[k++]){
      return 1;
    }
  }
  return 0;
}

// Checks to see if the server is subscribed to current channel
// 
// @ channel_name, the channel which the server may be subbed to
////////////////////////////////////////////////////////////////
int server_subbed(char * channel_name){
  channel * c = first_c;
  if(!c){
    printf("This should not happen!\n");
    return 0;
  }else if(strcmp(c->channel_name, channel_name) == 0){
    if(c->serv_subbed == true){
      return 1;
    } else {  
      c->serv_subbed = true;
      return 0; 
    }
  }
  while(c->next_c != NULL){
    if(strcmp(c->next_c->channel_name, channel_name) == 0){
      if(c->next_c->serv_subbed == true){
        return 1;
      } else {  
        c->next_c->serv_subbed = true;
        return 0; 
      }
    }
    c = c->next_c;
  }
  return 0;
}

// Sends s2s subscribe packets to all adjacent servers 
// except the server that sent the incoming message
// 
// @sock, the socket of the incoming client or adjacent server
///////////////////////////////////////////////////////////
void subscribe_adjacent_servers(char * channel_name, struct sockaddr_in * sock){
  server * cur_s = s->next_s;
  struct s2s_join s2s;
  s2s.s2s_type = S2S_JOIN;
  int len = strlen(channel_name);

  char recv_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, recv_ip, INET_ADDRSTRLEN);    

  *((char*)mempcpy(s2s.s2s_channel, channel_name, len)) = '\0';

  while(cur_s != NULL){
    int in_port = ntohs(sock->sin_port);
    unsigned short port = cur_s->port;
    if(port != in_port || (strncmp(cur_s->hostname, recv_ip, INET_ADDRSTRLEN) != 0)){
      printf("a%s:%u %s:%u send S2S Join %s\n", s->hostname, s->port, cur_s->hostname, cur_s->port, channel_name);
      fflush(stdout);
      int idx = 0;
      while((strcmp(cur_s->channels[idx], "") != 0) && (strcmp(cur_s->channels[idx], LEFT_CHANNEL) != 0)) 
      { idx++; }/* find an empty index */ 

      cur_s->channel_count++;
      *((char*)mempcpy(cur_s->channels[idx], channel_name, len)) = '\0';
      if(strcmp(cur_s->channels[idx], "") == 0){
        cur_s->channels[idx+1] = (char*)malloc(sizeof(char*));
        memset(cur_s->channels[idx+1], '\0', sizeof(char*));
      }
      char port[8];
      sprintf(port, "%u", cur_s->port);
      fflush(stdout);
      struct addrinfo * adj_serv = resolve_address(cur_s->hostname, port, 0);
      if(sendto(fd_sock, &s2s, sizeof(s2s), 0, adj_serv->ai_addr, addr_len) == -1) {
        printf("%s:%u ", s->hostname, s->port);
        fflush(stdout);
        perror("Subscribe adjacent servers:");
      }
    }
    cur_s = cur_s->next_s;
  }
}

// The resending of joins which happens every minute to ensure that network
// failures do not ruin our server topology
// 
//////////////////////////////////////////////////////////////////////////
void resend_join_servers(char * channel_name, struct sockaddr_in * sock){
  server * cur_s = s->next_s;
  struct s2s_join s2s;
  s2s.s2s_type = S2S_JOIN;
  int len = strlen(channel_name);

  char recv_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, recv_ip, INET_ADDRSTRLEN);    

  *((char*)mempcpy(s2s.s2s_channel, channel_name, len)) = '\0';

  while(cur_s != NULL){
    int in_port = ntohs(sock->sin_port);
    unsigned short port = cur_s->port;
    if(port != in_port || (strncmp(cur_s->hostname, recv_ip, INET_ADDRSTRLEN) != 0)){
      printf("a%s:%u %s:%u send S2S Join %s\n", s->hostname, s->port, cur_s->hostname, cur_s->port, channel_name);
      fflush(stdout);

      char port[8];
      sprintf(port, "%u", cur_s->port);
      fflush(stdout);
      struct addrinfo * adj_serv = resolve_address(cur_s->hostname, port, 0);
      if(sendto(fd_sock, &s2s, sizeof(s2s), 0, adj_serv->ai_addr, addr_len) == -1) {
        printf("%s:%u ", s->hostname, s->port);
        fflush(stdout);
        perror("Subscribe adjacent servers:");
      }
    }
    cur_s = cur_s->next_s;
  }
}


// Send a leave packet to a previous server since this server
// is a leaf and it is not subscribed to the channel
// 
/////////////////////////////////////////////////////////////
void leave_to_server(char * channel_name, struct sockaddr_in * sock){
  struct s2s_leave s2s;
  s2s.s2s_type = S2S_LEAVE;
  int len = strlen(channel_name);
  *((char*)mempcpy(s2s.s2s_channel, channel_name, len)) = '\0';
  unsigned short port = ntohs(sock->sin_port);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, ip, INET_ADDRSTRLEN);    

  printf("b%s:%u %s:%u send S2S Leave %s\n", s->hostname, s->port, ip, port, channel_name);
  fflush(stdout);
  if(sendto(fd_sock, &s2s, sizeof(s2s), 0, (struct sockaddr *) sock, addr_len) == -1) {
    printf("%s:%u ", s->hostname, s->port);
    fflush(stdout);
    perror("Leave to prev server:");
  }
}

// Sends s2s say packets to all adjacent servers except the server that sent the incoming message
// 
// 
/////////////////////////////////////////////////////////////////////////////////////////////////
void say_to_server(char * channel_name, char * username, char * text, struct sockaddr_in * sock, struct s2s_say s2s){
  server * cur_s = s->next_s;
  int len = 0;

  //need to check it against our uid. how to NOT check when your are from client say?
  if(strcmp(s2s.s2s_channel, "") == 0 || strcmp(s2s.s2s_username, "") == 0 || strcmp(s2s.s2s_text, "") == 0){
    len = strnlen(channel_name, CHANNEL_MAX);
    *((char*)mempcpy(s2s.s2s_channel, channel_name, len)) = '\0';
    len = strnlen(username, USERNAME_MAX);
    *((char*)mempcpy(s2s.s2s_username, username, len)) = '\0';
    len = strnlen(text, SAY_MAX);
    *((char*)mempcpy(s2s.s2s_text, text, len)) = '\0';
  }
  
  char recv_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, recv_ip, INET_ADDRSTRLEN);    

  while(cur_s != NULL){
    short in_port = ntohs(sock->sin_port);
    // We send to all adjacent servers except for the server we received from
    if(cur_s->port != in_port || (strncmp(cur_s->hostname, recv_ip, INET_ADDRSTRLEN) != 0)){
      // printf("Diff: %u vs %u & %s %s\n", cur_s->port, in_port, cur_s->hostname, recv_ip);
      printf("b%s:%u %s:%u send S2S say %s %s \"%s\"\n", s->hostname, s->port, cur_s->hostname, 
                                                     cur_s->port, username, channel_name, text);
      fflush(stdout);
      char port[8];
      sprintf(port, "%u", cur_s->port);
      fflush(stdout);
      struct addrinfo * adj_serv = resolve_address(cur_s->hostname, port, 0);
      
      if(sendto(fd_sock, &s2s, sizeof(s2s), 0, adj_serv->ai_addr, addr_len) == -1) {
        printf("%s:%u ", s->hostname, s->port);
        fflush(stdout);
        perror("Say to adjacent servers:");
      }
    }
    cur_s = cur_s->next_s;
  }
}

// Finds the channel to brodcast a say message to
// 
// @c the channel name to broadcast to; @username the name of the user
// broadcasting the message; @text_say the content of the message
///////////////////////////////////////////////////////////////////////////
void find_say_channel(char * channel_name, char * username, char * text_say){
  channel * c = first_c;
  if(!c){
    return;//this occurs if the server is without users but maintaining topology
  }else if(strcmp(c->channel_name, channel_name) == 0 && c->user_count > 0){
    broadcast_say(c, username, text_say);
    return;
  }
  while(c->next_c != NULL){
    if(strcmp(c->next_c->channel_name, channel_name) == 0 && c->next_c->user_count > 0){
      broadcast_say(c->next_c, username, text_say);
      return;
    }
    c = c->next_c;
  }
}

// Removes a user from the channel list, they are no longer 
// subscribed
// @channel_name, the channel the user is leaving
/////////////////////////////////////////////////////////
void user_leave_channel(char * channel_name, struct sockaddr_in * sock){

  user * temp_u = get_user(sock);
  channel * c = first_c;
  printf("Leave %s %s\n", channel_name, temp_u->username);
  fflush(stdout);
  if(!c){
    printf("channel user attempted to leave does not exist\n");
    fflush(stdout);
    return;
  }else if(strcmp(c->channel_name, channel_name) == 0){ //first channel
    user * departing_user = get_user(sock);
    user * u = c->first_u;
    if(strcmp(departing_user->username, u->username) == 0){
      c->first_u = u->next_u;
      c->user_count--;
      if(c->user_count == 0){
        remove_channel(c);
      }
      return;
    } else {
      while(u->next_u != NULL){
        if(strcmp(departing_user->username, u->username) == 0){
          temp_u = u->next_u;
          u->next_u = u->next_u->next_u;
          free(temp_u);
          printf("server: %s has left channel %s\n", departing_user->username, channel_name);
          fflush(stdout);
          c->user_count--;
          if(c->user_count == 0){
            remove_channel(c);
          }
          return;
        }
        u = u->next_u;
      }
    }
  }else{
    while(c->next_c != NULL){
      if(strcmp(c->next_c->channel_name, channel_name) == 0){
        user * departing_user = get_user(sock);
        user * u = c->next_c->first_u;
        if(strcmp(departing_user->username, u->username) == 0){
          temp_u = c->next_c->first_u;
          c->next_c->first_u = u->next_u;
          free_user(temp_u);
          c->next_c->user_count--;
          if(c->next_c->user_count == 0){
            remove_channel(c);
          }
          return;
        } else {
          while(u->next_u != NULL){
            if(strcmp(departing_user->username, u->username) == 0){
              temp_u = u->next_u;
              u->next_u = u->next_u->next_u;
              free_user(temp_u);
              printf("server: %s has left channel %s\n", departing_user->username, channel_name);
              fflush(stdout);
              c->next_c->user_count--;
              if(c->next_c->user_count == 0){
                remove_channel(c);
              }
              return;
            }
            u = u->next_u;
          }
        }
      }
      c = c->next_c;
    }
  }
}

// Returns a channel struct with the specified channel_name
// 
// 
///////////////////////////////////////////////////////////
channel * get_channel(char * channel_name){
  channel * c = first_c;
  if(!c){
    printf("Error, channel %s not found!\n", channel_name);
    return NULL;
  }
  if(strcmp(c->channel_name, channel_name) == 0){
    return c;
  } else {
    while(c->next_c){
      if(strcmp(c->next_c->channel_name, channel_name) == 0){
        return c->next_c;
      }
    }
  }
  printf("Error, channel %s not found!\n", channel_name);
  return NULL;
}

// Handles the login packets and joins user to channel Common
//
// @req request packet, @sock the user specific sock address
/////////////////////////////////////////////////////////////////
void handle_login(struct request * req, struct sockaddr_in * sock){
  struct request_login r = *(struct request_login *)req;
  if(user_count >= UCOUNT_MAX){
    printf("server: the maximum number of users are logged in right now \n");
    fflush(stdout);
    return;
  }
  if(user_exists(r.req_username)){
    printf("server: %s tried to login. Already logged in.\n", r.req_username);
    fflush(stdout);
    return;
  }

  printf("Request login %s\n", r.req_username);
  fflush(stdout);

  user_count++;
  super_user * su = NULL;
  su = init_super(su);
  *((char*)mempcpy(su->username, r.req_username, sizeof(r.req_username))) = '\0';
  su->sock = *sock;
  su->port = ntohs(sock->sin_port);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, ip, INET_ADDRSTRLEN);    
  *((char*)mempcpy(su->ip, ip, strlen(ip))) = '\0';

  add_super(su);
}

// Handles the loging out of a user by calling leave on each subscribed
// Channel and then removing the user from the super_user list
// 
////////////////////////////////////////////////////////////////////
void handle_logout(struct sockaddr_in * sock){
  channel * c = first_c;

  user * u = get_user(sock);

  //call leave on each channel so that user leaves all channels they are subscribed to
  if(c){
    user_leave_channel(c->channel_name, sock);
  }
  while(c->next_c != NULL){
    user_leave_channel(c->next_c->channel_name, sock);
    c = c->next_c;
  }

  super_user * su = first_su;
  if(strcmp(su->username, u->username) == 0){
    first_su = su->next_su;
  } else {
    if(strcmp(su->next_su->username, u->username) == 0){
      su->next_su = su->next_su->next_su;
    }
  }
}


// Handles the joining of a user to a channel               
//                                                          
// @req request packet, @sock the user specific sock address
//////////////////////////////////////////////////////////////
void handle_join(struct request * req, struct sockaddr_in * sock){
  user * u = NULL;
  u = get_user(sock);
  struct request_join r = *(struct request_join *)req;
  char * channel_name = r.req_channel;
  
  printf("Request join %s %s\n", u->username, channel_name);
  fflush(stdout);

  if(!channel_exists(channel_name)){
    channel_count++;
    add_channel(channel_name);
  } 
  
  if(!find_channel(u, channel_name)){ //find and add channel
    printf("Error adding user to channel %s\n", channel_name); 
    fflush(stdout);
  }

  channel * c = get_channel(channel_name);
  c->stale_channel = 0;

  if(!server_subbed(channel_name)){
    subscribe_adjacent_servers(channel_name, sock);
  }
}

// Handles the sending of say messages to all users in channel
// 
// @req request packet, @sock the user specific sock address
//////////////////////////////////////////////////////////////
void handle_say(struct request * req, struct sockaddr_in * sock){

  user * u = NULL;
  u = get_user(sock);
  struct request_say r = *(struct request_say *)req;
  char * channel_name = r.req_channel;
  char * text_say = r.req_text;

  printf("Request say %s \"%s\"\n", channel_name, text_say);
  fflush(stdout);

  if(!channel_exists(channel_name)){
    printf("Attempted to say something in a nonexistant channel!\n");
    fflush(stdout);
    return;
  }

  find_say_channel(channel_name, u->username, text_say);

  struct s2s_say s2s;
  s2s.s2s_type = S2S_SAY;
  s2s.uid = get_uid();
  say_to_server(channel_name, u->username, text_say, sock, s2s);
}

// Hanles the request to list all the channels and brodcast to user
// 
// @req request packet, @sock the user specific sock address
/////////////////////////////////////////////////////////
void handle_list(struct sockaddr_in * sock){

  int t_size = sizeof(struct text_list) + (sizeof(struct user_info) * channel_count);
  struct text_list * t = (struct text_list *)malloc(t_size);
  memset(t, '\0', t_size);
  t->txt_type = TXT_LIST;
  t->txt_nchannels = channel_count;

  printf("List\n");
  fflush(stdout);

  channel * c = first_c;
  if(c == NULL){
    return;
  }
  int len = 0;
  int k = 0;
  while(c != NULL){
    len = strlen(c->channel_name);
    *((char*)mempcpy(t->txt_channels[k++].ch_channel, c->channel_name, len)) = '\0';
    c = c->next_c;
  }
  if(sendto(fd_sock, t, t_size, 0, (struct sockaddr *) sock, addr_len) == -1) {
    printf("%s:%u ", s->hostname, s->port);
    fflush(stdout);
    perror("Send list text: ");
    exit(EXIT_FAILURE);
  } 
} 

// Handles a client leaving a channel on the server
// 
// @req the packet with the leave information @sock the clients addrinfo 
////////////////////////////////////////////////////////////////////////
void handle_leave(struct request * req, struct sockaddr_in * sock){
  struct request_leave r = *(struct request_leave *)req;
  char * channel_name = r.req_channel;
  user_leave_channel(channel_name, sock);
}

// Handles the request to list all users on a specified channel 
// 
// @req request packet, @sock the user specific sock address
/////////////////////////////////////////////////////////////////
void handle_who(struct request * req, struct sockaddr_in * sock){
  int len = 0;
  struct request_who r = *(struct request_who *)req;
  int t_size = 0;
  printf("Who %s\n", r.req_channel);
  fflush(stdout);
  struct text_who * t = NULL;
  int j = 1;
  int k = 0;
  channel * c = first_c;
  while(c != NULL){
    if(strcmp(c->channel_name, r.req_channel) == 0){
      t_size = sizeof(struct text_list) + (sizeof(struct user_info) * (c->user_count + 1));
      t = (struct text_who *)malloc(t_size);
      t->txt_type = TXT_WHO;
      len = strlen(r.req_channel);
      *((char*)mempcpy(t->txt_channel, r.req_channel, len)) = '\0';
      user * u = c->first_u;
      len = strlen(u->username);
      *((char*)mempcpy(t->txt_users[0].us_username, u->username, len)) = '\0';
      while(u->next_u != NULL){
        len = strlen(u->next_u->username);
        *((char*)mempcpy(t->txt_users[j].us_username, u->next_u->username, len)) = '\0';
        j++;
        u = u->next_u;
      }
    }
    c = c->next_c;
    k++;
  }
  t->txt_nusernames = j;

  if(sendto(fd_sock, t, t_size, 0, (struct sockaddr *) sock, addr_len) == -1) {
    printf("%s:%u ", s->hostname, s->port);
    fflush(stdout);
    perror("Send to txt who: ");
    exit(EXIT_FAILURE);
  } 
}

// Handles the joining of a server to a channel, as well as forwarding 
// 
// @req request packet, @sock the user specific sock address
////////////////////////////////////////////////////////////////////////
void server_handle_join(struct request * req, struct sockaddr_in *sock){
  struct s2s_join s2s = *(struct s2s_join *)req;
  char recv_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, recv_ip, INET_ADDRSTRLEN);  
  printf("S2S Join %s\n", s2s.s2s_channel);

  if(!channel_exists(s2s.s2s_channel)){
    channel_count++;
    add_channel(s2s.s2s_channel);
  } 

  channel * c = get_channel(s2s.s2s_channel);
  c->stale_channel = 0;

  if(!server_subbed(s2s.s2s_channel)){
    fflush(stdout);
    subscribe_adjacent_servers(s2s.s2s_channel, sock); //this is the main fucntion
  }
}

// Handles the leave packet of a server sent from another server
// 
// 
//////////////////////////////////////////////////////////////////////////
void server_handle_leave(struct request * req, struct sockaddr_in *sock){
  struct s2s_leave s2s = *(struct s2s_leave*)req;
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, ip, INET_ADDRSTRLEN);
  int port = ntohs(sock->sin_port);
  printf("S2S Leave %s\n", s2s.s2s_channel);

  server * cur_s = s->next_s;
  if(s->port == port && strncmp(s->hostname, ip, INET_ADDRSTRLEN) == 0){
    printf("Error, current server trying to leave channel\n");
    fflush(stdout);
  }
  int idx = 0;
  while(cur_s){
    if(cur_s->port == port && strncmp(cur_s->hostname, ip, INET_ADDRSTRLEN) == 0){
      while(idx < cur_s->channel_count){
        if(strncmp(cur_s->channels[idx], s2s.s2s_channel, (strlen(cur_s->channels[idx]))) == 0){
          int len = strlen(LEFT_CHANNEL);
          *((char*)mempcpy(cur_s->channels[idx], LEFT_CHANNEL, len)) = '\0';
          fflush(stdout);
          return;
        }
        idx++;
      }
    }
    cur_s = cur_s->next_s;
  }

}

// Handles the incoming say packet sent from another server
// 
// @req the packet containing say info, @sock the socket with the incoming server info
///////////////////////////////////////////////////////////////////////////////////////
void server_handle_say(struct request * req, struct sockaddr_in *sock){
  struct s2s_say s2s = *(struct s2s_say*)req;
  char recv_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, recv_ip, INET_ADDRSTRLEN);  

  printf("S2S say %s %s \"%s\"\n", s2s.s2s_username, s2s.s2s_channel, s2s.s2s_text);
  fflush(stdout);

  if(detect_loop(s2s.uid)){
    leave_to_server(s2s.s2s_channel, sock);
    return;
  }

  find_say_channel(s2s.s2s_channel, s2s.s2s_username, s2s.s2s_text);
  if(!s->next_s->next_s && !users_on_channel(s2s.s2s_channel)){ //this is a leaf
    leave_to_server(s2s.s2s_channel, sock);
    return;
  }

  if(!server_subbed(s2s.s2s_channel)){
    subscribe_adjacent_servers(s2s.s2s_channel, sock);
  }
  //check uid
  if(!detect_loop(s2s.uid)){
    say_to_server(s2s.s2s_channel, s2s.s2s_username, s2s.s2s_text, sock, s2s);
    s->uids[s->uid_idx++] = s2s.uid;
  } 
}

// Does all the switching from incoming packets to determine which handler to use
// based on the type of packet
// 
/////////////////////////////////////////////////////////////////////////////////
void request_handler(struct sockaddr_in * addr_in, char * hostname, char * port){
  struct request * req = (struct request*)malloc(sizeof(struct request*) + CLI_MAX);

  if ((recvfrom(fd_sock, req, CLI_MAX, 0, (struct sockaddr *) addr_in, &addr_len)) == -1) {
    printf("%s:%u ", s->hostname, s->port);
    fflush(stdout);
    perror("error recvfrom:");
    exit(EXIT_FAILURE);
  }

  char cli_ip[INET_ADDRSTRLEN];
  int cli_port = ntohs(addr_in->sin_port);
  inet_ntop(AF_INET, &addr_in->sin_addr, cli_ip, INET_ADDRSTRLEN);    

  struct hostent *h;
  if((h=gethostbyname(hostname)) == NULL) 
    {  herror("gethostbyname"); exit(1); }
  char * ip_addr = inet_ntoa(*((struct in_addr *)h->h_addr));
  *((char*)mempcpy(hostname, ip_addr, INET_ADDRSTRLEN)) = '\0';

  printf("c%s:%s %s:%d recv ", hostname, port, cli_ip, cli_port);
  fflush(stdout);

  switch(req->req_type){
    case REQ_LOGIN:{/////////// LOGIN ////////// LOGIN ////////// LOGIN ///////////////
      handle_login(req, addr_in);
      break;
    }
    case REQ_LOGOUT:{///////// LOGOUT ///////// LOGOUT ///////// LOGOUT ///////////////
        handle_logout(addr_in);
        break;
    }
    case REQ_JOIN:{//////////// JOIN /////////// JOIN /////////// JOIN ////////////////
      handle_join(req, addr_in);
      break;
    }
    case REQ_LEAVE:{///////// LEAVE ///////// LEAVE ///////// LEAVE ///////////////////
      fflush(stdout);
      handle_leave(req, addr_in);
      break;
    }
    case REQ_SAY:{///////// SAY //////////// SAY //////////// SAY /////////////////////
      handle_say(req, addr_in);
      break;
    }
    case REQ_LIST:{///////// LIST /////////// LIST /////////// LIST ///////////////////
      handle_list(addr_in);
      break;
    }
    case REQ_WHO:{///////// WHO ///////////// WHO ///////////// WHO ///////////////////
      handle_who(req, addr_in);
      break;
    }
    case S2S_JOIN:{///////// S2S_JOIN /////////// S2S_JOIN ////////// S2S_JOIN ////////
      server_handle_join(req, addr_in);
      break;
    }
    case S2S_LEAVE:{///////// S2S_LEAVE /////////// S2S_LEAVE ////////// S2S_LEAVE ////
      server_handle_leave(req, addr_in);
      break;
    }
    case S2S_SAY:{///////// S2S_SAY /////////// S2S_SAY ////////// S2S_SAY ////////////
      server_handle_say(req, addr_in);
      break;
    }
    default:{
      printf("Invalid packet\n");
      fflush(stdout);
    }
  }
  free(req);
}

// Sends joins to all adjacent servers with all channels that server is currently on
// 
// 
//////////////////////////////////////////////////////////////////////////////////////
void rebuild(struct sockaddr_in * addr_in){
  channel * c = first_c;
  if(!c){
    return;
  }
  resend_join_servers(c->channel_name, addr_in);
  while(c->next_c != NULL){
    resend_join_servers(c->next_c->channel_name, addr_in);
    c = c->next_c;
  }
}

// Floods the leave message to all adjacent servers. This happens when
// a channel becomes "stale" on the server, due to no join messages in 
// the last 2 minutes
////////////////////////////////////////////////////////////////////
void flood_leave(char * channel_name, struct sockaddr_in * sock){
  server * cur_s = s->next_s;
  struct s2s_leave s2s;
  s2s.s2s_type = S2S_LEAVE;
  int len = strlen(channel_name);

  char recv_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sock->sin_addr, recv_ip, INET_ADDRSTRLEN);    
  *((char*)mempcpy(s2s.s2s_channel, channel_name, len)) = '\0';

  while(cur_s != NULL){
    int in_port = ntohs(sock->sin_port);
    unsigned short port = cur_s->port;
    if(port != in_port || (strncmp(cur_s->hostname, recv_ip, INET_ADDRSTRLEN) != 0)){
      printf("a%s:%u %s:%u send S2S Leave %s\n", s->hostname, s->port, cur_s->hostname, cur_s->port, channel_name);
      fflush(stdout);

      char port[8];
      sprintf(port, "%u", cur_s->port);
      fflush(stdout);
      struct addrinfo * adj_serv = resolve_address(cur_s->hostname, port, 0);
      if(sendto(fd_sock, &s2s, sizeof(s2s), 0, adj_serv->ai_addr, addr_len) == -1) {
        printf("%s:%u ", s->hostname, s->port);
        fflush(stdout);
        perror("Subscribe adjacent servers:");
      }
    }
    cur_s = cur_s->next_s;
  }
}

// Looks at each channel to see if they have been joined 
// within the last 2 minutes, if they have not a flood leave is init
//
///////////////////////////////////////////////////////////////////
void check_stale_channels(struct sockaddr_in * sock){
  channel * cur_c = first_c;
  if(!cur_c){
    return;
  }
  channel * c = get_channel(cur_c->channel_name);
  c->stale_channel++;
  if(c->stale_channel == 2){
    flood_leave(c->channel_name, sock);
  }
  while(cur_c->next_c){
    c = get_channel(cur_c->next_c->channel_name);
    c->stale_channel++;
    if(c->stale_channel == 2){
      flood_leave(c->channel_name, sock);
    }
  }
}

int main(int argc, char * argv[]){

  signal(SIGINT, alarm_handler);
  signal(SIGTSTP, alarm_handler);
  struct sockaddr_in * addr_in = (struct sockaddr_in *)malloc(sizeof(*addr_in));
  memset(addr_in, '\0', sizeof(*addr_in));
  addr_len = sizeof(*addr_in);

  signal(SIGALRM, sig_proc);
  alarm(JOIN_TIMER);
  fd_set readfds;
  int sock_fd = 0;

  if(argc < 3 || (argc % 2) == 0 ){//argc cannot be even
    errno = EINVAL; //invalid arguements
    fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
    fflush(stdout);
    exit(EXIT_FAILURE);
  } else 
  {  build_tree(argv);  }

  char * hostname = argv[1];
  char port[32]; 
  *((char*)mempcpy(port, argv[2], strlen(argv[2]))) = '\0';

  resolve_address(hostname, argv[2], 1); 

  do {
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(fd_sock, &readfds);
    select(fd_sock+1, &readfds, NULL, NULL, NULL);
    
    if(FD_ISSET(fd_sock, &readfds)){
      //Handles the recvfrom and the packet type switch statement
      request_handler(addr_in, hostname, port);
    }
    else if(FD_ISSET(0, &readfds)){
      fd_set sec;
      struct timeval t;
      FD_ZERO(&sec);
      FD_SET(sock_fd, &sec);
      t.tv_sec = 1; 
      t.tv_usec = 0;
      if(timer_flag == true){
        timer_flag = false;
        check_stale_channels(addr_in);
        rebuild(addr_in);
        fflush(stdout);
        alarm(JOIN_TIMER);
      }
    }
  } while(!end);

  printf("Exiting...\n");
  free(addr_in);
  free_all();
  close(fd_sock);
  return 0;
}