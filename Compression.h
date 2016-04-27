/*
 * Compression.h
 *
 *  Created on: Apr 21, 2016
 *      Author: TJ
 */

#ifndef COMPRESSION_H_
#define COMPRESSION_H_

#include <map>

using namespace std;

string encodeMessage(const string &message, map<string, int> dictionary);
string decodeMessage(string message);

// C implementations - count = length of *message
char *encodeMessage(const char *message, int count, int *messageSize);
char *decodeMessage(const char *message, int count, int *messageSize);



#endif /* COMPRESSION_H_ */
