#ifndef SERVER_H
#define SERVER_H 

#include "duckchat.h"

#define UCOUNT_MAX 32
#define CCOUNT_MAX 32

void *mempcpy(void *dest, const void *src, size_t n);

typedef struct channel{
  char channel_name[CHANNEL_MAX];
  struct user * first_u;
  struct channel * next_c;
  int user_count;
  int serv_subbed;
  int stale_channel;//if this reaches 2, channel is stale & leave flooded
}channel;

typedef struct user{
  char username[USERNAME_MAX];
  struct sockaddr_in sock;
  unsigned short port;
  char ip[32];
  struct user * next_u;
}user;

typedef struct super_user{
  char username[USERNAME_MAX];
  struct sockaddr_in sock;
  unsigned short port;
  char ip[32];
  struct super_user * next_su;
}super_user;

typedef struct server{
  char hostname[INET_ADDRSTRLEN];
  int uids[32];//Apparently there are issues with creating a 
  int uid_idx;
  // char port[16];
  unsigned short port;
  int channel_count;
  char ** channels;
  struct server * next_s;
}server;

#endif