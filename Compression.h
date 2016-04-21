/*
 * Compression.h
 *
 *  Created on: Apr 21, 2016
 *      Author: tj
 */

#ifndef COMPRESSION_H_
#define COMPRESSION_H_

using namespace std;

// C implementations - count = length of *message
char *encodeMessage(const char *message, int count, int *messageSize);
char *decodeMessage(const char *message, int count, int *messageSize);



#endif /* COMPRESSION_H_ */
