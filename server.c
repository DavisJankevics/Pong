#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<errno.h>
#include<string.h>
#include<sys/mman.h>
#include "headers.h"
#include "network.h"


char *shared_memory = NULL;
int *client_count = NULL;
int *shared_data = NULL;



void get_shared_memory(struct SharedMemoryConfig*);
void gameloop(struct SharedMemoryConfig*);
void start_network(char*, struct SharedMemoryConfig*);
void accept_clients(struct SharedMemoryConfig*, int);
void process_client(unsigned char, int, struct SharedMemoryConfig*);
void recieve_client_socket(unsigned char, int, struct SharedMemoryConfig*);
void send_client_socket(unsigned char, int, struct SharedMemoryConfig*);
void process_client_data(unsigned char, struct SharedMemoryConfig*);
char* get_writeable_packet_in_buffer(char*, int);
char* get_readable_packet_in_buffer(char*, int);
char* get_client_buffer(char*, unsigned char, int);
void write_commit(char*);
void read_commit(char*);
void write_packet(char* packet, char id, int size, struct SharedMemoryConfig* sh_mem_config);

int main(int argc, char** argv){
    struct SharedMemoryConfig sh_mem_config;
    /* Here we keep all the information for easy shared memory access between processes */

    int pid = 0;
    char port[6];
    int port_info;
    port_info = get_port(argc, argv, port);
    int i;
    printf("Server started!\n");

    get_shared_memory(&sh_mem_config);
    /* create two processes - networking and game loop*/
    pid = fork();
    if(pid == 0){
        start_network(port, &sh_mem_config);
    }else{
        gameloop(&sh_mem_config);
    }
    return 0;
}
/*rewritten from example*/
void get_shared_memory(struct SharedMemoryConfig* sh_mem_config){
  /* The shared memory is built as follows:
  -----------------------------------------------------------------------------
  | 1b=client count | Incoming packets | Outgoing packets | Shared game state |
  -----------------------------------------------------------------------------
  
      The Incoming and Outgoing packets are arrays of packet buffers for each client:
      ------------------------------------------------------------------------------
      | Packet buffer for client=0 | ... |Packet buffer for client=(MAX_CLIENTS-1) |
      ------------------------------------------------------------------------------
  
          Each specific packet buffer contains the following information (buffer items and two numbers for checking if it is full/empty- int/out): 
          -------------------------------------------------------------------------------------------
          | 4b in | 4b out | full packet data [0] | ... | full packet data [PACKET_BUFFER_LENGTH-1] |
          -------------------------------------------------------------------------------------------

      The shared game state - this is updated in gameloop and is the source for packets sent back to clients:
      -----------------------------------------------------------------------------------
      | For now just a big array of GAME_STATE_SIZE bytes/chars - will be updated later |
      -----------------------------------------------------------------------------------

      -------------------------------------------------------------------------------------------
      |Player names and lobbies | lobby | gamestate  |
      -------------------------------------------------------------------------------------------
          Lobby
          ------------------------------------------------------
          | free lobby  | waiting lobby (has a player waiting) | 
          ------------------------------------------------------
          Gamestate packet data
          -------------------------------------------------------------------------------------------
          | in game MAX_PLAYERS * b | player_ready MAX_PLAYERS * b | player_not_afk MAX_PLAYERS * b |  
          -------------------------------------------------------------------------------------------
          Game info
          -------------------------------------------------------------------------------------------
          (| player_x | player_y | player_width| player_height| player_x_movement| player_y movement |)*2 
          | Ball_x | Ball_y | Ball _r | Ball_x_mov| Ball_y_move| Ball_pow |
          -------------------------------------------------------------------------------------------
  */

  int si = sizeof(int);
  int sf = sizeof(float);
  int maximum_packet_size = BYTES_PER_PACKET + MAX_TEAMS*BYTES_PER_TEAM + MAX_CLIENTS*BYTES_PER_CLIENT + MAX_BALLS*BYTES_PER_BALL + MAX_POWERUPS*BYTES_PER_POWERUP;
  int size_of_packet_buffer = si+si+ (maximum_packet_size)*PACKET_BUFFER_LENGTH;
  printf("  Getting the shared memory...\n");
  if(si!=4) {
      printf("  WARNING - integer size is not 4! Unexpected effects might arise...\n");
  }
  if(sf!=4) {
      printf("  WARNING - float size is not 4! Unexpected effects might arise...\n");
  }  
  sh_mem_config->shared_memory = mmap(NULL, 1+(size_of_packet_buffer*2)*MAX_CLIENTS+GAME_STATE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  sh_mem_config->client_count = (unsigned char*) (sh_mem_config->shared_memory);
  sh_mem_config->incoming_packets = (char*) ((sh_mem_config->shared_memory) + 1);
  sh_mem_config->outgoing_packets = (char*) ((sh_mem_config->incoming_packets)+(size_of_packet_buffer*MAX_CLIENTS));
  sh_mem_config->shared_gamestate_data = (char*) ((sh_mem_config->outgoing_packets)+(size_of_packet_buffer*MAX_CLIENTS));
  sh_mem_config->maximum_packet_size = maximum_packet_size;
  sh_mem_config->size_of_packet_buffer = size_of_packet_buffer;
  sh_mem_config->connected_player_id = (char* ) (sh_mem_config->shared_gamestate_data);
  sh_mem_config->client_Strings = (char**) (sh_mem_config->connected_player_id + MAX_CLIENTS);
  sh_mem_config->client_lobby = (char*) (sh_mem_config->client_Strings + (MAX_CLIENTS * 20));
  sh_mem_config->free_lobby_ID = (char*) (sh_mem_config->client_lobby + MAX_CLIENTS);
  sh_mem_config->waiting_lobby_ID = (char*) (sh_mem_config->free_lobby_ID + LOBBY_COUNT + 1); // 1 byte if there is waiting lobbies
  sh_mem_config->games = (char*) (sh_mem_config->waiting_lobby_ID + 1);
  sh_mem_config->in_game = (char*) (sh_mem_config->games);
  sh_mem_config->player_ready = (char*) (sh_mem_config->games+ MAX_CLIENTS);
  sh_mem_config->player_not_afk = (char*) (sh_mem_config->games + (MAX_CLIENTS * 2));
  sh_mem_config->packet_nr = (unsigned int*) (sh_mem_config->player_not_afk + (MAX_CLIENTS));
  sh_mem_config->last_packet_sent = (char* ) (sh_mem_config->packet_nr + (MAX_CLIENTS * 4));
  sh_mem_config->last_keyboard_input = (char*) (sh_mem_config->last_packet_sent + MAX_CLIENTS);
  sh_mem_config->game_info = (char* ) (sh_mem_config->last_keyboard_input + MAX_CLIENTS);

  sh_mem_config->waiting_lobby_ID[0] = 0;
  sh_mem_config->free_lobby_ID[0] = 0;
  
  printf("  Shared memory acquired!\n\n");
  return;
}
void gameloop(struct SharedMemoryConfig* sh_mem_config){

  int k = 0;
  int i = 0;
  int p = 0;
  char flag = 1;
  int packetSize;
  
  char packet[sh_mem_config->maximum_packet_size];
  char id, id2;
  unsigned char client_count = *(sh_mem_config->client_count);
  struct data_array data[45];
  int pn;

  printf("[Entering GAME LOOP!]\n");
  while(1){
    for( i = 0 ; i < client_count ; i++){
      
      id = sh_mem_config->connected_player_id[i];
      if(sh_mem_config->client_Strings[id] == NULL) continue;
      
      if(sh_mem_config->in_game[id] == 1 && sh_mem_config->last_packet_sent[id] >= 4){
        if(sh_mem_config->player_ready[id] == 0){
          add_packet_data(data, 5, id, id2, sh_mem_config);
          pn = sh_mem_config->packet_nr[id];
          pn++;
          packetSize = createPacket(packet, data, pn , 5 , 122);
          write_packet(packet, id, packetSize, sh_mem_config);
          sh_mem_config->packet_nr[id] = pn;

        }else{
          // caalculations - updating all locations
          printf("MAKING 7TH\n");
          add_packet_data(data, 7, id, id2, sh_mem_config);
          pn = sh_mem_config->packet_nr[id];
          pn++;
          packetSize = createPacket(packet, data, pn , 7, 137);
          write_packet(packet, id, packetSize, sh_mem_config);
          sh_mem_config->packet_nr[id] = pn;
        // they are in the game and accepted ready to play
        // Making 7th packet
        // checking for game end  
        }
  
      }else{
        printf("CLIENT NAME :  %s ",sh_mem_config->client_Strings[id]);
        printf("LOBBY :  %d\n",sh_mem_config->client_lobby[id]);
        for( k = i + 1 ; k < client_count ; k++){ // double loop isn't the most efficient tho, carrefull about mutexes
          id2 = sh_mem_config->connected_player_id[k];
          if(sh_mem_config->client_Strings[id2] == NULL) continue;

            printf("CLIENT NAME :  %s ",sh_mem_config->client_Strings[id2]);
            printf("LOBBY :  %d\n",sh_mem_config->client_lobby[id2]);
          if(k != i && sh_mem_config->client_lobby[id] == sh_mem_config->client_lobby[id2]){
            // out_buffer_2 = get_client_buffer(sh_mem_config->outgoing_packets, id2, size_of_packet_buffer);

              // they are in the same lobby - need to play (currently only 1v1 games)
              // Making the 4th packet

            data->size = 5;
            data[0].type = is_char;
            data[0].val.charVal = 2;
            data[1].type = is_char;
            data[1].val.charVal = id;
            data[2].type = is_string;
            data[2].val.stringVal = sh_mem_config->client_Strings[id];

            data[3].type = is_char;
            data[3].val.charVal = id2;
            data[4].type = is_string;
            data[4].val.stringVal = sh_mem_config->client_Strings[id2];
            pn = sh_mem_config->packet_nr[id];
            pn++;
            packetSize = createPacket(packet, data, pn , 4 , 43);
            write_packet(packet, id, packetSize, sh_mem_config);

            sh_mem_config->packet_nr[id] = pn;
            sleep(1);
            pn = sh_mem_config->packet_nr[id2];
            pn++;
            packetSize = createPacket(packet, data, pn , 4 , 43);
            write_packet(packet, id2, packetSize, sh_mem_config);

            sh_mem_config->packet_nr[id2] = pn;
            // sleep(1);
            sh_mem_config->in_game[id] = 1;
            sh_mem_config->in_game[id2] = 1;
          }
        }
      }
    }
    sleep(1); // just to slow down the text output
    client_count = *(sh_mem_config->client_count);
  }
}
void start_network(char* port, struct SharedMemoryConfig* sh_mem_config){
    int main_socket = -1;
    main_socket = get_server_socket(port);
    if(main_socket == -1 ){
        printf("Error opening main socket!\n");
        exit(1);
    }
    printf("Main socket created!\n");
    printf("  Starting accepting clients.. \n");
    accept_clients(sh_mem_config, main_socket);
    printf("  Stopped accepting clients (Should not happen!) - server networking stopped!\n");
}

void accept_clients(struct SharedMemoryConfig* sh_mem_config, int server_socket){
  unsigned char new_client_id = 0;
  int tid = 0;
  int client_socket = -1;
  *(sh_mem_config->client_count) = 0;


  while(1){
    client_socket = accept(server_socket, NULL, NULL);
    if(client_socket < 0){
      printf("ERROR accepting the client connection! ERRNO=%d Continuing...\n", errno);
      continue;
    }
    if(*(sh_mem_config->client_count) == MAX_CLIENTS){
        printf("Client count exceeding MAX_CLIENTS - ignoring new connections unil this changes, closing incoming connection!");
        close(client_socket);
        continue;        
    }
    new_client_id = *(sh_mem_config->client_count);
    *(sh_mem_config->client_count) += 1;
    int player_count = *(sh_mem_config->client_count);
    sh_mem_config->connected_player_id[player_count-1] = new_client_id;
    int i;
    for( i = 0 ; i < MAX_CLIENTS ; i ++){printf("ID : %d\n", sh_mem_config->connected_player_id[i]);}
    printf("NEW : %d\n",new_client_id);

    printf("CLIENTS : %d\n",*sh_mem_config->client_count);

    /* double fork and orphan to process, main thread closes socket and listens for a new one */
    tid = fork();
    if(tid == 0){
      close(server_socket);
      tid = fork();
      if(tid == 0){
        /* Need to check for disconnections! */
        process_client(new_client_id, client_socket, sh_mem_config);
        exit(0);
      } else {
        wait(NULL);
        printf("Successfully orphaned client %d\n", new_client_id);
        exit(0);
      }
    }else
      close(client_socket);

  }
}

void process_client(unsigned char id, int socket, struct SharedMemoryConfig* sh_mem_config){
  int tid = 0;
  printf("Processing client ID = %d, with socket ID = %d - creating two porocesses (1) networking, (2) data processing...\n", id, socket);

  tid = fork();
  if(tid == 0){
    close(socket);
    process_client_data(id, sh_mem_config);
  } else {
    tid = fork();
    if(tid==0){
      recieve_client_socket(id, socket, sh_mem_config);
    } else {
      send_client_socket(id, socket, sh_mem_config);
    }
  }
}


void recieve_client_socket(unsigned char id, int socket, struct SharedMemoryConfig* sh_mem_config){
    int buffer_size = sh_mem_config->size_of_packet_buffer;
    char* incoming_buffer = get_client_buffer(sh_mem_config->incoming_packets, id, buffer_size);
    int maximum_packet_size = sh_mem_config->maximum_packet_size;
    char* cur_packet = NULL;
    int i;
    char in_buf[BIGGEST_NET_BUFFER];
    char packet_check;
    int last_packet_nr = 0;
    printf("For client ID = %d, initiating networking loop for RECIEVING on socket ID = %d\n", id, socket);
    int read_bytes;
    while(1){
        read_bytes = readPacket(socket, in_buf);
        if(read_bytes != -1){
          packet_check = check_packet(in_buf,last_packet_nr);

          if(packet_check != -1){
              printf("--------------------------\n");
              printf("Received info :\n");
              print_bytes(in_buf,read_bytes);
              while((cur_packet = get_writeable_packet_in_buffer(incoming_buffer, maximum_packet_size)) == NULL) { sleep(1); }
              for( i = 0 ; i < read_bytes; i++) {cur_packet[i] = in_buf[i];}
              write_commit(incoming_buffer);
              print_bytes(cur_packet,read_bytes);
              last_packet_nr = get_int_from_bytes(in_buf+2);
          }
        }
    }
//   printf("The socket ended/is empty!\n");
}

void send_client_socket(unsigned char id, int socket, struct SharedMemoryConfig* sh_mem_config){
  int size_of_packet_buffer = sh_mem_config->size_of_packet_buffer;
  char* out_buffer = get_client_buffer(sh_mem_config->outgoing_packets, id, size_of_packet_buffer);
  int maximum_packet_size = sh_mem_config->maximum_packet_size;
  char* cur_packet = NULL;
  int size;
  char buffer[maximum_packet_size];
  int i;
  printf("For client ID = %d, initiating networking loop for SENDING on socket ID = %d\n", id, socket);

  while(1){
    while((cur_packet = get_readable_packet_in_buffer(out_buffer, maximum_packet_size))==NULL){ sleep(1); }
    for(i = 0; i < 11 + get_int_from_bytes(cur_packet+7)+3; i++){
      buffer[i] = cur_packet[i];
    }
    size = check_and_escape(buffer, 11 + get_int_from_bytes(buffer+7) + 3);
    if(buffer[0] != 0){
      send(socket, buffer, size, 0);
      sh_mem_config->last_packet_sent[id] = buffer[6];
      printf("Sent packet with NPK = %d and TYPE=%d...to %d printing.....\n", get_int_from_bytes(buffer+2),buffer[6], socket);

      print_bytes(buffer, size);
    }
    read_commit(out_buffer);
    sleep(1);
  }
}


void process_client_data(unsigned char id, struct SharedMemoryConfig* sh_mem_config){
  int size_of_packet_buffer = sh_mem_config->size_of_packet_buffer;
  char* in_buffer = get_client_buffer(sh_mem_config->incoming_packets, id, size_of_packet_buffer);
  char* out_buffer = get_client_buffer(sh_mem_config->outgoing_packets, id, size_of_packet_buffer);
  int maximum_packet_size = sh_mem_config->maximum_packet_size;
  char* cur_in_packet = NULL;
  char* cur_out_packet = NULL;
  char packet[maximum_packet_size];
  int packetSize;
  unsigned int pn;
  int client_count;
  int i = 0;
  char *name;
  struct data_array data[1];
  printf("For client ID = %d, starting data processing (reading acquired packets into gamestate, creating response packets)\n", id);
  while(1){
    while((cur_in_packet = get_readable_packet_in_buffer(in_buffer, maximum_packet_size))==NULL){ sleep(1); } /* nothing to process, spin in place */
    unsigned char packet_id = cur_in_packet[6];
    pn = get_int_from_bytes(cur_in_packet + 2);

    switch (packet_id)
    {
    case 1:
      /* player name send - putting into lobby */
      name = cur_in_packet+11;
      sh_mem_config->client_Strings[id] = name; // saving the name of the player in the lobby
      
      if(sh_mem_config->waiting_lobby_ID[0] >= 1){
        sh_mem_config->client_lobby[id] = sh_mem_config->waiting_lobby_ID[sh_mem_config->waiting_lobby_ID[0]];
        sh_mem_config->waiting_lobby_ID[0] -= 1;
        // printf("WAITING %d\n",sh_mem_config->waiting_lobby_ID[0]);
      }else{
        sh_mem_config->client_lobby[id] = sh_mem_config->free_lobby_ID[0];
        sh_mem_config->waiting_lobby_ID[0] += 1;
        sh_mem_config->free_lobby_ID[0] += 1;
      }
      pn++;
      client_count = sh_mem_config->client_count;
      data->size = 1;
      data[0].type = is_char;
      data[0].val.charVal = id;

      packetSize = createPacket(packet, data, pn , 2 , 1);
      while((cur_out_packet = get_writeable_packet_in_buffer(out_buffer, packetSize))==NULL){ sleep(1); } /* nowhere to write to, spin in place */
      for(i=0; i<packetSize;i++) cur_out_packet[i] = packet[i];
      sh_mem_config->packet_nr[id] = pn;

      // print_bytes(cur_out_packet, packetSize);
      // make and send the 2nd packet - accepting the player
      break;
    case 3:
      /* MEssages */
      break;
    case 6:
      /* Game ready accept */
      sh_mem_config->in_game[id] = 1;
      sh_mem_config->player_ready[id] = 1;
      sh_mem_config->player_not_afk[id] = 1;
      break;
    case 8:
      /* keyboard inputs */
      sh_mem_config->last_keyboard_input[id] = cur_in_packet[12]; // 11?
      break;
    case 9:
      sh_mem_config->player_not_afk[id] = 1;
      /* AFK CHECK */
        break;
    default:
      break;
    }
    read_commit(in_buffer);
    write_commit(out_buffer);

    sleep(1);
  }
  return;
}

char* get_client_buffer(char* buffer, unsigned char client_id, int size_of_packet_buffer){
  if(client_id >= MAX_CLIENTS) return NULL;
  return buffer+client_id*size_of_packet_buffer;
}


char* get_writeable_packet_in_buffer(char* buffer, int max){
  /* This function takes one of packet buffers (incoming or outgoing) for specific client
     and finds the first writeable (empty) packet, and returns pointer to it.
     If no writeable packet exists, it returns NULL */
     int* in = (int*)(buffer);
     int* out = (int*)(buffer+4);
     if(((*in+1) % PACKET_BUFFER_LENGTH) != *out){
       return buffer+8+(*in)*max;
     }

     return NULL;
}

void write_commit(char* buffer){
     int* in = (int*)(buffer);
     *in = (*in+1) % PACKET_BUFFER_LENGTH;    
}

char* get_readable_packet_in_buffer(char* buffer, int max){
  /* This function takes one of packet buffers (incoming or outgoing), for specific client
     and finds the first readable full and returns pointer to it.
     If no readable packet exists, it returns NULL */
     int* in = (int*)(buffer);
     int* out = (int*)(buffer+4);

     if(*out!=*in){
       return buffer+8+(*out)*max;  
     } 
     return NULL;
}

void read_commit(char* buffer){
     int* out = (int*)(buffer+4);
     *out = (*out+1) % PACKET_BUFFER_LENGTH;
}

void write_packet(char* packet, char id, int size, struct SharedMemoryConfig* sh_mem_config){
  char* out_buffer;
  char* cur_out_packet = NULL;
  int size_of_packet_buffer = sh_mem_config->size_of_packet_buffer;
  int i;
  out_buffer = get_client_buffer(sh_mem_config->outgoing_packets, id, size_of_packet_buffer);


  while((cur_out_packet = get_writeable_packet_in_buffer(out_buffer, size))==NULL){ sleep(1); } /* nowhere to write to, spin in place */
  for(i=0; i<size;i++) cur_out_packet[i] = packet[i];
  print_bytes(cur_out_packet, size);
  write_commit(out_buffer);
}