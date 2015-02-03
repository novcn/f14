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

//#include "duckchat.h"

/* This is the definition of the data structure used by the server and
 * each individual client to keep record of the current channels.
 * Channels contain a list of users currently subscribed to the channel. */


//=================================================================================
user * init_user(user * u, char* name, int port){
	u = (user*)malloc(sizeof(struct user));
	u-> user_name =(char*) malloc(sizeof(name)+2); // See above
	strcpy(u->user_name, name);
	u->next_user = NULL;
	u->prev_user = NULL;
	u->head = NULL;
	u->tail = NULL;
	u->channel_count = 0;
	u->port = port;
	u->IP = 0;
	return u;
}
channel * init_channel(channel* c, char* chanName){
	c = (channel*) malloc(sizeof(struct channel));
	c->channel_name =(char*)malloc(sizeof(chanName)+2);
	strcpy(c->channel_name,chanName);
	c-> next_channel = NULL;
	c-> prev_channel = NULL;	// Creates a structure in memory.
	c->user_count = 0;		// All variables and pointers init zero
	c->head = NULL;
	c->tail = NULL;
	return c;
}
//=================================================================================
/* Creates a new channel, initializes memory and
 * appends it to the channel list
 */
channel* new_channel(channel * channel_list, char* name){

	if(channel_list == NULL || name ==NULL){
		perror("new_channel: bad pointer");  // Error checking
		exit(1);
	}

	channel * c = channel_list;
	while(c->next_channel != NULL) // Move to the tail
		c = c->next_channel;
	channel * new_chan = NULL;
	new_chan = init_channel(new_chan, name);// inits pointer
	c-> next_channel = new_chan;	// Sets the new channel at the tail
	new_chan -> prev_channel = c;
	return new_chan;
}
//=================================================================================
user* add_new_user(user* user_list, char* new_username, int portnumb){

	if(user_list == NULL || portnumb <0){

		user* new_user = NULL;
		new_user = init_user(new_user, new_username, portnumb); // 
		return new_user;
	}
	else{
		user* new_user = NULL;
		new_user =  init_user(new_user, new_username, portnumb);

		while(user_list->next_user != NULL){
			user_list = user_list->next_user;
		}
		user_list->next_user = new_user;
		new_user->prev_user = user_list;
		return new_user;
	}
	return NULL;
}
//=================================================================================i
/* Prints a list of all the channel names to console */
channel* obliterate_channel(channel* channelname){
	channel* c = channelname;	// Pointers
	if(channelname == NULL)
	{
		perror("oblit. chann: bad param");
		exit(1);
	}
	if(c->user_count == 0){ // Channel has no users
		if(c->next_channel != NULL && c->prev_channel != NULL){ // All channels removed, fix broken chain of users
			// Middle of the chain
			c->prev_channel->next_channel = c->next_channel;			// Middle of the channel chain
			c->next_channel->prev_channel = c->prev_channel;
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}
		else if(c->prev_channel == NULL && c->next_channel != NULL){ // Userlist head node being removed of list >1

			free(c->channel_name);
			c = c->next_channel;
			free(c->prev_channel);
			c->prev_channel = NULL;
			printf("Oblit chan: removed channellist head\n");
		}
		else if (c->next_channel == NULL && c->prev_channel == NULL){// Final node being removed
			printf("Deleting final channel\n");
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}

		else if(c->prev_channel != NULL && c->next_channel == NULL){ // Tail user of list >1
			c->prev_channel->next_channel = NULL;
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}
	}
	else if(c->user_count ==1){ // Only one user exists on this channel

		free(c->head->user_name);	// Free username, user struct
		free(c->head);
		if(c->next_channel != NULL && c->prev_channel != NULL){ // All channels removed, fix broken chain of users
			// Middle of the chain
			c->prev_channel->next_channel = c->next_channel;			// Middle of the channel chain
			c->next_channel->prev_channel = c->prev_channel;
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}
		else if(c->prev_channel == NULL && c->next_channel != NULL){ // Userlist head node being removed of list >1

			free(c->channel_name);
			c = c->next_channel;
			free(c->prev_channel);
			c->prev_channel = NULL;
			printf("Oblit chan: removed channellist head\n");
		}
		else if (c->next_channel == NULL && c->prev_channel == NULL){// Final node being removed
			printf("Deleting final channel\n");
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}

		else if(c->prev_channel != NULL && c->next_channel == NULL){ // Tail user of list >1
			c->prev_channel->next_channel = NULL;
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}
	}
	else if(c->user_count >1){		// Channel has more than one user?

		user* uP = c->tail->prev_user; // Pointing to the first user of the channel's user list
		int i =0, bound = c->user_count	;
		while(i<bound){
			free(c->tail->user_name);	// Yes: free users
			free(c->tail);
			c->tail = uP;
			c->user_count--;
			if(uP->prev_user != NULL){
				uP = uP->prev_user;
				i++;
			}
			else{
				assert(c->head == c->tail);
				assert(c->user_count == 1);
				free(c->head->user_name);
				free(c->head);
				break;
			}
		}
		if(c->next_channel != NULL && c->prev_channel != NULL){ // All channels removed, fix broken chain of users
			// Middle of the chain
			c->prev_channel->next_channel = c->next_channel;			// Middle of the channel chain
			c->next_channel->prev_channel = c->prev_channel;
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}
		else if(c->prev_channel == NULL && c->next_channel != NULL){ // Userlist head node being removed of list >1

			free(c->channel_name);
			c = c->next_channel;
			free(c->prev_channel);
			c->prev_channel = NULL;
			printf("Oblit chan: removed channellist head\n");
		}
		else if (c->next_channel == NULL && c->prev_channel == NULL){// Final node being removed
			printf("Deleting final channel\n");
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}

		else if(c->prev_channel != NULL && c->next_channel == NULL){ // Tail user of list >1
			c->prev_channel->next_channel = NULL;
			free(c->channel_name);
			free(c);
			c = NULL;
			channelname = NULL;
			return NULL;
		}
	}
	return NULL;
}
//=================================================================================
/* This is to be used before shutting down the client or server.
 * Completely free all allocated memory, including "Common".
 */
user* obliterate_user(user* username){
	user* u = username;	// Pointers
	if(username == NULL){
		perror("oblit. user: bad param\n");
		exit(EXIT_FAILURE);
	}

	if(u->channel_count == 0){ // user has no channels
		if(u->next_user != NULL && u->prev_user != NULL){ // All channels removed, fix broken chain of users
			// Middle of the chain
			u->prev_user->next_user = u->next_user;			// Middle of the user chain
			u->next_user->prev_user = u->prev_user;
			free(u->user_name);
			free(u);
			u = NULL;
			username = NULL;
			return NULL;
		}
		else if(u->prev_user == NULL && u->next_user != NULL){ // Userlist head node being removed of list >1

			free(u->user_name);
			u = u->next_user;
			free(u->prev_user);
			u->prev_user = NULL;
			printf("Oblit user: removed userlist head\n");
		}
		else if (u->next_user == NULL && u->prev_user == NULL){// Final node being removed
			printf("Deleting final user\n");
			free(u->user_name);
			free(u);
			username = NULL;
			return NULL;
		}

		else if(u->prev_user != NULL && u->next_user == NULL){ // Tail user of list >1
			u->prev_user->next_user = NULL;
			free(u->user_name);
			free(u);
			username = NULL;
			return NULL;
		}
	}
	else if(u->channel_count ==1){ // Only one user exists on this user

		free(u->head->channel_name);	// Free username, user struct
		free(u->head);
		if(u->next_user != NULL && u->prev_user != NULL){ // All channels removed, fix broken chain of users
			// Middle of the chain
			u->prev_user->next_user = u->next_user;			// Middle of the user chain
			u->next_user->prev_user = u->prev_user;
			free(u->user_name);
			free(u);
			u = NULL;
			username = NULL;
			return NULL;
		}
		else if(u->prev_user == NULL && u->next_user != NULL){ // Userlist head node being removed of list >1

			free(u->user_name);
			u = u->next_user;
			free(u->prev_user);
			u->prev_user = NULL;
			printf("Oblit user: removed userlist head\n");
		}
		else if (u->next_user == NULL && u->prev_user == NULL){// Final node being removed
			printf("Deleting final user\n");
			free(u->user_name);
			free(u);
			username = NULL;
			return NULL;
		}

		else if(u->prev_user != NULL && u->next_user == NULL){ // Tail user of list >1
			u->prev_user->next_user = NULL;
			free(u->user_name);
			free(u);
			username = NULL;
			return NULL;
		}
	}
	else if(u->channel_count >1){		// user has more than one channel?

		channel* cP = u->tail->prev_channel;
		int i =0, bound = u->channel_count;
		while(i<bound){
			free(u->tail->channel_name);	// Yes: free channels
			free(u->tail);
			u->tail = cP;
			u->channel_count--;
			if(cP->prev_channel != NULL){
				cP = cP->prev_channel;
				i++;
			}
			else{
				assert(u->head == u->tail);
				assert(u->channel_count == 1);
				free(u->head->channel_name);
				free(u->head);
				break;
			}
		}
		if(u->next_user != NULL && u->prev_user != NULL){ // All channels removed, fix broken chain of users
			// Middle of the chain
			u->prev_user->next_user = u->next_user;			// Middle of the user chain
			u->next_user->prev_user = u->prev_user;
			free(u->user_name);
			free(u);
			u = NULL;
			username = NULL;
			return NULL;
		}
		else if(u->prev_user == NULL && u->next_user != NULL){ // Userlist head node being removed of list >1

			free(u->user_name);
			u = u->next_user;
			free(u->prev_user);
			u->prev_user = NULL;
			printf("Oblit user: removed userlist head\n");
		}
		else if (u->next_user == NULL && u->prev_user == NULL){// Final node being removed
			printf("Deleting final user\n");
			free(u->user_name);
			free(u);
			username = NULL;
			return NULL;
		}

		else if(u->prev_user != NULL && u->next_user == NULL){ // Tail user of list >1
			u->prev_user->next_user = NULL;
			free(u->user_name);
			free(u);
			username = NULL;
			return NULL;
		}
	}
return NULL;
}

channel* free_all_channels(channel* channel_list){

	if(channel_list == NULL){
		printf("Error free all: bad arg"); // Error checking
		exit(EXIT_FAILURE);
	}
	channel* leader = channel_list;
	channel* o = channel_list;
	channel* fencepost = channel_list;
	user *uP = NULL;
	printf("Freeing all channels!!!\n");

	if(channel_list != NULL&& channel_list->next_channel!=NULL){	// Case: More than one channel exists

		while(o->next_channel != NULL)
			o = o->next_channel;
		leader = o->prev_channel;	// Sets o to final channel and leader to second-to-last

		while(o != fencepost){		// Moving from tail to head, removing channels
			o = obliterate_channel(o);
			//	list_channels(channel_list);
			o = leader;
			if(leader != fencepost)	// Keep going until 1 channel remains
				leader = leader -> prev_channel;
			else {
				if(leader->user_count ==0){
					free(leader->channel_name);
					free(leader);
					break;
				}
				else if(leader->user_count ==1){
					free(leader->head->user_name);
					free(leader->head);
					free(leader->channel_name);
					free(leader);
					break;
				}
				else if(leader->user_count > 1){
					unsigned int i=0;
					uP = leader->tail;
					for(i=0;i<leader->user_count;i++){	// Delete users on this channel

						if(uP == leader-> head){
							free(uP->user_name);
							free(uP);
							free(leader->channel_name);
							free(leader);
							break;
						}
						else{
							uP = leader -> tail -> prev_user;
							free(leader -> tail->user_name);
							free(leader -> tail);
							leader->tail = uP;
						}

					}
				}
			}
		}
	}
	else if(channel_list != NULL ){ 	// Case: one channel exists
		unsigned int i = 0;

		if(channel_list->user_count == 0){
			free(channel_list->channel_name);
			free(channel_list);
			return NULL;
		}
		else if ( channel_list-> user_count ==1){
			free(channel_list->tail->user_name);
			free(channel_list->tail);
			free(channel_list->channel_name);
			free(channel_list);
			return NULL;
		}
		else if (channel_list->user_count >1){
			uP =channel_list->tail;
			
			for(i=0;i< channel_list->user_count;i++){
				if(channel_list->tail->prev_user == NULL){
					free(channel_list->tail->user_name);
					free(channel_list->tail);
					free(channel_list->channel_name);
					free(channel_list);
					return NULL;
				}
				else{
					free(channel_list->tail->user_name);
					channel_list->tail = channel_list->tail->prev_user;
					free(channel_list->tail->next_user);
				}
			}
		}
	}
	else{
		perror("Lincoln's function.\n");
		exit(EXIT_FAILURE);
	}
	return channel_list;
}
//=================================================================================

user* free_all_users(user* user_list){

	if(user_list == NULL){
		printf("Error free all: bad arg"); // Error checking
		exit(EXIT_FAILURE);
	}
	user* leader = user_list;
	user* o = user_list;
	user* fencepost = user_list;
	channel *cP = NULL;
	printf("Freeing all users!!!\n");

	
	if(user_list != NULL && user_list ->next_user == NULL ){ 	// Case: one user exists
		int i = 0;

		if(user_list->channel_count == 0){
			free(user_list->user_name);
			free(user_list);
			return NULL;
		}
		else if ( user_list-> channel_count ==1){
			free(user_list->tail->channel_name);
			free(user_list->tail);
			free(user_list->user_name);
			free(user_list);
			return NULL;
		}
		else if (user_list->channel_count >1){
			cP =user_list->tail;
			for(i=0;i< user_list->channel_count;i++){
				if(user_list->tail->prev_channel == NULL){
					free(user_list->tail->channel_name);
					free(user_list->tail);
					free(user_list->user_name);
					free(user_list);
					return NULL;
				}
				else{
					free(user_list->tail->channel_name);
					user_list->tail = user_list->tail->prev_channel;
					free(user_list->tail->next_channel);
				}
			}
		}
	}
	else if(user_list != NULL&& user_list->next_user!=NULL){	// Case: More than one user exists

		while(o->next_user != NULL)
			o = o->next_user;
		leader = o->prev_user;	// Sets o to final user and leader to second-to-last

		while(o != fencepost){		// Moving from tail to head, removing users
			o = obliterate_user(o);
			//	list_channels(user_list);
			o = leader;
			if(leader != fencepost)	// Keep going until 1 channel remains
				leader = leader -> prev_user;
			else {
				if(leader->channel_count ==0){
					free(leader->user_name);
					free(leader);
					break;
				}
				else if(leader->channel_count ==1){
					free(leader->head->channel_name);
					free(leader->head);
					free(leader->user_name);
					free(leader);
					return NULL;
				}
				else if(leader->channel_count > 1){
					int i=0;
					cP = leader->tail;
					for(i=0;i<leader->channel_count;i++){	// Delete users on this channel

						if(cP == leader-> head){
							free(cP->channel_name);
							free(cP);
							free(leader->user_name);
							free(leader);
							break;
							return NULL;
						}
						else{
							cP = leader -> tail -> prev_channel;
							free(leader -> tail->channel_name);
							free(leader -> tail);
							leader->tail = cP;
						}

					}
				}
			}
		}
	}
	else{
		perror("Lincoln's function.\n");
		exit(EXIT_FAILURE);
	}
	return user_list;
}

int find_user(channel* search_chan,char* username ){

	if(search_chan == NULL || username == NULL){
		printf("find_user:bad param");
		return 0;
	}
	channel* cP = search_chan;
	user* uP = search_chan->head;
	if(cP->user_count == 0)
		return 0;

	else if(cP->user_count ==1)
		if(strcmp(uP->user_name, username) == 0)
			return 1;
		else return 0;

	else if(cP->user_count >1){
		unsigned int i = 0;
		for(i=0; i< cP->user_count; i++){

			if(strcmp(uP->user_name,username)==0)
				return 1;
			else
				uP = uP->next_user;
		}
		return 0;
	}
	return 0;
}
//=================================================================================
/* Prints a list of all the channel names to console */

void list_all_channels(channel* head){
	if (head == NULL){
		printf("List channels: Head nonexistent.\n");
		//exit(EXIT_FAILURE);
	}
	else{
		unsigned short n=1;
		printf("Channel list:\n");
		do{
			printf("%d.%s\n",n++,head->channel_name );
			head = head-> next_channel;
			//sleep(1);
		}while(head != NULL);
		n = 1;
	}
}
//=================================================================================i
void list_users(channel* channel_list){
	//assert(channel_list->user_count >0);
	//assert(channel_list->head != NULL);
	if(channel_list == NULL)
		printf("Cannot print users, channel does not exist\n");
	if(channel_list->user_count ==0 || channel_list->head == NULL)
		printf("[%s] has no current users.\n", channel_list->channel_name);
	else{
		user* x = channel_list->head;
		unsigned short n=1;
		printf("Active users on [%s]:\n",channel_list->channel_name );

		do{
			printf("%d.%s\n",n++,x->user_name );
			x = x-> next_user;
		}while(x != NULL);
		n = 1;
	}
}
//=================================================================================
channel* add_user_to_channel(channel* ch, char* newusername){

	if(ch == NULL || ch->channel_name == NULL)
		printf("Add_user_to_channel: bad params\n");

	if(ch == NULL || newusername  == NULL){
		perror("add_user_to_channel: bad param");
		exit(EXIT_FAILURE);
	}
	if(ch->user_count ==0){
		user* newuser = NULL;
		newuser = init_user(newuser, newusername, 0);
		ch->head = newuser;
		ch->tail = newuser;
		ch->user_count++;
		return ch;
	}
	else if(ch->user_count>0){

		user* newuser = NULL;
		newuser = init_user(newuser, newusername, 0);
		ch->tail->next_user = newuser;
		newuser->prev_user = ch->tail;
		ch->tail = newuser;
		ch->user_count++;
		return ch;
	}
	return NULL;
}
//=================================================================================
user* add_channel_to_user(user* u, char* newchannelname){

	char* newname = newchannelname;

	if(u == NULL || newchannelname == NULL){
		perror("a_c_t_u: bad param");
		exit(-1);
	}	
	if(u->head == NULL&& u->channel_count ==0){				// First channel	
		channel * nc = NULL;
		nc = init_channel(nc, newname);
		u->head = nc;
		u->tail = nc;
		u->channel_count++;
		return u;
	}
	else if(u->head != NULL && u->channel_count >0){
		channel * nc = NULL;		
		nc = init_channel(nc, newname);
		u->tail->next_channel = nc;
		nc->prev_channel = u->tail;
		u->tail = nc;
		u->channel_count++;
		return u;
	}
	return NULL;
}
//=================================================================================//
/* Subscribe to the named channel; create if it does not exist */
user* join_channel(user* client,channel* channel_list, char* name){
	if (client== NULL || channel_list ==NULL ){
		perror("join channel: bad param");
		exit(EXIT_FAILURE);
	}
	if(client->channel_count ==0){		// Client currently unsubscribed
		channel* c = channel_list;		// Find out whether or not the channel exists
		while(c != NULL){		// Check each channel name
			if(strcmp (name, c->channel_name) ==0){	// Case: We have a match!

				add_user_to_channel(c, client->user_name);	// Channel exists, add user to channel
				add_channel_to_user(client, c->channel_name); // vice versa
				return client;                    				// Subscribe to named channelname
			}
			else
			c = c->next_channel;
		}
		//did not find the channel, creating and subscribing to new one.
		channel* newchan = new_channel(channel_list, name);
		add_user_to_channel(newchan, client->user_name);	// usage: channel*, char * name
		add_channel_to_user(client, newchan->channel_name);
		return client;
	}
	else if(client->channel_count == 1){ // Case user has one channel sub'd: probably common
		if(strcmp(client->head->channel_name,name) ==0){ // Single user is a match!

			printf("%s is already subscribed to [%s]", client->user_name, name);
			return client;
		}
		else {
			channel* c = channel_list;		// Find out whether or not the channel exists
			
			while(c != NULL){		// Check each channel name
				if(strcmp (name, c->channel_name) ==0){	// Case: We have a match!
					
					add_user_to_channel(c, client->user_name);	// Channel exists, add user to channel
					add_channel_to_user(client, c->channel_name); // vice versa
					return client;                    				// Subscribe to named channelname

				}
				else
				c = c->next_channel;
			}
			channel* newchan = new_channel(channel_list, name);
			add_user_to_channel(newchan, client->user_name);	// usage: channel*, char * name
			add_channel_to_user(client, newchan->channel_name);
			return client;
		}
	}
	else { // User has more than one channel to look through

		channel* c = client->head;
		while(c!= NULL){
			if(strcmp(c->channel_name,name) ==0){// we have a match
				printf("JOIN: %s is already subscribed, make [%s] the active channel.", client->user_name, name); // look thru all client's channels
				return client;								
			}
			else
				c = c->next_channel;
		}
		c = channel_list;		// Find out whether or not the channel exists
		while(c != NULL){		// Check each channel name
			if(strcmp (name, c->channel_name) ==0){	// Case: We have a match!
				
				add_user_to_channel(c, client->user_name);	// Channel exists, add user to channel
				add_channel_to_user(client, c->channel_name); // vice versa
				return client;                    				// Subscribe to named channelname
			}
			else
			c = c->next_channel;
		}
		channel* newchan = new_channel(channel_list, name);
		add_user_to_channel(newchan, client->user_name);	// usage: channel*, char * name
		add_channel_to_user(client, newchan->channel_name);
		return client;
	}
}
//==========================================================================================================
user* logout(user* u, channel* channel_list){//, channel* first_channel){

	if(u == NULL|| channel_list == NULL){
		perror("Logout error");
		exit(EXIT_FAILURE);
	}
	char* username = u -> user_name;


								// Check the channel list first
	channel* cp = channel_list;
	user* up = cp->head;
	while(cp!= NULL){ 			// iterating through channels

		// Iterating through channel list
			up = cp->head; // head is a user
			
			while(up!= NULL){ // iterate through those users, checking names

				if(strcmp(username, up->user_name)==0){// Found a match
					
					if(up == cp->head){ 				    // Match is the head user of the channel
						
						free(up->user_name);	
						if(cp->user_count >1){				// ... case: more than one user exists on channel
							up = up->next_user; 
							free(up->prev_user);
							up->prev_user = NULL;
							cp->head = up;
							cp->user_count--;
								if(cp->user_count ==0){
									channel_list = cp->next_channel;
									obliterate_channel(cp);
								}
						}
						else if(cp ->user_count ==1){	// ... case: head is the only user on the channel
							free(up);
							cp->head = NULL;
							cp->tail = NULL;
							cp->user_count --;
								if(cp->user_count ==0){
									channel_list = cp->next_channel;
									obliterate_channel(cp);
								}
						}
					}
					else if (up == cp->tail){ 			// Match is the tail user of the channel

						free(up->user_name);
						up = up->prev_user;
						free(up->next_user);
						up->next_user = NULL;
						cp->tail = up;
						cp->user_count--;
						if (up == cp->head)
							assert(cp->user_count ==1);
							if(cp->user_count ==0)
								obliterate_channel(cp);
					}

					else if( up != cp->head &&  up != cp->tail ){	// Match is the middle of the channel

						assert(cp->user_count >=3);
						free(up->user_name);
						up->prev_user->next_user = up->next_user;
						up->next_user->prev_user = up->prev_user;
						free(up);
						up = NULL;
						cp->user_count--;
					}
				}
				else up = up->next_user;	 // No match, move to next user
			}		
		cp = cp->next_channel;
		}

	
	// The channel list is now free of the user entry
	// Now manipulating the user list

	// LOGOUT FROM USER LIST

	obliterate_user(u);	

	return NULL;
}

void leave_channel(user * u, channel* channel_list, char * channelname){

	//remove channel from user
	if(u== NULL || u->channel_count == 0||channel_list == NULL){
		printf("leave_channel: bad param");
		exit(-1);
	}
	int flag = 0;
	/*Modifying user list first */
	if(u->channel_count == 1){  // user has a single channel, check if we need to leave it

		if(strcmp (channelname, u->head->channel_name) == 0){
			free(u->head->channel_name);
			free(u->head);
			u->head = u->tail= NULL;
		}
		else printf("Leave channel: %s is not a member of [%s]\n", u->user_name, channelname);
	}
	else if(u->channel_count >1){
		channel* cp = u->head;
		while(cp!= NULL){
			if(strcmp (channelname, cp->channel_name) == 0){ // found a match

				if(cp->prev_channel != NULL && cp->next_channel == NULL){ // tail end
					free(cp->channel_name);
					cp = cp->prev_channel;
					free(cp->next_channel);
					cp->next_channel = NULL;
					u->tail = cp;
					u->channel_count --;
					cp = NULL;
					flag =1;
				}
				else if(cp->prev_channel == NULL && cp->next_channel != NULL){  // head end
					free(cp->channel_name);
					cp = cp->next_channel;
					free(cp->prev_channel);
					cp->prev_channel = NULL;
					u -> head = cp;
					u->channel_count--;
					cp = NULL;
					flag =1;
				}
				else if ( cp->prev_channel != NULL && cp->next_channel != NULL){ // middle of the list
					free(cp->channel_name);
					cp->prev_channel->next_channel = cp->next_channel;
					cp->next_channel->prev_channel = cp->prev_channel; // fixing link
					free(cp);
					cp = NULL;
					u->channel_count--;
					flag =1;
				}
			}
			else cp = cp->next_channel;

		}
		if (flag ==1){ // removed from the user list, go to channel list

			cp = channel_list; // Iterate through channels
			while(cp != NULL){

				if(strcmp(channelname, cp->channel_name)==0){	// Found the correct channel in the list

					if(cp->user_count == 1)
						cp = obliterate_channel(cp);	// single user, erase channel

					else if(cp->user_count >1){  // else more users exist on this channel

						user* up = cp->head;
						while(up != NULL){

							if(strcmp(u->user_name, up->user_name ) == 0){ // found the right user
								if(up->prev_user != NULL && up->next_user == NULL){ // tail user
									up->prev_user->next_user = NULL;
									free(up->user_name);
									up = up->prev_user;
									free(up->next_user);
									cp->tail = up;
									cp->user_count--;
								}
								else if(up->prev_user == NULL && up->next_user != NULL){// head user
									up->next_user->prev_user = NULL;
									free(up->user_name);
									up = up->next_user;
									free(up->prev_user);
									cp->user_count--;
									cp->head = up;
								}
								else if(up->prev_user != NULL && up->next_user != NULL){ // middle user
									up->next_user->prev_user = up->prev_user;
									up->prev_user->next_user = up->next_user;
									free(up->user_name);
									free(up);
									cp->user_count--;
								}
							}
							else up= up->next_user;
						}	
					}	 // found the right channel
				}
				else cp = cp->next_channel;
			}
		}
	}
}

user* get_user(user* user_list, int port){

	if(user_list == NULL || port <0){

		printf("get_user, bad param\n");
		return NULL;
	}

	user* u = user_list;
	while(u!= NULL){

		if(u->port == port)
			return u;
		else u = u->next_user;

	}
	return NULL;

}

/*
// Leave the named channel
void leave_channel(user* u, channel* channel_list, char* toLeave){

	int uport = u->port;

	if(u == NULL || channel_list == NULL || toLeave== NULL){
		printf("Bad params: leave channel\n");
		exit(EXIT_FAILURE);
	}

	int x =0;
	channel* cp = u->head;
	channel* cpp = NULL;
	

	if(u->channel_count >1){					// Make sure the user has a channel to leave
		for(x =0; x < u->channel_count; x++){	// Iterate through channels, looking for match
			if(strcmp(toLeave, cp->channel_name )==0){

				if(cp == u->head){ // head channel matches in list >1
					free(cp->channel_name);
					cp = cp->next_channel;
					free(cp->prev_channel);
					u->head = cp;
					u->channel_count--;
				}
				else if( cp == u->tail){ // tail channel matches in list >1
					free(cp->channel_name);
					cp = cp->prev_channel;
					free(cp->next_channel);
					u->tail = cp;
					u->channel_count--;
				}
				else if(cp != u->tail && cp != u->head){

					free(cp->channel_name);
					cp->prev_channel->next_channel = cp->next_channel;
					cp->next_channel->prev_channel = cp->prev_channel;
					free(cp);
					u->channel_count--;
				}
			}
		}
	}
	else if(u->channel_count == 1){
		if(strcmp(toLeave, u->head->channel_name )==0){
			free(u->head->user_name);
			free(u->head);
			u->channel_count--;
			u->head = NULL;
			u->tail = NULL;
		}
	}	


	cpp = channel_list;

	while(cpp != NULL){






	}

*/

