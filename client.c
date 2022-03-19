#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include "headers.h"
#include "network.h"
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include <sys/mman.h>

void myInit();
void myKeyboard(unsigned char key, int x, int y);
void mySpecialKeyboard(int key, int x, int y);
void myDisplay(char* packet);
void myMouseInputs();

int start_client_connection(char* host, char* port);
void socket_listener(int socket);
void data_sender(int socket);
void gameloop();
void RenderString(float x, float y, void *font, const char* string, float, float, float);
void first_screen();
void lobby();
void game_screen(char* packet);
void make_packet(int id, char, char ,char*);
void game_accept();

int state = 0;
char keys[5];
char player_name[20];
int packetNr = 1;
struct player_packets *pp;
char player_ID;
char* opponent[20];

int main(int argc, char** argv){
    struct player_packets player_stored_packets;
    player_stored_packets.incoming = mmap(NULL, CLIENT_PACKET_HOLDER_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    player_stored_packets.outgoing = mmap(NULL, CLIENT_PACKET_HOLDER_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    player_stored_packets.outgoing_empty = 1; // currently not used
    player_stored_packets.incoming_empty = 1; // currently not used
    pp = &player_stored_packets;
    opponent[0] = '\0';
    char host[255];
    char port[6];
    int pid;
    int client_socket = -1;
    player_ID = -1;

    // int packetNr;
    // packetNr = 1;
    int host_info, port_info, packet_size, state;
    port_info = get_port(argc, argv, port);
    host_info = get_ip(argc, argv, host);
    if(port_info == -1 || host_info == -1){
        printf("Parameter reading error!\n");
        return -1;
    }
    client_socket = get_client_socket(host, port);
    if(client_socket == -1){
        printf("ERROR opening client socket - exiting!\n");
        exit(-1);
    }
    pid = fork();
    if(pid == 0){
        socket_listener(client_socket);
    } else {
        pid = fork();
        if(pid == 0){
            data_sender(client_socket);
        } else {
            close(client_socket);
                glutInit(&argc, argv);
                myInit();
                glutKeyboardFunc(myKeyboard);
                glutSpecialFunc(mySpecialKeyboard);
                glutMouseFunc(myMouseInputs);
                glutIdleFunc(gameloop);
                glutDisplayFunc(myDisplay);
                glutMainLoop(); 
                /*glutSwapBuffers();*/

                /*drawing*/
                glBegin(GL_LINE_LOOP); /* line loop = connects back after 4 points*/

                glVertex2f(4.0,200.0);
                /*glVertex2i(100,200);*/
                glEnd();
            // gameloop();  
        }      
    }  
    
    return 0;

}

void socket_listener(int socket){
    char* cur_packet = NULL;
    int i;
    char in_buf[BIGGEST_NET_BUFFER];
    char packet_check;
    int last_packet_nr = 0;
    int read_bytes;
    printf("RECEIVING!\n");
    while(1){
        read_bytes = readPacket(socket, in_buf);
        if (read_bytes == -1) break;
        packet_check = check_packet(in_buf,last_packet_nr);
        if(read_bytes != -1 && packet_check){
            printf("--------------------------\n");
            printf("Received info :\n");

            for( i = 0 ; i < read_bytes; i++) {pp->incoming[i] = in_buf[i];} // storing the last packet in incoming sector
            print_bytes(pp->incoming, read_bytes);
            last_packet_nr = get_int_from_bytes(in_buf+2);
            pp->incoming_empty = 0;
            }
    }
}

void data_sender(int socket){
    int size, i;
    char* buff;
    int last_sent_packet_nr = 0;
    int cur_packet_nr = 0;

    i = 0;
    while(1){
        if(pp->outgoing[0] == '-'){
            cur_packet_nr = get_int_from_bytes(pp->outgoing+2);
            if(cur_packet_nr > last_sent_packet_nr){
                buff = pp->outgoing;
                size = check_and_escape(buff, 11 + get_int_from_bytes(buff+7) + 3);
                send(socket, buff, size,0);

                print_bytes(pp->outgoing,size);
                last_sent_packet_nr = cur_packet_nr;
                pp->outgoing_empty = 1;
                printf("Sent packet nr %d \n",last_sent_packet_nr);
            }
        }
    }
}

void gameloop(){
    usleep(50000);

    if(keys[4] == 1) exit(0);
    
    if(packetNr <= get_int_from_bytes(pp->incoming + 2)){

        char* incoming = pp->incoming;
        unsigned int pn = get_int_from_bytes(pp->incoming+2);
        unsigned char Id = pp->incoming[6];
        char name_read = 0;
        int i;
        char* name_one = malloc(20), *name_two = malloc(20);
        switch (Id)
        {
        case 2:
            break;
        case 4:
            // state = 3;
            if(!name_read){
                strcpy(name_one , pp->incoming+13);
                strcpy(name_two , pp->incoming+34);
                // printf("%s %s\n",name_one, name_two);
                name_read = 1;
            }
            if(strcmp(name_one, player_name) == 0){
                strcpy(opponent ,name_two);
            }else{
                strcpy(opponent ,name_one);
            }
            break;
        case 5 :
            // printf("%f\n",get_float_from_bytes(pp->incoming+11+11));
            state = 2;
            break;
        case 7 :
            state = 3;

            break;
        default:
            break;
        }
    packetNr = pn;
    }
    myDisplay(pp->incoming);
    // add signal to send packet 9
}

void myInit( void ){
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1240,680);
    glutInitWindowPosition(0,0);
    glutCreateWindow("Game window");
    
    glClearColor(0.1,0.1,0.3,1.0); /*background coloring*/

    glColor3f(0.0f,1.0f,0.0f); /*color for drawing*/
    glPointSize(1.0); /*sets the drawable pixel count*/

    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(-620,620,-340,340);  /*for 2d*/ 
}


void myKeyboard(unsigned char key, int x, int y){
    // printf("%d %d\n",key, name_length);
    int name_length = strlen(player_name);
    printf("%d %d\n",key, name_length);
    if(state == 0){
        if (name_length < 20){
            if(printable_char(key) != '#'){
            player_name[name_length] = key;
            player_name[name_length+1] = '\0';
            }
        }
        if( key == 8 && name_length > 0 && state == 0){
            player_name[name_length-1] = '\0';
        }
        if( key == 13 ){
            make_packet(1,'\0','\0',"");
            state = 1;
        }
    }
    int modifiers = glutGetModifiers();
    if(modifiers & GLUT_ACTIVE_SHIFT) printf("SHIFT is pressed!\n");
    if(modifiers & GLUT_ACTIVE_CTRL) printf("CTRL is pressed!\n");
    if(modifiers & GLUT_ACTIVE_ALT) printf("ALT is pressed!\n");

    if(key == 27){ /*ESC*/
    // if(key == 'q' || key == 'Q'){
        keys[4] = 1;
    }
}
void mySpecialKeyboard(int key, int x, int y){
    if(key == GLUT_KEY_UP){
        keys[0] = 1;
    }
        if(key == GLUT_KEY_DOWN){
        keys[1] = 1;
    }
        if(key == GLUT_KEY_LEFT){
        keys[2] = 1;
    }
        if(key == GLUT_KEY_RIGHT){
        keys[3] = 1;
    }
}

void myDisplay(char* packet){
    glClear(GL_COLOR_BUFFER_BIT);
    switch (state)
    {
    case 0: first_screen(player_name);break;
    case 1: lobby();break;
    case 2: game_accept(); break;
    case 3: game_screen(packet);break;
    
    default:
        break;
    }
    glutSwapBuffers();
}

void circle (float x, float y,float r){
    float cx,cy;
    glBegin(GL_LINE_STRIP);

    float i = 0.0;
    while ( i <= 2* M_PI ){
        cx = x + 5 * sin(i);
        cy = y + 5 * cos(i);
        i = i + 0.1;
        glVertex2f(cx,cy);
    }
    glEnd();
}


void RenderString(float x, float y, void *font, const char* string, float r, float g, float b)
{  
  glColor3f(r,g,b); 
  glRasterPos2f(x, y);
  glutBitmapString(font, string);
}

void first_screen(char* name){

    glBegin(GL_LINE_LOOP); /*Frame*/
        glVertex2i(-600,-320);
        glVertex2i(-600,320);
        glVertex2i(600,320);
        glVertex2i(600,-320);
    glEnd();

    glBegin(GL_LINE_LOOP);/*Button start*/
        glVertex2i(-100,-200);
        glVertex2i(-100,-150);
        glVertex2i(100,-150);
        glVertex2i(100,-200);
    glEnd();

    glBegin(GL_LINE_LOOP);/*Name input*/
        glVertex2i(-200,-100);
        glVertex2i(-200,-50);
        glVertex2i(200,-50);
        glVertex2i(200,-100);
    glEnd();
    RenderString(-100,-85,GLUT_BITMAP_TIMES_ROMAN_24, name, 1.0,1.0,1.0);

    RenderString(-50,200,GLUT_BITMAP_TIMES_ROMAN_24, "PONG",1.0,1.0,0.5);
    RenderString(-35,-185,GLUT_BITMAP_TIMES_ROMAN_24, "PLAY",1.0,1.0,0.5);
}
void lobby(){
    glBegin(GL_LINE_LOOP);/*Player box for team 1*/
        glVertex2i(-400,200);
        glVertex2i(-400,250);
        glVertex2i(-200,250);
        glVertex2i(-200,200);
    glEnd();
    glBegin(GL_LINE_LOOP);/*Player box for team 2*/
        glVertex2i(400,200);
        glVertex2i(400,250);
        glVertex2i(200,250);
        glVertex2i(200,200);
    glEnd();
    RenderString(-380,220,GLUT_BITMAP_TIMES_ROMAN_24, player_name, 1.0,1.0,1.0);
    RenderString(220,220,GLUT_BITMAP_TIMES_ROMAN_24, opponent, 1.0,1.0,1.0);
    
    RenderString(-400,260,GLUT_BITMAP_TIMES_ROMAN_24, "Team 1", 1.0,1.0,1.0);
    RenderString( 200,260,GLUT_BITMAP_TIMES_ROMAN_24, "Team 2", 1.0,1.0,1.0);
    RenderString(-70,300,GLUT_BITMAP_TIMES_ROMAN_24, "Game Lobby",1.0,1.0,0.5);

}

void game_accept(){
    RenderString(-200,220,GLUT_BITMAP_TIMES_ROMAN_24, "GAME READY!\nPress anywhere to accept.", 1.0,1.0,1.0);
}
void game_screen(char *packet){
    float player_x_1 = get_float_from_bytes(packet + 78);
    float player_y_1 = get_float_from_bytes(packet + 82);
    float player_width = get_float_from_bytes(packet + 86);
    float player_height = get_float_from_bytes(packet + 90);
    float player_goal_x_1 = get_float_from_bytes(packet + 21);
    float player_goal_x_2 = get_float_from_bytes(packet + 25);
    float player_goal_y_1 = get_float_from_bytes(packet + 29);
    float player_goal_y_2 = get_float_from_bytes(packet + 33);
    float enemy_x_1 = get_float_from_bytes(packet + 117);
    float enemy_y_1 = get_float_from_bytes(packet + 121);
    float enemy_width = get_float_from_bytes(packet + 125);
    float enemy_height = get_float_from_bytes(packet + 129);
    float enemy_goal_x_1 = get_float_from_bytes(packet + 38);
    float enemy_goal_x_2 = get_float_from_bytes(packet + 42);
    float enemy_goal_y_1 = get_float_from_bytes(packet + 46);
    float enemy_goal_y_2 = get_float_from_bytes(packet + 50);
    float ball_X = get_float_from_bytes(packet + 134);
    float ball_Y = get_float_from_bytes(packet + 138);
    float ball_radius = get_float_from_bytes(packet + 142);
    char ball_type_color = (packet[146]);
    char id;
    
    
    if(id == packet[94]){
        glColor3f(1.0f,0.0f,0.0f); 
    }else{
        glColor3f(0.0f,1.0f,0.0f); 
    }
    glBegin(GL_LINE_LOOP); /*Player box for team 2*/
        glVertex2i(player_x_1,player_y_1);
        glVertex2i(player_x_1 + player_width,player_y_1);
        glVertex2i(player_x_1 + player_width,player_y_1+ player_height);
        glVertex2i(player_x_1,player_y_1+ player_height);
    glEnd();

    if(id == packet[94]){
        glColor3f(0.0f,1.0f,0.0f); 
    }else{
        glColor3f(1.0f,0.0f,0.0f); 
    }
    glBegin(GL_LINE_LOOP); /*Enemy box for team 2*/
        glVertex2i(enemy_x_1,enemy_y_1);
        glVertex2i(enemy_x_1 + enemy_width,enemy_y_1);
        glVertex2i(enemy_x_1 + enemy_width,enemy_y_1+ enemy_height);
        glVertex2i(enemy_x_1,enemy_y_1+ enemy_height);
    glEnd();
    glColor3f(0.1, 1.0 ,1.0);
    glBegin(GL_LINE_LOOP); /*Enemy box for team 2*/
        glVertex2i(player_goal_x_1,player_goal_y_1);
        glVertex2i(player_goal_x_2,player_goal_y_1);
        glVertex2i(player_goal_x_2,player_goal_y_2);
        glVertex2i(player_goal_x_1,player_goal_y_2);
    glEnd();
    glBegin(GL_LINE_LOOP); /*Enemy box for team 2*/
        glVertex2i(enemy_goal_x_1,enemy_goal_y_1);
        glVertex2i(enemy_goal_x_2,enemy_goal_y_1);
        glVertex2i(enemy_goal_x_2,enemy_goal_y_2);
        glVertex2i(enemy_goal_x_1,enemy_goal_y_2);
    glEnd();
    circle(ball_X, ball_Y, ball_radius);
}

void myMouseInputs(int key, int pressed, int x, int y){
    printf("MOUSE %d %d, %d\n",key,x,y);
    if (state == 0){
        if(key == GLUT_LEFT_BUTTON){
            if(x >= STANDARD_WIDTH/2-100 &&  x <= STANDARD_WIDTH/2 + 100 && y <=STANDARD_HEIGHT/2 +200 && y >= STANDARD_HEIGHT/2 +150){
                if(strlen(player_name) != 0){
                    make_packet(1,'\0','\0',"");
                    state = 1;
                }
            }
        }
    }
    if (state == 2){// need adjusting
        if(key == GLUT_LEFT_BUTTON){
            if(x >= STANDARD_WIDTH/2-300 &&  x <= STANDARD_WIDTH/2 + 300 && y <=STANDARD_HEIGHT/2 +400 && y >= STANDARD_HEIGHT/2 -150){
                if(strlen(player_name) != 0){
                    make_packet(6,'\0','\0',"");
                    state = 3;
                }
            }
        }
    }
}

void make_packet(int id, char target_id, char source_id, char *message){
    int packet_size, i;
    struct data_array data[80];
    switch (id)
    {
    case 1:
        data->size = 1;
        data[0].type = is_string;
        data[0].val.stringVal = player_name;
        packet_size = createPacket(pp->outgoing, data,packetNr,(unsigned char) 1, 20);

        pp->outgoing_empty = 0;
        break;
    case 3:
        data->size = 3;
        data[0].type = is_char;
        data[0].val.charVal = target_id;
        data[1].type = is_char;
        data[1].val.charVal = source_id;
        data[2].type = is_string;
        data[2].val.stringVal = message;
        packet_size = createPacket(pp->outgoing, data,packetNr,(unsigned char) 3, 258);
        pp->outgoing_empty = 0;
        break;

    case 6:
        data->size = 1;
        data[0].type = is_char;
        data[0].val.charVal = player_ID;
        packet_size = createPacket(pp->outgoing, data,packetNr,(unsigned char) 6, 1);
        pp->outgoing_empty = 0;
        break;
    case 8:
        data->size = 1;
        data[0].type = is_char;
        data[0].val.charVal = source_id; // using the place for source id as the input char
        packet_size = createPacket(pp->outgoing, data,packetNr,(unsigned char) 8, 1);
        pp->outgoing_empty = 0;
        break;
    case 9:
        data->size = 0;
        packet_size = createPacket(pp->outgoing, data,packetNr,(unsigned char) 9, 0);
        pp->outgoing_empty = 0;
        break;
    default:
        break;
    }
    packetNr++;
}