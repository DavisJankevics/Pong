#include "headers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int str_length(char* mystring){
    int result = 0 ;
    int posititon = 0;
    while (mystring[posititon] != '\0')
    {
        posititon ++;
        result ++;
    }
    return result;
}
int str_copy(char* source, char* destination){
    int position = 0;
    while( source[position] != '\0' ){
        destination[position] = source[position];
        position++;
    }
    destination[position] = '\0';
    return 0;
}

int str_find(char* needle, char* haystack){
    int result = 0;
    int i = 0;
    int needleLength = str_length(needle);
    int haystackLength = str_length(haystack);
    while(haystack[i] != '\0'){
        int result = 0;
        if( haystack[i] == needle[0] && i+needleLength <= haystackLength){
            int n;
            for( n = 0 ; n < needleLength ; n++){
                if(haystack[n + i] == needle[n]) result++;
            }
        }
        if(result == needleLength) return i;
        i++;
    }
    return -1;
}

int get_port(int argc, char** argv, char* dest){
    int i;
    for(i = 1; i < argc ; i++){
        char *param = argv[i];
        
        if(str_find("-p=", param) == 0){
            param += 3;
            str_copy(param,dest);
            return atoi(dest);
        }
    }
    return -1;
}
int get_ip(int argc, char** argv, char* dest){
    int i;
    for(i = 1; i < argc ; i++){
        char *param = argv[i];
        char* retVal;
        if(str_find("-a=", param) == 0){
            param += 3;
            str_copy(param,dest);
            return 0;
        }
    }
    return -1;
}
char printable_char(char c){
    if(isprint(c)) return c;
    return '#';
}

void print_bytes(void* packet, int count){
    int i;
    unsigned char* p = (unsigned char*) packet;
    if(count>999){
        printf("Cannot print more than 999 bytes! You asked for %d\n",count);
        return;
    }
    printf("Printing %d bytes...\n",count);
    printf("[NPK] [C] [HEX] [DEC] [ BINARY ]\n");
    printf("================================\n");
    for(i=0;i<count;i++){
        printf(" %3d | %c | %02X | %3d | %c%c%c%c%c%c%c%c\n",i,printable_char(p[i]),p[i],p[i],
        p[i] & 0x80 ? '1':'0',
        p[i] & 0x40 ? '1':'0',
        p[i] & 0x20 ? '1':'0',
        p[i] & 0x10 ? '1':'0',
        p[i] & 0x08 ? '1':'0',
        p[i] & 0x04 ? '1':'0',
        p[i] & 0x02 ? '1':'0',
        p[i] & 0x01 ? '1':'0'
        );
    }
}

void universal_store_int_as_bytes_big_endian(void* packet, int data){
    int i;
    int s = sizeof(int);
    char *p = packet;
    for( i = 0 ; i < s; i++){
        p[i] = (data >> (s-i-1)*8) & 0xFF;
    }
}
void universal_store_float_as_bytes_big_endian(void* packet, float data){
    int i;
    int s = sizeof(float);
    char *p = packet;
    memcpy(packet, (unsigned char*) (&data), 4);
}
void universal_store_char_as_bytes(void* packet, unsigned char data){
    char *p = packet;
    p[0] = data;
}                                                                   
void universal_store_string_as_bytes(void* packet, unsigned char* data, int len){
    int i;
    int s = strlen(data);
    unsigned char *p = packet;
    for( i = 0 ; i < len; i++){
        if(i < s)
            p[i] = data[i];
        else
            p[i] = '\0';
    }
}

char checkSum(void* packet, int len){
    char* p = packet;
    char c;
    int i;
    for(i = 2; i < len ; i++){
        c ^= (p[i]);
    }
    return c;
}
int createPacket(void* packet, struct data_array *data, unsigned int PacketNumber, unsigned char PacketID, int dataLength){
    int size;
    size = 0;
    char *p = packet;
    universal_store_string_as_bytes(p,"--", 2);
    p += 2* sizeof(char);
    universal_store_int_as_bytes_big_endian(p,PacketNumber);
    p += sizeof(int);
    universal_store_char_as_bytes(p,PacketID);
    p += sizeof(char);
    universal_store_int_as_bytes_big_endian(p,dataLength);
    p += sizeof(int);
    size = 11;
    struct data_array *d;
    d = data;
    char checkSumChar, c;/*type, target, source, id, player_count;*/
    char *string;
    int counter, i, len, number;
    len = 0;
    float f_number;
    for( i = 0 ; i < d->size ; i ++ ){
        switch (d[i].type)
        {
        case is_string:
            string = d[i].val.stringVal;
            if(PacketID == 1 || PacketID == 4 || PacketID == 5 || PacketID == 9)
                len = 20;
            else if (PacketID == 3)
                len = 256;
            universal_store_string_as_bytes(p,string, len);
            p += len;
            size += len;
            break;
        case is_int:
            number = d[i].val.intVal;
            universal_store_int_as_bytes_big_endian(p,number);
            p += sizeof(int);
            size += sizeof(int);
            break;
        case is_char:
            c = d[i].val.charVal;
            universal_store_char_as_bytes(p,c);
            p += sizeof(char);
            size += sizeof(char);
            break;
        case is_float:
            f_number = d[i].val.floatVal;
            universal_store_float_as_bytes_big_endian(p,f_number);
            p += sizeof(float);
            size += sizeof(float);
            break;    
        default:
            break;
        }
    }
    checkSumChar = checkSum(packet, size);
    universal_store_char_as_bytes(p,checkSumChar);
    p += sizeof(char);
    size += 1;
    universal_store_string_as_bytes(p,"--",2);
    p += 2* sizeof(char);
    size += 2;
    return size;
}
int readPacket(int socet, char* buf){
    int read_bytes, total_read_bytes, reading;
    reading = 1;
    total_read_bytes = 0;
    char content = 0;
    char c, c1, c2;

    while(reading){

        if(total_read_bytes < 512){
            read_bytes = read(socet, &c, 1);
            if(read_bytes != -1){
                if(c =='-' && content){
                    read_bytes = read(socet, &c1, 1);
                    if(c1 == '-'){
                        reading = 0;
                        buf[total_read_bytes] = c;
                        total_read_bytes += read_bytes;

                        buf[total_read_bytes] = c1;
                        total_read_bytes += read_bytes;

                        return total_read_bytes;
                    }
                }else if (!content){
                    read_bytes = read(socet, &c1, 1);
                    if(read_bytes == -1 || c1 != '-'){
                        return -1;
                    }else{
                        content = 1;
                        buf[total_read_bytes] = c;
                        total_read_bytes += read_bytes;
                        buf[total_read_bytes] = c1;
                        total_read_bytes += read_bytes;
                        continue;
                    }
                }
                /*un-escaping*/
                if(content){
                    if(c == '-' || c == '?'){
                        read_bytes = read(socet, &c1, 1);
                        if(c1 != '*'){
                            printf("UNESCAPED CHAR, The packet may be invalid\n");
                        }
                    }
                    buf[total_read_bytes] = c;
                    total_read_bytes += read_bytes;
                }
            }
        }
    }
    return total_read_bytes;
}

int check_and_escape(char *packet, int len){
    int i, dif;
    dif = 0;
    char rewrittenPacket[2*len];
    for(i = 0 ; i < len ; i++){
        if(packet[i] == '-' && i < len - 2 && i >=2){
            rewrittenPacket[i+dif] = '?';
            dif++;
            rewrittenPacket[i+dif] = '-';
        }else if (packet[i] == '?'){
            rewrittenPacket[i+dif] = packet[i];
            dif++;
            rewrittenPacket[i+dif] = '*';
        }else{
            rewrittenPacket[i+dif] = packet[i];
        }
    }
    rewrittenPacket[len+dif] = '\0';
    memcpy(packet,rewrittenPacket, len+dif);
    return len+dif;
}
char check_packet(char* packet, int last_nr){
    char is_good;
    is_good = 0;
    int data_length;
    char expected_check_sum, real_check_sum;
    if(packet[6] != 0){
    if(last_nr <= get_int_from_bytes(packet+2)){
        data_length = get_int_from_bytes(packet+7);
        real_check_sum = checkSum(packet+2, data_length + 9);
        expected_check_sum = packet[11+data_length];
        if( real_check_sum == expected_check_sum ){
            is_good = 1;
        }
    }}
    return is_good;

}

int get_int_from_bytes(char* packet){
    int i, n;
    i = 0;
    for( n = 0 ; n < 4 ; n ++ ){
        i += ((unsigned char) packet[n] << (8*(4-n-1)));
    }
    return i;
}
float get_float_from_bytes(char* packet){
    int i, n;
    i = 0;
    float x = 0.0;
    char Buffer[4];
    for(i = 0 ; i < 4 ; i ++){
        Buffer[i] = packet[i];        
    }
    x = *(float *)&Buffer;    
    return x;
}