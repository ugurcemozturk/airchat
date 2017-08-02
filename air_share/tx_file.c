/****************************************************************************
 * C01: FILE DESCRIPTION
 ****************************************************************************
 * Copyright AirTies Wireless Networks
 * Created		: Jul 24, 2017
 * Authors		:
 * - K. Serdar AY
 * -
 * Description	: 
 ****************************************************************************
 * C02: INCLUDE FILES
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>

#include "tx_file.h"

#include "myether.h"
#include "packets.h"
/*****************************************************************************
 * C03:	  PRIVATE CONSTANT DEFINITIONS
 *****************************************************************************/
#define MAX_IFNAMSIZ 32
#define ETHER_TYPE   ((u_int16_t) 0x1234)
#define MY_NAME "ugurcem"
#define MY_SURNAME "ozturk"
#define MAX_FRAGMENT_SIZE 1500
/*****************************************************************************
 * C04:   PRIVATE DATA TYPES
 *****************************************************************************/

/*****************************************************************************
 * C05:   PRIVATE DATA DECLARATIONS
 *****************************************************************************/
static int *index_shuffle_arr;
/*****************************************************************************
 * C06:   PRIVATE (LOCAL) FUNCTION PROTOTYPES
 *****************************************************************************/
static void usage();
static void shuffle(void *array, size_t n, size_t size);
/*****************************************************************************
 * C07:   GLOBAL DATA DECLARATIONS
 *****************************************************************************/

/*****************************************************************************
 * C08:   GLOBAL FUNCTIONS
 *****************************************************************************/

int sendFile(char *a,char *b,char *c){
	int sfd;
	char ifname[MAX_IFNAMSIZ] = {0};
	FILE *fp = NULL;
	char* file_name;
	char* str_fragment_size;
	uint32_t file_size;
	uint32_t fragment_size = 0;
	uint16_t fragment_count = -1;
	char *fragment_buffer = NULL;
	struct file_bcast *header;
	int ret;
	struct timespec sleep_time;
	int i;



	file_name = b;
	str_fragment_size = c;

	fp = fopen((const char*)file_name, "r");
	if(!fp){
		fprintf(stderr, "ERROR: cannot open file %s, errno: %d\n", file_name, errno);
		goto bail;
	}

	/*get file length*/
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	if(file_size <= 0){
		fprintf(stderr, "ERROR: file length of %s s invalid!!!, errno: %d\n", file_name, errno);
		goto bail;
	}

	fragment_size = (uint32_t)atoi(str_fragment_size);
	if(fragment_size <= 0){
		fprintf(stderr, "ERROR: fragment size is invalid!!!, errno: %d\n", errno);
		goto bail;
	}
	if(fragment_size > file_size){
		fprintf(stderr, "ERROR: fragment size is bigger than file_size!!!, errno: %d\n", errno);
		goto bail;
	}
	if(file_size == fragment_size)
		fragment_count = 1;
	else
		fragment_count = file_size / fragment_size + 1;

	snprintf(ifname, MAX_IFNAMSIZ, "%s", a);

	if (!net_device_up(ifname)) {
		fprintf(stderr, "%s is not up\n", ifname);
		goto bail;
	}

	sfd = net_create_raw_socket(ifname, ETHER_TYPE);
	if (sfd == -1) {
		fprintf(stderr, "failed to init socket\n");
		goto bail;
	}

	fragment_buffer = malloc(fragment_size + sizeof(*header));
	if(!fragment_buffer){
		fprintf(stderr, "memory allocation error!\n");
		goto bail;
	}
	header = (struct file_bcast*)fragment_buffer;
	/*fill fix fields*/
	/* fill ethernet headers */
	net_get_iface_mac(ifname, (char *)header->eth.ether_shost);
	header->eth.ether_type = htons(ETHER_TYPE);
	memcpy(header->eth.ether_dhost, ETH_ADD_BCAST, ETH_ALEN);
	/* fill file proto fields */
    header->type = 0x06;
    snprintf(header->name, sizeof(header->name)-1, "%s", MY_NAME);
    snprintf(header->surname, sizeof(header->surname)-1, "%s", MY_SURNAME);
    snprintf(header->file_name, sizeof(header->file_name)-1, "%s", basename(file_name));
    header->file_size = file_size;
    header->file_fragment_count = fragment_count;

    index_shuffle_arr = malloc(header->file_fragment_count * sizeof(int));
	if(!index_shuffle_arr){
		fprintf(stderr, "index_shuffle_arr memory allocation error!\n");
		goto bail;
	}

    for(i = 0; i < header->file_fragment_count; i++)
    	index_shuffle_arr[i] = i;
    shuffle(index_shuffle_arr, header->file_fragment_count, sizeof(int));

	while(1){
		int i = 0;

		fprintf(stderr, "file name: %s\n"
						"file_size: %u\n"
						"fragment_size: %u\n"
						"fragment_count: %u\n"
						"STARTING...\n",
						basename(header->file_name), header->file_size, fragment_size, header->file_fragment_count);

		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time, NULL);

		while(i < fragment_count){
			int fr_size;
			int ndx = index_shuffle_arr[i];

			if(ndx == (fragment_count - 1)){
				fr_size = file_size - (fragment_count-1)*fragment_size;
			}
			else
				fr_size = fragment_size;

			ret = fseek(fp, ndx*fragment_size, SEEK_SET);
			if(ret){
				fprintf(stderr, "ERROR: fseek failed ret: %d, errno: %d\n", ret, errno);
				goto bail;
			}
			ret = fread(((struct file_bcast*)fragment_buffer)->fragment_data, fr_size, 1, fp);
			if(ret < 1){
				fprintf(stderr, "ERROR: fread failed ret: %d, errno: %d\n", ret, errno);
				goto bail;
			}

		    header->fragment_index = ndx+1;
		    header->fragment_size = fr_size;

		    fprintf(stderr, "sending %d/%d len:%d\n",
		    		header->fragment_index, header->file_fragment_count, header->fragment_size);
		    ret = send(sfd, (char*)header, fr_size + sizeof(*header), 0);
		    if(ret != (fr_size + sizeof(*header))){
		    	fprintf(stderr, "ERROR: send failed ret: %d, errno: %d\n", ret, errno);
				goto bail;
		    }
			i++;
			sleep_time.tv_sec = 0;
			sleep_time.tv_nsec = 10000000;
			nanosleep(&sleep_time, NULL);
		}
	}

	return 0;

bail:
	return -1;
}
/*****************************************************************************
 * C09:   PRIVATE FUNCTIONS
 *****************************************************************************/
static void usage(){
	fprintf(stderr, "\nUsage:\n./tx_file <ifname> <file> <fragment_size>\n");
	fprintf(stderr, "Example:\n./tx_file eth0 /home/ksay/my_file.txt 1450\n");
}

/* arrange the N elements of ARRAY in random order.
 * Only effective if N is much smaller than RAND_MAX;
 * if this may not be the case, use a better random
 * number generator. */
static void shuffle(void *array, size_t n, size_t size){
    char tmp[size];
    char *arr = array;
    size_t stride = size * sizeof(char);

    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; ++i) {
            size_t rnd = (size_t) rand();
            size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

            memcpy(tmp, arr + j * stride, size);
            memcpy(arr + j * stride, arr + i * stride, size);
            memcpy(arr + i * stride, tmp, size);
        }
    }
}
/*****************************************************************************
 * C10:   END OF CODE
 *****************************************************************************/
