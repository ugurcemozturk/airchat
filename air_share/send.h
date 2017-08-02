/*
 * send.h
 *
 *  Created on: Jul 27, 2017
 *      Author: cem
 */

#ifndef SRC_SEND_H_
#define SRC_SEND_H_

int send_hello(char *destMac);
int send_file_ACK(char *destMac,char PID,char status);
#endif /* SRC_SEND_H_ */
