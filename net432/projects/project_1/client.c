#include <sys/types.h>
#include <wchar.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>
#include "client.h"
#include "duckchat.h"
#include "raw.h"
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t end = false;
#define SER_MAX 1024

const char * EXIT = "/exit";
const char * SWITCH = "/switch";
const char * JOIN = "/join";
const char * LEAVE = "/leave";
const char * LIST = "/list";
const char * WHO = "/who";

char line[SAY_MAX + 1];
char *c_pos = line;
/**
   *Signal handler allowing us to still free() when using SIGINT
 *to exit program
 */
//void alarm_handler(){
  //end = true;
//}

/**
 *Initializes a channel and returns it
 *
 *@param c, the channel to be initialized
 *@return c, the initialized channel
 */
user * init_user(user * u){
  u = (user*)malloc(sizeof(*u));
  u->first_channel = NULL;
  // u->first_channel = init_channel(c);
  u->channels_subd = 0;
  return u;
}

channel * init_channel(channel * c){
  c = (channel*)malloc(sizeof(*c));
  c->next = NULL;
  return c;
}

/**
 *Frees each channel in subscribed list and the frees the user
 *
 *@param u, the user to be freed 
 *@param c, the channel to be freed
 */
void free_user(user * u, channel * c){
  c = u->first_channel;
  while( c ){
    channel * temp = c;
    c = c->next;
    free(temp);
  } 
  free(u);
}

void free_channel(channel * c){
  if(!c){
    printf("Error: Trying to free an unallocated channel!\n");
  }
  free(c);
}

/**
 *THIS IS FOR TESTING ONLY =========================================================
 */
void print_channels(user * u){
  struct channel * current = u->first_channel;
  if(current){
    printf("\'%s\'\n", current->channel_name);
    while(current->next){
      printf("\'%s\'\n", current->next->channel_name);
      current = current->next;
    }
  }
}

/**
 *Switches a users active channel by iterating through the subscribed channel list
 *
 *@param channel_name, the name of a channel received after the /switch command
 *@return, returns 0 if the channel was not in the list of subscribed channels
 */
int switch_channel(user * u, char * channel_name){
  int len = strlen(channel_name);
  channel * current = u->first_channel;
  if(!current){
    return 0;
  } else if(strcmp(u->active_channel, channel_name) == 0){
    printf("Already active on channel %s", channel_name);
    return 1;
  } else if(strcmp(current->channel_name, channel_name) == 0){
    *((char*)mempcpy(u->active_channel, current->channel_name, len)) = '\0';
    return 1;
  }
  while(current->next){
    if(strcmp(current->next->channel_name, channel_name) == 0){
      *((char*)mempcpy(u->active_channel, channel_name, len)) = '\0';
      return 1;
    }
    current = current->next;
  }
  return 0;
}

/**
 *add a channel to a users subscribed to list
 *
 *@param u, the user subscribing to the channel
 *@param channel_name, the name of the channel user is subscribing to
 */
int subscribe_channel(user * u, char * chan_name){
  int len = strlen(chan_name);
  struct channel * new_c = NULL;
  struct channel * current = u->first_channel;
  if(!current){
    u->first_channel = init_channel(u->first_channel);
    *((char*)mempcpy(u->first_channel->channel_name, chan_name, len)) = '\0';
    u->channels_subd++;
    return 0;
  }
  else if(strcmp(current->channel_name, chan_name) == 0){
    return 0;
  } else {
    while(current->next){
      if(strcmp(current->next->channel_name, chan_name) == 0)
        return 0;
      current = current->next;
    }
    new_c = init_channel(new_c);
    *((char*)mempcpy(new_c->channel_name, chan_name, len)) = '\0';
    current->next = new_c;
    u->channels_subd++;
  }
  return 1;
} 

int leave_channel(user * u, char * channel_name){
  channel * current = u->first_channel;
  channel * temp = NULL;
  if(!current){
    return 0;
  }
  if(strcmp(current->channel_name, channel_name) == 0){
    if(current->next){  
      u->first_channel = current->next;
    } 
    u->first_channel = current = NULL; // current = NULL; // u->first_channel = NULL;
  } else {
    while(current->next){
      if(strcmp(current->next->channel_name, channel_name) == 0){
        temp = current->next;
        current->next = current->next->next;
        free_channel(temp);
      } else{
        current = current->next;
      }
    }
  }
  return 1;
}


int kbhit(){
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

char * get_line(){
    // char * line = (char*)malloc(sizeof(char*) * SAY_MAX);
    // int c_count = 0;
    char c = getchar();
    if(c == '\n'){
      printf("\n");
      fflush(stdout);
      *c_pos = '\0';
      c_pos = line;
      return line;
    }
    if(((int)c) == 127){//backspace character
      printf("\b \b");
      fflush(stdout);
      // printf("backspace");
      fflush(stdout);
      c_pos--;
    }
    else{
      printf("%c", c);
      fflush(stdout);
      *c_pos++ = c;
    }
  return NULL;
}

void parse_input(char * input, struct addrinfo * _result, user * u, int fd_sock){
  printf(">");
  fflush(stdout);
  int num_bytes = 0;
  int len = 0;
  const char * TKN_DELIM = " \n";
  if(input[0] == '/'){
    char * command = strtok(input, TKN_DELIM);
    /* ---- EXIT ---- EXIT ---- EXIT ---- EXIT ---- EXIT ---- EXIT */
    if(strcmp(command, EXIT) == 0){
      struct request_logout req_l;
      memset(&req_l, 0, sizeof(struct request_logout));
      req_l.req_type = REQ_LOGOUT;
      if((num_bytes = sendto(fd_sock, &req_l, sizeof(struct request_logout), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
        perror("Send To Req Logout: ");
        exit(EXIT_FAILURE);
      } 
      end = true;
      // _exit(fd_sock); FIX THIS SHIT TODO
    }
    /* ---- SWITCH ---- SWITCH ---- SWITCH ---- SWITCH ---- SWITCH ---- */
    else if(strcmp(command, SWITCH) == 0){
      char * channel_name = strtok(NULL, TKN_DELIM);
      if(channel_name){
        printf("Channel Calling Switch On: \'%s\'\n", channel_name);
        if(!switch_channel(u, channel_name)){
          printf("Switch returned an error...\nAre you subscribed to the channel?\n");
        }
      }
    }
    /* ---- JOIN ----- JOIN ----- JOIN ----- JOIN ----- JOIN ----- */
    else if(strcmp(command, JOIN) == 0){
      struct request_join req_j;
      memset(&req_j, 0, sizeof(struct request_join));
      req_j.req_type = REQ_JOIN;     
      char * channel_name = strtok(NULL, TKN_DELIM);
      if(channel_name){
        len = strlen(channel_name);
        *((char*)mempcpy(req_j.req_channel, channel_name, len)) = '\0';
        if(subscribe_channel(u, channel_name)){
          if((num_bytes = sendto(fd_sock, &req_j, sizeof(struct request_join), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
            perror("Send To Req Join: ");
            exit(EXIT_FAILURE);
          } 
        } else {
          printf("Already subscribed to %s, switched to that channel\n", channel_name);
        }
        *((char*)mempcpy(u->active_channel, channel_name, len)) = '\0';
      }
    }
    /* ---- LEAVE ---- LEAVE ---- LEAVE ---- LEAVE ---- LEAVE ---- */
    else if(strcmp(command, LEAVE) == 0){
      struct request_leave req_l;
      memset(&req_l, 0, sizeof(struct request_leave));
      char * channel_name = strtok(NULL, TKN_DELIM);
      *((char *)mempcpy(req_l.req_channel, channel_name, len)) = '\0';
      if((num_bytes = sendto(fd_sock, &req_l, sizeof(struct request_leave), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
        perror("Send To Req Leave: ");
        exit(EXIT_FAILURE);
      } else {
        leave_channel(u, channel_name);
      }
    }
    /* ---- LIST ---- LIST ---- LIST ---- LIST ---- LIST ---- */
    else if(strcmp(command, LIST) == 0){
      struct request_list req_l;
      memset(&req_l, 0, sizeof(struct request_list));
      req_l.req_type = REQ_LIST;
      if((num_bytes = sendto(fd_sock, &req_l, sizeof(struct request_list), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
        perror("Send To Req lIST: ");
        exit(EXIT_FAILURE);
      }
    }
    /* ---- WHO ----- WHO ----- WHO ----- WHO ----- WHO ----- */
    else if(strcmp(command, WHO) == 0){
      struct request_who req_w;
      memset(&req_w, 0, sizeof(struct request_who));
      req_w.req_type = REQ_WHO;
      char * channel_name = strtok(NULL, TKN_DELIM);
      if(channel_name){
        len = strlen(channel_name);
        *((char*)mempcpy(req_w.req_channel, channel_name, len)) = '\0';
        if((num_bytes = sendto(fd_sock, &req_w, sizeof(struct request_who), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
          perror("sendto req who: ");
          exit(EXIT_FAILURE);
        }
      }
    }
    else 
      printf("Invalid Command: \'%s\'\n", command);
    // free(command);
  /* ---- SAY ----- SAY ----- SAY ----- SAY ----- SAY ----- */
  }else{ //say
    // print_channels(u);
    // printf("Active: %s\n", u->active_channel);
    struct request_say req_s;
    memset(&req_s, 0, sizeof(struct request_say));
    req_s.req_type = REQ_SAY;
    len = strlen(u->active_channel);
    *((char *)mempcpy(req_s.req_channel, u->active_channel, len)) = '\0';
    if( (len = strlen(input)) > SAY_MAX){
      printf("Server only accepts messages of %d length and shorter\n", SAY_MAX);
      fflush(stdout);
    } else {
      *((char *)mempcpy(req_s.req_text, input, len)) = '\0';
      if((num_bytes = sendto(fd_sock, &req_s, sizeof(struct request_say), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
        perror("Send To Req Say: ");
        exit(EXIT_FAILURE);
      }
    }
  }
}

void handle_text(struct addrinfo * _result, int fd_sock){
  // int req_type = -1;
  // char * packet = (char*)malloc(sizeof(char*) * SER_MAX);
  // memcpy(&req_type, packet, sizeof(int));

  struct text * txt = (struct text*)malloc(sizeof(struct text) + SER_MAX);
  if(recvfrom(fd_sock, txt, sizeof(*txt) + SER_MAX, 0, _result->ai_addr, &_result->ai_addrlen) == -1) {
    perror("sendto: ");
    exit(EXIT_FAILURE);
  }

  switch(txt->txt_type){
    case TXT_SAY:{
      struct text_say * txt_s = (struct text_say *)txt;
      printf("[%s][%s]: %s\n", txt_s->txt_channel, txt_s->txt_username, txt_s->txt_text);
      fflush(stdout);
      break;
    }
    case TXT_LIST:{
      int k = 0;
      struct text_list * txt_l = (struct text_list *)txt;
      printf("Existing channels:\n");
      fflush(stdout);
      while(k < txt_l->txt_nchannels){
        printf(" %s\n", txt_l->txt_channels[k++].ch_channel);
        fflush(stdout);
      }
      break;
    }
    case TXT_WHO:{
      int k = 0;
      struct text_who * txt_w = (struct text_who *)txt;
      printf("Users on channel %s:\n", txt_w->txt_channel);
      fflush(stdout);
      while(k < txt_w->txt_nusernames){
        printf(" %s\n", txt_w->txt_users[k++].us_username);
        fflush(stdout);
      }
      break;
    }
    case TXT_ERROR:{
      char * error_message = (char*)malloc(sizeof(char*) * 64);
      printf("%s\n", error_message);
      fflush(stdout);
      break;
    }
    default:{
      printf("Unknown command\n");
      fflush(stdout);
    }
  }
}

int exit_cli(int fd_sock, struct addrinfo * result){
  // free_user(u, c);
  printf("Exiting...\n");
  fflush(stdout);
  freeaddrinfo(result);
  close(fd_sock);
  cooked_mode();
  exit(EXIT_SUCCESS);
}

int main(int argc, char * argv[]){

  if(argc != 4){
    errno = EINVAL; //invalid arguements
    fprintf(stderr, "Usage: ./client server_socket server_port user\n");
    exit(EXIT_FAILURE);
  }

  fd_set readfds;
  char * server_hostname;
  user * u = NULL;
  // channel * c = NULL;
  // struct request * r = NULL;
  u = init_user(u);
  int fd_sock, info_ret;

  //signal(SIGINT, alarm_handler);
  raw_mode();

  server_hostname = argv[1];
  char * server_port = argv[2];
  strncpy(u->username, argv[3], USERNAME_MAX + 1);

  struct addrinfo hints;
  struct addrinfo * result, * _result;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; //allows ipv6 or ipv4
  hints.ai_socktype = SOCK_DGRAM;

  info_ret = getaddrinfo(server_hostname, server_port, &hints, &result);
  if( info_ret != 0 ){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(info_ret));
    exit(EXIT_FAILURE);
  }

  _result = result;
  while(_result != NULL){
    if( (fd_sock = socket(_result->ai_family, _result->ai_socktype, _result->ai_protocol)) == -1 ){
      perror("Socket Failed..\n");
      _result = _result->ai_next;
      continue;
    }
    break;
  }

  if(_result == NULL){//no bind was successful
    fprintf(stderr, "Total Socket Failure\n");
    exit(EXIT_FAILURE);
  } 

  int num_bytes = -1;

  /* Immediately Logging User In */
  // struct request_login * req_l = (struct request_login*)malloc(sizeof(struct request_login));
  struct request_login req_l;
  memset(&req_l, 0, sizeof(struct request_login));
  req_l.req_type = REQ_LOGIN;
  int len = strlen(u->username);
  *((char *)mempcpy(req_l.req_username, u->username, len)) = '\0';

  if((num_bytes = sendto(fd_sock, &req_l, sizeof(struct request_login), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
    perror("Send To Req Login: ");
    exit(EXIT_FAILURE);
  }

  /* Joining Channel Common */
  struct request_join req_j;
  memset(&req_j, 0, sizeof(struct request_join));
  req_j.req_type = REQ_JOIN;
//  char * default_channel = NULL;

 char temp[7] = "Common";
char* default_channel = temp;

  len = strlen(default_channel);
  *((char *)mempcpy(u->active_channel, default_channel, len)) = '\0';
  *((char *)mempcpy(req_j.req_channel, default_channel, len)) = '\0';
  subscribe_channel(u, default_channel);
  if((num_bytes = sendto(fd_sock, &req_j, sizeof(struct request_join), 0, _result->ai_addr, _result->ai_addrlen)) == -1) {
    perror("Send To Req Join: ");
    exit(EXIT_FAILURE);
  }

  char * line = (char*)malloc(sizeof(char*) * SAY_MAX);
  printf(">");
  fflush(stdout);
  do{
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    FD_SET(fd_sock, &readfds);
    select(fd_sock+1, &readfds, NULL, NULL, NULL);
    if (FD_ISSET(0, &readfds)) {
      line = get_line();
      if(line != NULL){
        parse_input(line, _result, u, fd_sock);
      }
    } else if (FD_ISSET(fd_sock, &readfds)) {
      handle_text(_result, fd_sock);
    }
  }while(!end);

  exit_cli(fd_sock, result);
}
