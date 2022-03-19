#ifndef MY_NET_LIB
#define MY_NET_LIB
#include <errno.h>
#define MAX_CLIENTS 10 /* must be less than 128 */
#define PACKET_BUFFER_LENGTH 5
#define BIGGEST_NET_BUFFER 10000
#define MINIMUM_PACKET_SIZE 10
#define DEFAULT_PORT "12345"
#define DEFAULT_IP "127.0.0.1"

#define MAX_CLIENTS 10
#define SHARED_MEMORY_SIZE 1024

#define MAX_TEAMS 3
#define BYTES_PER_TEAM 13
#define BYTES_PER_CLIENT 18
#define MAX_BALLS 10
#define BYTES_PER_BALL 13
#define MAX_POWERUPS 10
#define BYTES_PER_POWERUP 17
#define BYTES_PER_PACKET 22 /* 10 per overall packet, 12 per item counts and width/height in GameState packet */
#define GAME_STATE_SIZE 10000 /* double check how big and how to store data */
#define LOBBY_COUNT 5


struct SharedMemoryConfig {
  char* shared_memory;
  unsigned char* client_count;
  char* incoming_packets;
  char* outgoing_packets;
  char* shared_gamestate_data;
  int maximum_packet_size;
  int size_of_packet_buffer;
  char* connected_player_id;
  char **client_Strings;
  char *client_lobby;
  char *free_lobby_ID;
  char *waiting_lobby_ID;
  char *games;
  char *in_game;
  char *player_ready;
  char *player_not_afk;
  char *game_info;
  unsigned int *packet_nr;
  char *last_packet_sent;
  char *last_keyboard_input;

};
int get_server_socket(char*);
int get_client_socket(char*, char*);
int get_port_parameter(int, char**, char*);
int get_host_parameter(int, char**, char*);

void add_packet_data(struct data_array*, int id, char, char , struct SharedMemoryConfig* sh_mem_config);


#endif
