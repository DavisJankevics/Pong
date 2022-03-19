#ifndef SERVER_CLIENT_HEADERS
#define SERVER_CLIENT_HEADERS


#define STANDARD_WIDTH 1240
#define STANDARD_HEIGHT 680
#define CLIENT_PACKET_HOLDER_SIZE 1000
#define TEAM_ONE_GOAL_X1 -340.0
#define TEAM_ONE_GOAL_X2 -340.0
#define TEAM_ONE_GOAL_Y1 -1240.0
#define TEAM_ONE_GOAL_Y2  1240.0
#define TEAM_TWO_GOAL_X1  340.0
#define TEAM_TWO_GOAL_X2  340.0
#define INITIAL_X_1 -300.0
#define INITIAL_Y_1 -300.0
#define INITIAL_X_2 300.0
#define INITIAL_Y_2 -300.0
#define INITIAL_WIDTH 500.0
#define INITIAL_HEIGHT 30.0


struct data_array{
    int size;
    enum { is_int, is_float, is_char, is_string } type;
    union {
        int intVal;
        float floatVal;
        char charVal;
        char* stringVal;
    } val;
};
struct player_packets{
    char* incoming;
    char* outgoing;
    int incoming_empty;
    int outgoing_empty;
};

int str_length(char*);
int str_copy(char*, char*);
int str_find(char*, char*);
int get_port(int,char**, char*);
int get_ip(int, char**, char*);
char printable_char(char);
void print_bytes(void*, int);
void universal_store_int_as_bytes_big_endian(void*, int);
void universal_store_int_as_bytes_little_endian(void*, int);
int createPacket(void* packet, struct data_array *data, unsigned int PacketNumber, unsigned char PacketID, int dataLength);
int readPacket(int socket, char* buf);
int check_and_escape(char *packet, int size);
void process_packet(char* packet);
char checkSum(void* packet, int len);
int get_int_from_bytes(char* packet);
float get_float_from_bytes(char* packet);
char check_packet(char* packet, int); 
void universal_store_float_as_bytes_big_endian(void* packet, float data);
#endif