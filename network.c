#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include "headers.h"
#include "network.h"

int get_server_socket(char* port){
  int server_socket;
  int opt_value = 1;
  struct addrinfo hints, *list_of_addresses, *a;

  printf("Opening server socket on port %s\n", port);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;  
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, port, &hints, &list_of_addresses);
  if(list_of_addresses == NULL) printf("No valid addresses!\n");
  for(a = list_of_addresses; a!=NULL; a=a->ai_next){
    printf("... Creating socket ...\n");
    if((server_socket = socket(a->ai_family, a->ai_socktype, a->ai_protocol))<0)
      continue;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt_value, sizeof(int));
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, (const void*)&opt_value, sizeof(int));
    printf("... Binding socket ...\n");
    if(bind(server_socket, a->ai_addr, a->ai_addrlen)==0)
      break; 
    printf("ERROR binding server! ERRNO=%d\n", errno);
    close(server_socket);
  }
   
  if(a==NULL) {
    printf("No connection was made - cleanup...\n");
    freeaddrinfo(list_of_addresses);
    return -1;
  } 
  freeaddrinfo(list_of_addresses);
  printf("... Listening to socket ...\n");
  if(listen(server_socket, MAX_CLIENTS)<0){
    close(server_socket);
    return -1;
  }
  printf("Server socket successfully opened - listening...\n");
  return server_socket;
  }

int get_client_socket(char* host, char* port){
  int client_socket = -1;
  struct addrinfo hints, *list_of_addresses, *a;
  printf("Opening client socket to %s:%s\n", host, port);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  getaddrinfo(host, port, &hints, &list_of_addresses);

  if(list_of_addresses == NULL) printf("No valid addresses!\n");
  for(a = list_of_addresses; a!=NULL; a=a->ai_next){
    printf("... Creating socket ...\n");
    if((client_socket = socket(a->ai_family, a->ai_socktype, a->ai_protocol))<0)
      continue;

    if(connect(client_socket, a->ai_addr, a->ai_addrlen)!=-1)
      break;
    printf("ERROR conneting to socket! ERRNO=%d\n", errno);
    close(client_socket);
  }
  if(a==NULL) {
    printf("No connection was made - cleanup...\n");
    freeaddrinfo(list_of_addresses);
    return -1;
  } 
  freeaddrinfo(list_of_addresses);

  return client_socket;
}


void add_packet_data(struct data_array *data, int packet_id, char player_id1, char player_id2, struct SharedMemoryConfig* sh_mem_config){
    int k, m, size;
    size = 69;
    m = 0;
    char id, lobby_id;
    char item_position;
    data->size = 0;
    char* p;
    if(packet_id == 5){

      lobby_id = sh_mem_config->client_lobby[player_id1];
      sh_mem_config->game_info[id * size] = INITIAL_X_1;

      sh_mem_config->game_info[id * size + 4] = (INITIAL_Y_1);

      sh_mem_config->game_info[id * size + 8] = (INITIAL_X_2);

      sh_mem_config->game_info[id * size + 12] = (INITIAL_Y_2);

      sh_mem_config->game_info[id * size + 16] = (0.0);

      sh_mem_config->game_info[id * size + 20] = (0.0);


      sh_mem_config->game_info[id * size + 24] = (-INITIAL_X_1);

      sh_mem_config->game_info[id * size + 28] = (INITIAL_Y_1);

      sh_mem_config->game_info[id * size + 32] = (-INITIAL_X_2);

      sh_mem_config->game_info[id * size + 36] = (INITIAL_Y_2);

      sh_mem_config->game_info[id * size + 40] = (0.0);

      sh_mem_config->game_info[id * size + 44] = (0.0);

      sh_mem_config->game_info[id * size + 48] = (0.0); // Ball

      sh_mem_config->game_info[id * size + 52] = (0.0);

      sh_mem_config->game_info[id * size + 56] = (50.0);

      sh_mem_config->game_info[id * size + 60] = (1.0);

      sh_mem_config->game_info[id * size + 64] = (1.0);

      p[0] = 0;
       
    }
    if(packet_id == 5 || packet_id ==7){
        
        data[m].type = is_int;
        data[m].val.intVal = STANDARD_WIDTH; m++;
        data[m].type = is_int;
        data[m].val.intVal = STANDARD_HEIGHT; m++;
        data[m].type = is_char;
        data[m].val.charVal = 2; m++;
        for( k = 0 ; k < 2 ; k++){
            data[m].type = is_char;
            data[m].val.charVal = k; m++;
            data[m].type = is_float;// team goal locations
            data[m].val.floatVal = TEAM_ONE_GOAL_X1 + (2 * TEAM_ONE_GOAL_X1 * (-k)); m++;
            data[m].type = is_float;
            data[m].val.floatVal = TEAM_ONE_GOAL_X1 + (2 * TEAM_ONE_GOAL_X1 * (-k)); m++;
            data[m].type = is_float;
            data[m].val.floatVal = TEAM_ONE_GOAL_Y1; m++;
            data[m].type = is_float;
            data[m].val.floatVal = TEAM_ONE_GOAL_Y2; m++;
        }
        data[m].type = is_char;
        data[m].val.charVal = 2; m++;
        for( k = 0 ; k < 2 ; k++){// player locations
            if(k == 0){
              id = player_id1;
              item_position = 0;
            }else{
              id = player_id2;
              item_position = 6 * 4;
            }
            data[m].type = is_char;
            data[m].val.charVal = id; m++;
            data[m].type = is_char;
            data[m].val.charVal = sh_mem_config->player_ready[id]; m++;
            data[m].type = is_char;
            data[m].val.charVal = k; m++;
            data[m].type = is_string;
            data[m].val.stringVal = sh_mem_config->client_Strings[id]; m++;
            
            data[m].type = is_float;
            data[m].val.floatVal = sh_mem_config->game_info[lobby_id * size + item_position]; m++; item_position+=4;
            data[m].type = is_float;
            data[m].val.floatVal = sh_mem_config->game_info[lobby_id * size + item_position]; m++; item_position+=4;
            data[m].type = is_float;
            data[m].val.floatVal = sh_mem_config->game_info[lobby_id * size + item_position]; m++; item_position+=4;
            data[m].type = is_float;
            data[m].val.floatVal = sh_mem_config->game_info[lobby_id * size + item_position]; m++; item_position+=4;
        }
        data->size = m;    
            
    }if (packet_id == 7){
      data[m].type = is_char;
      data[m].val.charVal = id; m++;
      for( k = 0 ; k < data[m-1].val.charVal; k ++){
        data[m].type = is_float; 
        data[m].val.floatVal = sh_mem_config->game_info[lobby_id + 48];m++; //x
        data[m].type = is_float;
        data[m].val.floatVal = sh_mem_config->game_info[lobby_id + 52];m++; //y
        data[m].type = is_float;
        data[m].val.floatVal = sh_mem_config->game_info[lobby_id + 56];m++;// radius
        data[m].type = is_char;
        data[m].val.charVal = sh_mem_config->game_info[lobby_id + 69];m++; // colour
      }
      data[m].type = is_char;
      data[m].val.charVal = id; m++;
      
      for( k = 0 ; k < data[m-1].val.charVal; k ++){
        data[m].type = is_char;
        data[m].val.charVal = 1;m++; // type
        data[m].type = is_float; 
        data[m].val.floatVal = 1;m++; //x
        data[m].type = is_float; 
        data[m].val.floatVal = 1;m++; //y
        data[m].type = is_float;
        data[m].val.floatVal = 1;m++;//width
        data[m].type = is_float;
        data[m].val.floatVal = 1;m++;// height
        }
    }
}