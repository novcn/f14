#ifndef CLIENT_H
#define CLIENT_H 

#include "duckchat.h"

void *mempcpy(void *dest, const void *src, size_t n);

typedef struct channel{
	char channel_name[CHANNEL_MAX];
	struct channel * next;
}channel;

typedef struct user{
	char username[USERNAME_MAX];
	char active_channel[CHANNEL_MAX];
	channel * first_channel;
  int channels_subd;
}user;


user * init_user(user * u);

channel * init_channel(channel * c);

/**
 *Initializes a channel and returns it
 *
 *@param c, the channel to be initialized
 *@return c, the initialized channel
 */
channel * init_channel(channel * c);

/**
 *
 *
 *
 */
int subscribe_channel(user * u, char * channel_name);

/**
 *Frees each channel in subscribed list and the frees the user
 *
 *@param u, the user to be freed 
 *@param c, the channel to be freed
 */
void _free(user * u, channel * c);


/**
 *Switches a users active channel by iterating through the subscribed channel list
 *
 *@param channel_name, the name of a channel received after the /switch command
 *@return, returns 0 if the channel was not in the list of subscribed channels
 */
int switch_channel(user * u, char * channel_name);

#endif
