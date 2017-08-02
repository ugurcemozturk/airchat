#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#include "send.h"
#define MY_DEST_MAC0    0x00
#define MY_DEST_MAC1    0x80
#define MY_DEST_MAC2    0xc9
#define MY_DEST_MAC3    0x00
#define MY_DEST_MAC4    0x07
#define MY_DEST_MAC5    0x02

#define DEFAULT_IF    "enx0080c9000702"
#define BUF_SIZ        1024

int send_hello(char *destMac) {
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];

	/* Get interface name */

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

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = destMac[0];
	eh->ether_dhost[1] = destMac[1];
	eh->ether_dhost[2] = destMac[2];
	eh->ether_dhost[3] = destMac[3];
	eh->ether_dhost[4] = destMac[4];
	eh->ether_dhost[5] = destMac[5];
	/* Ethertype field */
	//eh->ether_type = htons(ETH_P_IP);
	eh->ether_type = htons(0x1234);
	tx_len += sizeof(struct ether_header);

	/* Packet data */

	sendbuf[tx_len++] = 0x02;

	sendbuf[tx_len++] = 0x48;
	sendbuf[tx_len++] = 0x65;
	sendbuf[tx_len++] = 0x6c;
	sendbuf[tx_len++] = 0x6c;
	sendbuf[tx_len++] = 0x6f;
	/*
	 char message_array[6]= {0x48,0x65,0x6c,0x6c,0x6f};
	 int j;
	 for(j=0;j<sizeof(message_array);j++){
	 printf("%c",hello_arr[j]);
	 }
	 */
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

	/* Send packet */
	if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*) &socket_address,
			sizeof(struct sockaddr_ll)) < 0)
		printf("Send failed\n");

	return 0;
}

int send_file_ACK(char *destMac,char PID,char status) {
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];

	/* Get interface name */

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

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = destMac[0];
	eh->ether_dhost[1] = destMac[1];
	eh->ether_dhost[2] = destMac[2];
	eh->ether_dhost[3] = destMac[3];
	eh->ether_dhost[4] = destMac[4];
	eh->ether_dhost[5] = destMac[5];
	/* Ethertype field */
	//eh->ether_type = htons(ETH_P_IP);
	eh->ether_type = htons(0x1234);
	tx_len += sizeof(struct ether_header);

	/* Packet data */

	sendbuf[tx_len++] = 0x09;
	sendbuf[tx_len++] = PID;
	sendbuf[tx_len++] = status;


	/*
	 char message_array[6]= {0x48,0x65,0x6c,0x6c,0x6f};
	 int j;
	 for(j=0;j<sizeof(message_array);j++){
	 printf("%c",hello_arr[j]);
	 }
	 */
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

	/* Send packet */
	if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*) &socket_address,
			sizeof(struct sockaddr_ll)) < 0)
		printf("Send failed\n");

	return 0;
}
