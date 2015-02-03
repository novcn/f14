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
#include "list.h"

#define CLI_MAX 128
#include <signal.h>
#include "duckchat.h"
int end = false;


void alarm_handler(){
  end = true;
}


int count_users(channel * ch, char * channel_name){
  channel * c = ch; //first
  //int u_count = 0;
  if(c == NULL){
    printf("Null channel \n");
    return -1;
  }
  while(c != NULL){
    if(strcmp(ch->channel_name, channel_name) == 0){
      return c->user_count;
    }
    c = c->next_channel;
  }
  return -1;
}

int main(int argc, char * argv[]){

struct channel* common = NULL; // Always have common on startup
  
  user* first_user = NULL;  
  int fd_sock;
  struct addrinfo hints, *result, *_result;
	int rv;
  int numbytes;
  struct sockaddr_in client_addr;
  socklen_t addr_len;
  //char s[INET6_ADDRSTRLEN];
  //signal(SIGINT, alarm_handler);

  int numberofclients =0;


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

  while(!end){

    // char * packet = (char*)malloc(sizeof(char*) * 128);
    struct request * req = (struct request*)malloc(sizeof(struct request) + CLI_MAX);

    if ((numbytes = recvfrom(fd_sock, req, CLI_MAX , 0, (struct sockaddr *) &client_addr, &addr_len)) == -1) {
      perror("error recvfrom:");
      exit(EXIT_FAILURE);
    }

    unsigned short from_port = client_addr.sin_port; // Create a port number variable

    //int req_type;
//	req_type = -1;
    // struct request * req = (struct request*)malloc(sizeof(struct request)+ CLI_MAX);
    // memcpy(&req_type, packet, sizeof(int));
    // mempcpy(req, packet, sizeof(request_t));
  //  printf("req: %d\n", req->req_type);

    switch(req->req_type){
      case REQ_LOGIN:{////////// LOGIN ////////// LOGIN ////////// LOGIN /////////// 
        printf("REQ_LOGIN\n");
        struct request_login * req_l = (struct request_login *)req;
        // char * username = (char*)malloc(sizeof(char*) * USERNAME_MAX);
        // *((char*)mempcpy(username, (&packet[sizeof(int)]), USERNAME_MAX)) = '\0';
        printf("uname: %s\n", req_l->req_username);

        if(numberofclients == 0 ){   // First user to join?
          first_user = init_user(first_user, req_l->req_username,from_port);
          numberofclients++; 
        }                                                      // Not first user
        else if(numberofclients >0){
          add_new_user(first_user, req_l->req_username, from_port); //Init new_client; change user_name
          numberofclients++;
        }
        printf("%s just requested login.\n", req_l->req_username);
        break;
      }
      case REQ_LOGOUT:{///////// LOGOUT ///////// LOGOUT ///////// LOGOUT //////////
        
        user* u = get_user(first_user, from_port);
        //struct request_logout * req_l = (struct request_logout*)req;
        if(u == first_user && u->next_user == NULL){    // head of the list being removed
          common = free_all_channels(common);
          u = free_all_users(u);
          common = NULL;
          u = NULL;
          numberofclients--;
          break;
        }
        else if(u == first_user && u->next_user !=NULL){
          first_user = u->next_user;
          logout(u, common);
          numberofclients--;
        }
        else if(u != first_user){
          //first_user = u->
          logout(u, common);
          numberofclients--;
          }
          printf("Printing common after logout\n");
          printf("REQ_LOGOUT\n");
          break;
      }
      case REQ_JOIN:{///////// JOIN //////////// JOIN //////////// JOIN //////////// 
        printf("REQ_JOIN\n");
        struct request_join * req_j = (struct request_join*)req;

        // char * channel_name = (char*)malloc(sizeof(char*) * CHANNEL_MAX);
        // *((char*)mempcpy(channel_name, (&packet[sizeof(int)]), CHANNEL_MAX)) = '\0';
        
        if(numberofclients == 1){
	char temp[7] = "Common";
          common = init_channel(common, temp);  // Always have common on startup
          }
        user* u = get_user(first_user, from_port);
        u = join_channel(u, common, req_j->req_channel);
        list_all_channels(common);
        // free(channel_name);
        break;
      }
      case REQ_LEAVE:{///////// LEAVE ///////// LEAVE ///////// LEAVE /////////////
        printf("REQ_LEAVE\n");
        struct request_leave * req_l = (struct request_leave*)req;
        // char * channel_name = (char*)malloc(sizeof(char*) * CHANNEL_MAX);
        // *((char*)mempcpy(channel_name, (&packet[sizeof(int)]), CHANNEL_MAX)) = '\0';
        user* u = get_user(first_user, from_port);
        if(strcmp(req_l->req_channel, (char*)common->channel_name )==0 ){ // User is attempting to leave the first channel

          if(common->user_count == 1 && common->head->port == u->port){ // Last channel remaining
            free(common->head->user_name);
            free(common->head);
            free(common->channel_name);
            free(common);
            common = NULL;
            break;
          }
          else if (common->user_count >1){
            leave_channel(u, common, req_l->req_channel);
          }
        }
        else{
          leave_channel(u, common, req_l->req_channel);
          printf("cname: %s\n", req_l->req_channel);
          break;
        }
      }
      case REQ_SAY:{///////// SAY //////////// SAY //////////// SAY ///////////////
        printf("REQ_SAY\n");
        struct request_say * req_s = (struct request_say*)req;
        int len = strlen(req_s->req_channel);
        struct text_say txt_s;
        memset(&txt_s, 0, sizeof(struct text_say));
        txt_s.txt_type = TXT_SAY;
        *((char*)mempcpy(txt_s.txt_channel, req_s->req_channel, len)) = '\0';
        len = strlen(req_s->req_text);
        *((char*)mempcpy(txt_s.txt_text, req_s->req_text, len)) = '\0';
        
        channel * c = common;
        user * u;
        int k = 0; 
        while(c != NULL){
          if(strcmp(c->channel_name, req_s->req_channel) == 0){
            u = c->head;
            while(u != NULL){
              len = strlen(u->user_name);
              *((char*)mempcpy(txt_s.txt_username, u->user_name, USERNAME_MAX)) = '\0';
              if((numbytes = sendto(fd_sock, &txt_s, sizeof(struct text_say), 0, (struct sockaddr *) &client_addr, addr_len)) == -1) {
                perror("Send To Req Logout: ");
                exit(EXIT_FAILURE);
              } 
              u = u->next_user;
              k++;
            }
          }
          c = c->next_channel;
        }

        break;
      }
      case REQ_LIST:{///////// LIST /////////// LIST /////////// LIST ////////////
        printf("REQ_LIST\n");
        struct text_list txt_l;
        memset(&txt_l, 0, sizeof(struct text_list));
        txt_l.txt_type = TXT_LIST;
        //struct user_info uinfo;
        channel * c = common;
        //user * u;
        int k = 0; 
        while(c != NULL){
          *((char*)mempcpy(txt_l.txt_channels[k].ch_channel, c->channel_name, CHANNEL_MAX)) = '\0';
          k++;
          c = c->next_channel;
        }
        txt_l.txt_nchannels = k;

        if((numbytes = sendto(fd_sock, &txt_l, sizeof(struct text_list) + (k * sizeof(struct channel_info)), 0, (struct sockaddr *) &client_addr, addr_len)) == -1) {
          perror("Send To Req Logout: ");
          exit(EXIT_FAILURE);
        } 
        break;
      }
      case REQ_WHO:{///////// WHO ///////////// WHO ///////////// WHO //////////////

        //char channel_name[CHANNEL_MAX];
        // *((char*)mempcpy(channel_name, (&packet[sizeof(char*)]), CHANNEL_MAX)) = '\0';
        struct request_who * req_w = (struct request_who*)req;
        printf("REQ_WHO\n");
        //int len =0;
	//len =  sizeof(req_w->req_channel);
        struct text_who txt_w;
        memset(&txt_w, 0, sizeof(struct text_who));
        txt_w.txt_type = TXT_WHO;
        int u_count = count_users(common, req_w->req_channel);
        txt_w.txt_nusernames = u_count;
        // who_users(common, channel_name, u_count, txt_users);

        //struct user_info uinfo;
        // struct user_info txt_users[u_count];
        channel * c = common;
        user * u;
        int k = 0; 
        while(c != NULL){
          if(strcmp(c->channel_name, req_w->req_channel) == 0){
            u = c->head;
            while(u != NULL){
              //len = strlen(u->user_name);
              *((char*)mempcpy(txt_w.txt_users[k].us_username, u->user_name, USERNAME_MAX)) = '\0';
              u = u->next_user;
              k++;
            }
          }
          c = c->next_channel;
        }

        if((numbytes = sendto(fd_sock, &txt_w, sizeof(struct text_who) + (u_count * sizeof(struct user_info)), 0, (struct sockaddr *) &client_addr, addr_len)) == -1) {
          perror("Send To Req Logout: ");
          exit(EXIT_FAILURE);
        } 
        break;
      }
      default:{
        printf("Default");
      }
    }

    free(req);
  }

  close(fd_sock);
  return 0;
}
