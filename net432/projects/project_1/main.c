#include "list.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define REQ_LOGIN 0
#define REQ_LOGOUT 1
#define REQ_JOIN 2
#define REQ_LEAVE 3
#define REQ_SAY 4
#define REQ_LIST 5
#define REQ_WHO 6
#define TXT_SAY 0
#define TXT_LIST 1
#define TXT_WHO 2
#define TXT_ERROR 3

int
main(int argc, char ** argv){
   user* first_user = NULL;
   int i = 0, numberofclients = 0;
   
   char* user_name = "Leeroy\0";
   char* second_name = "Jenkins\0";
   char* logout_user = "logout_user";
   struct channel* common = init_channel(common, "Common");  // Always have common on startup
   int packet_type = 0;
   int x = 99;
   for(i=0; i<2; i++){
      switch(packet_type){

         case REQ_LOGIN:              // Login request received
            if(numberofclients == 0 ){   // First user to join?
                                       // Yes
               first_user = add_new_user(first_user, user_name, x++); // Init first user
               common = add_user_to_channel(common, user_name );           // Adds new user to common's user list
               first_user = add_channel_to_user(first_user, common->channel_name); // Adds "common" to new user's channel list
        	      numberofclients++; 
            }                                                      // Not first user
            else if(numberofclients >0){
               user* new_client = add_new_user(first_user, second_name, x++ ); //Init new_client; change user_name
               common = add_user_to_channel(common, new_client->user_name);            // Add new user to "Common" list
               new_client = add_channel_to_user(new_client, common->channel_name);         // Add "common" to user's channel list
               
               //new_client = NULL;
               free(new_client);
		         numberofclients++;
            }
            break;
            
         case REQ_LOGOUT:

            break;
      }
      
   }

user* new_clien = add_new_user(first_user, logout_user, x++); //Init new_client; change user_name
              common= add_user_to_channel(common, new_clien->user_name);            // Add new user to "Common" list
               new_clien = add_channel_to_user(new_clien, common->channel_name);         // Add "common" to user's channel list
               numberofclients++;


   list_all_channels(common);
   list_users(common);

   printf("Logging out logout user\n");
   logout(new_clien, common);
               new_clien = NULL;
               //free(new_client->user_name);
              // free(new_clien);

   printf("Logged out, reprinting!!!!!!\n");
   list_all_channels(common);
   list_users(common);

   printf("\n\nTrying to leave_channel\n");
   leave_channel(first_user->next_user, common, "Common");
   list_users(common);

   printf("Jenkins gone?\n");



   free_all_users(first_user);
   int i =-1;
   i = free_all_channels(common);
   if (i == 0)
      printf("All channels free\n");

   list_all_channels(common);
   list_users(common);   

return 0;
}
