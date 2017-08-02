#ifndef _PACKETS_H
#define _PACKETS_H

#define MAX_NAME_SIZE 10
#define MAX_FILE_NAME_SIZE 32

#include <netinet/if_ether.h>

enum {
   QUERY_BROADCAST,
   QUERY_UNICAST,
   HELLO_RESPONSE,
   CHAT,
   CHAT_ACK,
   CHAT_EXIT,
   FILE_BROADCAST,
   FILE_MD5_ACK,
   FILE_QUERY_UNICAST,
   FILE_STATUS,
} EN_PACKET;

struct query_bcast {
    struct ether_header eth;
    uint8_t type;
    char name[MAX_NAME_SIZE];
    char surname[MAX_NAME_SIZE];
} __attribute__((packed));

struct query_ucast {
    struct ether_header eth;
    uint8_t type;
    char name[MAX_NAME_SIZE];
    char surname[MAX_NAME_SIZE];
    char target_name[MAX_NAME_SIZE];
    char target_surname[MAX_NAME_SIZE]; 
}  __attribute__((packed));

struct file_bcast {
    struct ether_header eth;
    uint8_t type;
    char name[MAX_NAME_SIZE];
    char surname[MAX_NAME_SIZE];
    char file_name[MAX_FILE_NAME_SIZE];
    uint32_t file_size;
    uint16_t file_fragment_count;
    uint16_t fragment_index;
    uint32_t fragment_size;
    char fragment_data[0];
}  __attribute__((packed));

#endif
