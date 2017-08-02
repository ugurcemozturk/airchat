/*
 * rec.c
 *
 *  Created on: Jul 27, 2017
 *      Author: cem
 */

/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

/*
 * Following codes may have some similarities with Efe Baloglu due to brainstorms that we did
 * together.
 * */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

#include "packets.h"
#include "send.h"
#include "tx_file.h"

struct contact {
	char mac[11];
	char name[9];
	char surname[9];
	struct contact *next;
}*myContactList = NULL, *first = NULL, *last = NULL;

FILE *f;

char *savedMessage;

static uint8_t bcast[22];

uint8_t transmitMac[5];
uint8_t id[1];

int count = 0;
int idCounter = 0;

unsigned int parse_char(char c);

#define DEST_MAC0    0xff
#define DEST_MAC1    0xff
#define DEST_MAC2    0xff
#define DEST_MAC3    0xff
#define DEST_MAC4    0xff
#define DEST_MAC5    0xff

#define ETHER_TYPE    0x1234

#define DEFAULT_IF    "enx0080c9000702"
#define BUF_SIZ        1024

uint8_t *getPacket(ssize_t numbytes, uint8_t *buf) {
	uint8_t *packet = malloc(sizeof(numbytes - ETH_HLEN));
	memset(packet, 0, numbytes - ETH_HLEN);
	memcpy(packet, &buf[ETH_HLEN], numbytes - ETH_HLEN);

	return packet;
}
char *getTargetMAC(ssize_t numbytes, uint8_t *buf) {
	char *targetAddr = malloc(ETH_ALEN);
	memset(targetAddr, 0, ETH_ALEN);
	memcpy(targetAddr, &buf[ETH_ALEN], ETH_ALEN);
	return targetAddr;
}

/*
 * BURAYI MUTLAKA MAKROLARLA DUZELT, IGRENC.
 * */
char *getFileName(uint8_t *buf) {
	char *fileName = malloc(sizeof(char) * 31);
	memset(fileName, 0, 31);
	memcpy(fileName, &buf[15], 31);
	return fileName;
}
char getPID(uint8_t *buf) {
	char pid;
	pid = buf[47];
	return pid;
}

char getStatus(uint8_t *buf) {
	char status;
	status = buf[16];
	return status;
}
char *getIncomingMsg(uint8_t *buf,ssize_t numbytes){
	uint8_t len=numbytes-MSG_START;
	char *msg=malloc(sizeof(char)*len);
	memset(msg,0,len);
	memcpy(msg,&buf[MSG_START],len);

}
int checkMac(struct ether_header *eh, uint8_t *mac) {
	/* Check the packet is for me */
	if (eh->ether_dhost[0] == mac[0] && eh->ether_dhost[1] == mac[1]
			&& eh->ether_dhost[2] == mac[2] && eh->ether_dhost[3] == mac[3]
			&& eh->ether_dhost[4] == mac[4] && eh->ether_dhost[5] == mac[5]) {
		printf("Correct destination MAC address\n");
		return 1;
	} else {
		printf("Wrong destination MAC: %x:%x:%x:%x:%x:%x\n", eh->ether_dhost[0],
				eh->ether_dhost[1], eh->ether_dhost[2], eh->ether_dhost[3],
				eh->ether_dhost[4], eh->ether_dhost[5]);
		return 0;
	}

}

int main(int argc, char *argv[]) {
	int sockfd, ret, i;
	int sockopt;
	ssize_t numbytes;
	struct ifreq ifopts; /* set promiscuous mode */
	struct ifreq if_ip; /* get ip addr */
	uint8_t buf[BUF_SIZ];
	char ifName[IFNAMSIZ];

	uint8_t *packet;
	char *targetMac;

	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Header structures */
	struct ether_header *eh = (struct ether_header *) buf;

	memset(&if_ip, 0, sizeof(struct ifreq));

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
		perror("listener: socket");
		return -1;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ - 1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt)
			== -1) {
		perror("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ - 1)
			== -1) {
		perror("SO_BINDTODEVICE");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	repeat: printf("listener: Waiting to recvfrom...\n");
	numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
	printf("listener: got packet %lu bytes\n", numbytes);

	char messageType;
	printf("\tData:\n");

	messageType = (char) buf[14];

	switch (messageType) {
	case QUERY_BROADCAST:
		/* Check the packet is for me */
		if (eh->ether_dhost[0] == DEST_MAC0 && eh->ether_dhost[1] == DEST_MAC1
				&& eh->ether_dhost[2] == DEST_MAC2
				&& eh->ether_dhost[3] == DEST_MAC3
				&& eh->ether_dhost[4] == DEST_MAC4
				&& eh->ether_dhost[5] == DEST_MAC5) {
			printf("Correct destination MAC address\n");
		} else {
			printf("Wrong destination MAC: %x:%x:%x:%x:%x:%x\n",
					eh->ether_dhost[0], eh->ether_dhost[1], eh->ether_dhost[2],
					eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5]);
			ret = -1;
			goto done;
		}

		f = fopen("contacts.txt", "a+");

		myContactList = (struct contact*) malloc(sizeof(struct contact));
		write2file(f, buf);
		fclose(f);

		if (first == NULL) {
			myContactList->next = NULL;
			first = myContactList;
			last = myContactList;
			printf("Linked list Created!\n");
		} else {
			myContactList->next = NULL;
			last->next = myContactList;
			last = myContactList;
			printf("Data Inserted in the Linked list!\n");
		}

		break;
	case QUERY_UNICAST:
		targetMac = getTargetMAC(numbytes, buf);
		send_hello(targetMac);
		break;
	case HELLO_RESPONSE:

		printf("Data Type: Discovery Unicast!\n");
		targetMac = getTargetMAC(numbytes, buf);
		send_hello(targetMac);
		break;
	case CHAT:
		printf("New chat message: !\n");
		break;
	case CHAT_ACK:
		printf("44!\n");
		break;
	case CHAT_EXIT:
		printf("44!\n");
		break;
	case FILE_BROADCAST:
		printf("44!\n");
		break;
	case FILE_MD5_ACK:
		printf("44!\n");
		break;
	case FILE_QUERY_UNICAST:
		printf("88!\n");
		char pid = getPID(buf);
		char *fname = getFileName(buf);
		if (access(fname, F_OK) != -1) {
			puts("file exits!!!!\n");
			send_file_ACK(targetMac, pid, '1');
			sendFile("enx0080c9000702", "lila.mp3", "1000");

		} else {
			puts("not file exits!!!!\n");
			send_file_ACK(targetMac, pid, '0');
		}
		break;
	case 0x09:
		printf("99!\n");
		char status = getStatus(buf);
		printf("Status: %c\n", status);
		break;

	default:
		printf("Bad input, quitting!\n");
		break;
	}

	//print_All();
	//decode_bcast();

	//for (i=0; i<sizeof(buf2); i++) printf("%c", buf2[i]);
	printf("\n");

	done: goto repeat;

	close(sockfd);
	return ret;
}

static void decode_bcast() {
	struct query_bcast *q;
	q = (struct query_bcast*) bcast;

	fprintf(stdout, "* decoding broadcast query *\n");
	fprintf(stdout, "q->type: %d\n", q->type);

	fprintf(stdout, "q->name: %s\n", q->name);
	fprintf(stdout, "q->surname: %s\n", q->surname);
}

void write2file(FILE *f, uint8_t buf[60]) {
	int k, control = 0;
	char line[256];

	for (k = 6; k < 12; k++) {

		while (fgets(line, sizeof(line), f) != NULL) /* read a line */
		{

			if (buf[6] == (parse_char(line[0]) * 0x10 + parse_char(line[1]))
					&& buf[7]
							== (parse_char(line[2]) * 0x10 + parse_char(line[3]))
					&& buf[8]
							== (parse_char(line[4]) * 0x10 + parse_char(line[5]))
					&& buf[9]
							== (parse_char(line[6]) * 0x10 + parse_char(line[7]))
					&& buf[10]
							== (parse_char(line[8]) * 0x10 + parse_char(line[9]))
					&& buf[11]
							== (parse_char(line[10]) * 0x10
									+ parse_char(line[11]))) {
				printf("Received mac!");
				control = 1;
			}

		}
	}

	if (control == 0) {
		for (k = 6; k < 12; k++) {

			fprintf(f, "%02X", buf[k]);
		}
		fprintf(f, "%s", " ");

		for (k = 15; k < 25; k++) {
			if (buf[k] == 0x00) {
				k++;
				continue;
			}
			fprintf(f, "%c", buf[k]);
		}

		fprintf(f, " %s", " ");

		for (k = 25; k < 35; k++) {
			if (buf[k] == 0x00) {
				k++;
				continue;
			}
			fprintf(f, "%c", buf[k]);
		}

		fprintf(f, " %s", " \n");
	}

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
