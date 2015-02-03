#ifndef __LIST_H__
#define __LIST_H__
#include "list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#define REQ_LOGIN 0
#define REQ_LOGOUT 1
#define REQ_JOIN 2
#define REQ_LEAVE 3
#define REQ_SAY 4
#define REQ_LIST 5
#define REQ_WHO 6

/* Define codes for text types.  These are the messages sent to the client. */
#define TXT_SAY 0
#define TXT_LIST 1
#define TXT_WHO 2
#define TXT_ERROR 3


//=================================================================================
typedef struct user{
	char* user_name;
	struct user* next_user;		//user structure
	struct user* prev_user;
	struct channel * head;
	struct channel * tail;
	int channel_count;
	int port;
	int IP;
}user;

typedef struct channel{
	char * channel_name;
	struct channel * next_channel;	//channel structure
	struct channel * prev_channel;
	struct user *head;
	struct user *tail;
	unsigned int user_count;
}channel;

user * init_user(user * u, char* name, int port);
channel * init_channel(channel* c, char* chanName);
channel* new_channel(channel * channel_list, char* name);
user* add_new_user(user* user_list, char* new_username, int portnumb);
channel* obliterate_channel(channel* channelname);
user* obliterate_user(user* username);
channel* free_all_channels(channel* channel_list);
user* free_all_users(user* user_list);
int find_user(channel* search_chan,char* username );
void list_all_channels(channel* head);
void list_users(channel* channel_list);
channel* add_user_to_channel(channel* ch, char* newusername);
user* add_channel_to_user(user* u, char* newchannelname);
user* join_channel(user* client,channel* channel_list, char* name);
user* logout(user* u, channel* channel_list);
void leave_channel(user * u, channel* channel_list, char * channelname);
user* get_user(user* user_list, int port);

#endif

