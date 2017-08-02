/*
 * send.c
 *
 *  Created on: Jul 27, 2017
 *      Author: cem
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#include <unistd.h>

#define MY_DEST_MAC0    0xff
#define MY_DEST_MAC1    0xff
#define MY_DEST_MAC2    0xff
#define MY_DEST_MAC3    0xff
#define MY_DEST_MAC4    0xff
#define MY_DEST_MAC5    0xff

#define ETHER_TYPE    0x1234
#define DEFAULT_IF    "enx0080c9000702"
#define BUF_SIZ        1024

#define MAX_NAME_SIZE 10

static const char filename[] = "contacts.txt";
FILE *file;

int messageId = 0;
int fileRequest = 0;

static uint8_t unicastMac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static uint8_t myName[] = {0x75, 0x67, 0x75, 0x72, 0x63, 0x65, 0x6d, 0x00, 0x00, 0x00};
static uint8_t mySurname[] = { 0x6f, 0x7a, 0x74, 0x75, 0x72, 0x6b, 0x00, 0x00, 0x00, 0x00 };

static uint8_t requestedFileName[] = { 0x6c, 0x69, 0x6c, 0x61, 0x2e, 0x6d, 0x70,
		0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00 };

//static uint8_t myBroadCast[];

void selectUser(int counterUser);
//void sendUnicast(int userLine);
void createMac(char* line);
unsigned int parse_char(char c);

enum {
	QUERY_BROADCAST, QUERY_UNICAST, HELLO_RESPONSE, CHAT, CHAT_ACK, EXITING,
} EN_PACKET;

struct query_bcast {
	uint8_t type;
	char name[MAX_NAME_SIZE];
	char surname[MAX_NAME_SIZE];
}__attribute__((packed));

struct query_ucast {
	uint8_t type;
	char name[MAX_NAME_SIZE];
	char surname[MAX_NAME_SIZE];
	char target_name[MAX_NAME_SIZE];
	char target_surname[MAX_NAME_SIZE];
}__attribute__((packed));

struct helloResponse {
	char type[0];
	char responder_name[9];
	char responder_surname[9];
	char target_name[9];
	char target_surname[9];
} helloResponse;

struct chat {
	char type[0];
	char length[1];
	char packet_id[0];
	char message[160];
} chat;

struct chatAck {
	char type[0];
	char packet_id[1];
} chatAck;

struct exiting {
	char type[0];
	char name[9];
	char surname[9];
} exiting;

int main(int argc, char *argv[]) {
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];

	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
		perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
		perror("SIOCGIFHWADDR");

// name surname ascii olarak tanımla
// for içinde to hex ve her basamakta send bufa gönder
// receiverda macleri tut
// receiverda linked listte tutlan structları serialize ederek dosyaya kaydet
	/* Packet data */
	/*
	 sendbuf[tx_len++] = 0xde;
	 sendbuf[tx_len++] = 0xad;
	 sendbuf[tx_len++] = 0xbe;
	 sendbuf[tx_len++] = 0xef;

	 sendbuf[tx_len++] = 0x48;
	 sendbuf[tx_len++] = 0x65;
	 sendbuf[tx_len++] = 0x6c;
	 sendbuf[tx_len++] = 0x6c;
	 sendbuf[tx_len++] = 0x6f;
	 */

	int input, input2, i, k, counterUser;

	printf("1. Send BroadCast\n");
	printf("2. Send UniCast \n");
	printf("3. Exit\n");
	printf("8. Send file request\n");
	printf("Selection: ");

	scanf("%d", &input);
	switch (input) {
	case 1:

		/* Index of the network device */
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		/* Address length*/
		socket_address.sll_halen = ETH_ALEN;
		/* Destination MAC */
		socket_address.sll_addr[0] = MY_DEST_MAC0;
		socket_address.sll_addr[1] = MY_DEST_MAC1;
		socket_address.sll_addr[2] = MY_DEST_MAC2;
		socket_address.sll_addr[3] = MY_DEST_MAC3;
		socket_address.sll_addr[4] = MY_DEST_MAC4;
		socket_address.sll_addr[5] = MY_DEST_MAC5;

		/* Construct the Ethernet header */
		memset(sendbuf, 0, BUF_SIZ);
		/* Ethernet header */
		eh->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
		eh->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
		eh->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
		eh->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
		eh->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
		eh->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];
		eh->ether_dhost[0] = MY_DEST_MAC0;
		eh->ether_dhost[1] = MY_DEST_MAC1;
		eh->ether_dhost[2] = MY_DEST_MAC2;
		eh->ether_dhost[3] = MY_DEST_MAC3;
		eh->ether_dhost[4] = MY_DEST_MAC4;
		eh->ether_dhost[5] = MY_DEST_MAC5;
		/* Ethertype field */
		//eh->ether_type = htons(ETH_P_IP);
		eh->ether_type = htons(ETHER_TYPE);
		tx_len += sizeof(struct ether_header);

		sendbuf[tx_len++] = 0x00;
		for (i = 0; i < sizeof(myName); i++) {
			sendbuf[tx_len++] = myName[i];
		}
		for (k = 0; k < sizeof(mySurname); k++) {
			sendbuf[tx_len++] = mySurname[k];
		}
		// here add message id

		if (sendto(sockfd, sendbuf, tx_len, 0,
				(struct sockaddr*) &socket_address, sizeof(struct sockaddr_ll))
				< 0) {
			printf("Send failed\n");
		} else {
			printf("Send successful!\n");
		}

		break;
	case 2:
		counterUser = 0;
		fputs("Select person to chat: \n", stdout); /* write the line */
		file = fopen(filename, "r");
		if (file != NULL) {
			char line[128]; /* or other suitable maximum line size */
			while (fgets(line, sizeof line, file) != NULL) /* read a line */
			{
				counterUser++;
				printf("%d: ", counterUser);
				fputs(line, stdout); /* write the line */

			}
			fclose(file);
		} else {
			perror(filename); /* why didn't the file open? */
		}
		selectUser(counterUser); //hanig mac adresine yollayacagini secip, dest mac'e assign eder.

		/* Index of the network device */
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		/* Address length*/
		socket_address.sll_halen = ETH_ALEN;
		/* Destination MAC */
		socket_address.sll_addr[0] = unicastMac[0];
		socket_address.sll_addr[1] = unicastMac[1];
		socket_address.sll_addr[2] = unicastMac[2];
		socket_address.sll_addr[3] = unicastMac[3];
		socket_address.sll_addr[4] = unicastMac[4];
		socket_address.sll_addr[5] = unicastMac[5];

		/* Construct the Ethernet header */
		memset(sendbuf, 0, BUF_SIZ);
		/* Ethernet header */
		eh->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
		eh->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
		eh->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
		eh->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
		eh->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
		eh->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];
		eh->ether_dhost[0] = unicastMac[0];
		eh->ether_dhost[1] = unicastMac[1];
		eh->ether_dhost[2] = unicastMac[2];
		eh->ether_dhost[3] = unicastMac[3];
		eh->ether_dhost[4] = unicastMac[4];
		eh->ether_dhost[5] = unicastMac[5];
		/* Ethertype field */
		//eh->ether_type = htons(ETH_P_IP);
		eh->ether_type = htons(ETHER_TYPE);
		tx_len += sizeof(struct ether_header);

		sendbuf[tx_len++] = 0x01;
		for (i = 0; i < sizeof(myName); i++) {
			sendbuf[tx_len++] = myName[i];
		}
		for (k = 0; k < sizeof(mySurname); k++) {
			sendbuf[tx_len++] = mySurname[k];
		}
		// here add message id

		if (sendto(sockfd, sendbuf, tx_len, 0,
				(struct sockaddr*) &socket_address, sizeof(struct sockaddr_ll))
				< 0) {
			printf("Send failed\n");
		} else {
			printf("Send successful!\n");
		}

		break;
	case 3:

		break;
	case 4:
		printf("Thanks!\n");
		break;
	case 8:
		counterUser = 0;
		fputs("Select person to want file: \n", stdout); /* write the line */
		file = fopen(filename, "r");
		if (file != NULL) {
			char line[128]; /* or other suitable maximum line size */
			while (fgets(line, sizeof line, file) != NULL) /* read a line */
			{
				counterUser++;
				printf("%d: ", counterUser);
				fputs(line, stdout); /* write the line */

			}
			fclose(file);
		} else {
			perror(filename); /* why didn't the file open? */
		}
		selectUser(counterUser);

		/* Index of the network device */
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		/* Address length*/
		socket_address.sll_halen = ETH_ALEN;
		/* Destination MAC */
		socket_address.sll_addr[0] = unicastMac[0];
		socket_address.sll_addr[1] = unicastMac[1];
		socket_address.sll_addr[2] = unicastMac[2];
		socket_address.sll_addr[3] = unicastMac[3];
		socket_address.sll_addr[4] = unicastMac[4];
		socket_address.sll_addr[5] = unicastMac[5];

		/* Construct the Ethernet header */
		memset(sendbuf, 0, BUF_SIZ);
		/* Ethernet header */
		eh->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
		eh->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
		eh->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
		eh->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
		eh->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
		eh->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];
		eh->ether_dhost[0] = unicastMac[0];
		eh->ether_dhost[1] = unicastMac[1];
		eh->ether_dhost[2] = unicastMac[2];
		eh->ether_dhost[3] = unicastMac[3];
		eh->ether_dhost[4] = unicastMac[4];
		eh->ether_dhost[5] = unicastMac[5];
		/* Ethertype field */
		//eh->ether_type = htons(ETH_P_IP);
		eh->ether_type = htons(ETHER_TYPE);
		tx_len += sizeof(struct ether_header);

		sendbuf[tx_len++] = 0x08;
		for (i = 0; i < sizeof(requestedFileName); i++) {
			sendbuf[tx_len++] = requestedFileName[i];
		}
		sendbuf[tx_len++] = fileRequest;
		fileRequest++;

		// here add message id

		if (sendto(sockfd, sendbuf, tx_len, 0,
				(struct sockaddr*) &socket_address, sizeof(struct sockaddr_ll))
				< 0) {
			printf("Send failed\n");
		} else {
			printf("Send successful!\n");
		}

		break;
	default:
		printf("Bad input, quitting!\n");
		break;
	}
	getchar();

}

void selectUser(int counterUser) {
	int k, i;
	int count = 1;
	scanf("%d", &k);
	for (i = 1; i < counterUser+1; i++) {
		if (k == i) {
			file = fopen(filename, "r");
			if (file != NULL) {
				char line[256]; /* or other suitable maximum line size */
				while (fgets(line, sizeof(line), file) != NULL) /* read a line */
				{
					//printf("LineNumber: %d , Counter: %d",k,count);
					if (count == k) {
						printf("Selected Person: %s ", line);
						createMac(&line);
						//hex_print1(line);
						//setUser(lineNumber);

						count++;
					} else {
						count++;
					}
				}
				fclose(file);

			} else {
				//file doesn't exist
			}
		}
	}
}

/*
 void sendUnicast(int userLine){
 printf("heree2: %d",userLine);
 }
 */

void createMac(char* line) {
	int i, j = 0;

	for (i = 0; i < 6; i++) {
		unicastMac[i] = parse_char(line[j]) * 0x10 + parse_char(line[j + 1]);
		j = j + 2;
	}
	/*
	 for (i = 0; i < 6; i++) {
	 fprintf(stdout, "%02x ", ((uint8_t *) unicastMac)[i]);
	 }*/
}

unsigned int parse_char(char c) {
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('a' <= c && c <= 'f')
		return 10 + c - 'a';
	if ('A' <= c && c <= 'F')
		return 10 + c - 'A';

	abort();
}

