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

// Appends the specied string to message, including null bytes
void addToString(string &message, string &add)
{
	for(int i = 0; i < add.size(); i++)
	{
		message += add[i];
	}
}

// Receive messages from the client until one ends with the null byte, indicating that all commands have been sent
string receiveEntireMessage(int &clientSock)
{
//	DEBUG("RECEIVE ENTIRE MESSAGE START\n");
	string ret;

	int x = 0;
	string test[2] = {"1", "2"};

	do
	{
		//Receive request from client
		struct sockaddr_in clientAddress;
		unsigned int clientAddressLen = sizeof(clientAddress);	//in-out parameter
		char clientBuffer[MAXLINE+1];	//Buffer for incoming client requests
		memset(clientBuffer, 0, MAXLINE+1);
		
		int recvMsgSize;
		//Block until receive a guess from a client
//		if((recvMsgSize = recvfrom(clientSock, clientBuffer, MAXLINE, 0,
//				(struct sockaddr *) &clientAddress, &clientAddressLen)) < 0) {
//			quit("could not receive client request - recvfrom() failed");
//		}

		memcpy(clientBuffer + strlen(clientBuffer), test[x].c_str(), strlen(test[x].c_str()) + x);  
		recvMsgSize = strlen(test[x].c_str()) + x;

		ret.reserve(ret.size() + recvMsgSize);

		// Append received message into return string
		for(int i = 0; i < recvMsgSize; i++)
		{
			ret += clientBuffer[i];
		}

//	DEBUG("ret so far = " << ret << "\n");
	x++;
	} while(ret[ret.size() - 1] != '\0');

/*
	DEBUG("RETURNING (" << ret.size() << ") {" << ret << "}\n");
	for(int i = 0; i < ret.size(); i++)
	{
		cout << "(" << i << ") " << (int) (unsigned char) ret[i] << " | " << ret[i] << endl;
	}
*/
	return ret;
}
















