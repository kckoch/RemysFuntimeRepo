#include "serverMessenger.h"
#include "utility.h"
#include "setupClientSocket.inc"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>     /* for memset() */
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>      /* for getHostByName() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* for time() */
#include <signal.h>
#include <iostream>

#include "string"

#define MAXLINE 1000
#define USE_NEW_METHOD 1

#define DEBUG(X) cout << "{" << __LINE__ << "} | " << X; cout.flush();

using namespace std;

//Method Signatures
char* getRobotID(char* msg);
uint32_t getRequestID(char* msg);
char* getRequestStr(char* msg);
char* generateHTTPRequest(char* robotAddress, char* robotID, char* requestStr, char* imageID);
char* getRobotPortForRequestStr(char* requestStr);
void flushBuffersAndExit(int x);

struct sockaddr_in clientAddress;

void printBytes(const char* string, int bytes) {
    int i;
    for (i = 0; i < bytes; i++) {
        if (!(i % 8)) printf("\n");
        printf("%.2X ", *(string + i) & 0xff);
    }
}


// Appends the specied string to message, including null bytes
void addToString(string &message, string &add)
{
	for(int i = 0; i < add.size(); i++)
	{
		message += add[i];
	}
}

// Receive messages from the client until one ends with the null byte, indicating that all commands have been sent
// Algorithm and supporting functions written by TJ Wills
string receiveEntireMessage(int &clientSock)
{
	string ret;

//	do
	{
		//Receive request from client
		unsigned int clientAddressLen = sizeof(clientAddress);	//in-out parameter
		char clientBuffer[MAXLINE+1];	//Buffer for incoming client requests
		memset(clientBuffer, 0, MAXLINE+1);
		
		int recvMsgSize;
		//Block until receive a guess from a client
		if((recvMsgSize = recvfrom(clientSock, clientBuffer, MAXLINE, 0,
				(struct sockaddr *) &clientAddress, &clientAddressLen)) < 0) {
			quit("could not receive client request - recvfrom() failed");
		}
		cout << endl << recvMsgSize << endl;
	
		printBytes(clientBuffer, recvMsgSize);

		cout << endl << endl << endl;

		ret.reserve(ret.size() + recvMsgSize);

		// Append received message into return string
		for(int i = 0; i < recvMsgSize; i++)
		{
			ret += clientBuffer[i];
		}

		printBytes(ret.c_str(), ret.length());

	} //while(ret[ret.size() - 1] != '\0');

	cout << endl << "I SAID HEYYYY RECIEVING HOLIDAYS" << endl;
	return ret;
}
















