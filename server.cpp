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
#include <math.h>
#include <sys/time.h>

#include "string"

#include "serverReceiveEntireMessage.cpp"

#define MAXLINE 1000
#define USE_NEW_METHOD 1

using namespace std;

//Method Signatures
char* getRobotID(char* msg);
uint32_t getRequestID(char* msg);
char* getRequestStr(char* msg);
char* generateHTTPRequest(char* robotAddress, char* robotID, char* requestStr, char* imageID, double &lengthOrDegrees);
char* getRobotPortForRequestStr(const char* requestStr);
void flushBuffersAndExit(int x);
string getNextCommand(string message, int &position);
double getTime();

//Main Method
int main(int argc, char *argv[])
{
	if(argc != 5) {
		quit("Usage: %s <server_port> <robot_IP/robot_hostname> <robot_ID> <image_id>", argv[0]);
	}
	//read args
	unsigned short localUDPPort = atoi(argv[1]);
	char* robotAddress = argv[2];
	char* robotID = argv[3];
	char* imageID = argv[4];
	
	plog("Read arguments");
	plog("Robot address: %s", robotAddress);
	plog("Robot ID: %s", robotID);
	plog("Image ID: %s", imageID);
	
	//listen for ctrl-c and call flushBuffersAndExit()
	signal(SIGINT, flushBuffersAndExit);

	//Create socket for talking to clients
	int clientSock;
	if((clientSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		quit("could not create client socket - socket() failed");
	}
	
	plog("Created client socket: %d", clientSock);
		
	//Construct local address structure for talking to clients
	struct sockaddr_in localAddress;
	memset(&localAddress, 0, sizeof(localAddress));		//Zero out structure
	localAddress.sin_family = AF_INET;					//Any Internet address family
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);	//Any incoming interface
	localAddress.sin_port = htons(localUDPPort);		//The port clients will sendto

	if(bind(clientSock, (struct sockaddr *) &localAddress, sizeof(localAddress)) < 0) {
		quit("could not bind to client socket - bind() failed");
	}
	
	plog("binded to client socket");

	for(;;) {
//		plog("Start loop to handle client request");
		
//		struct sockaddr_in clientAddress;

		// commented out to test pareMessage
		string received = receiveEntireMessage(clientSock);
//		string received = "\14MOVE 1.2 3.4\4STOP\11GET IMAGE\7GET GPS\10GET DGPS\12GET LASERS\14TURN 1.2 3.4\4STOP\10not real";
		received += '\0';
		printBytes(received.c_str(), received.length());		
		cout << endl << received << endl;

		char const *temp = received.c_str();		
	
		//get ID
		unsigned int ID = ntohl(*(uint32_t*)temp);
		
		//printBytes(temp, 8);

		//get number of messages
		temp += 4;
		unsigned int numMessages = ntohl(*(uint32_t*)temp);
		
		//get message index
		temp += 4;
		unsigned int mesIndex = ntohl(*(uint32_t*)temp);
	
		//get RobotID
		temp+=4;
		char *RobotIDstr = new char[988];
		strcpy(RobotIDstr, temp);
		int robotPort = atoi(RobotIDstr);	

		cout << "ID: " << ID << " numMessages: " << numMessages << " mesIndex: " << mesIndex << " RobID: " << robotPort << endl;

/*
		unsigned int IDnumber = ntohl((ID[0] << 24) | (ID[1] << 16) | (ID[2] << 8) | ID[3]);
		cout << "IDNUMBER: " << IDnumber << endl; 
	
		strncpy(ID, &received[4], 4);
		IDnumber = ntohl((ID[0] << 24) | (ID[1] << 16) | (ID[2] << 8) | ID[3]);
        cout << "IDNUMBER: " << IDnumber << endl;
*/	
/*		//Receive request from client
		struct sockaddr_in clientAddress;
unsigned int clientAddressLen = sizeof(clientAddress);	//in-out parameter
		char clientBuffer[MAXLINE+1];	//Buffer for incoming client requests
		memset(clientBuffer, 0, MAXLINE+1);
		
		int recvMsgSize;
		//Block until receive a guess from a client
		if((recvMsgSize = recvfrom(clientSock, clientBuffer, MAXLINE, 0,
				(struct sockaddr *) &clientAddress, &clientAddressLen)) < 0) {
			quit("could not receive client request - recvfrom() failed");
		}
		
		cout << "Received request of bytes equal to " << recvMsgSize << endl;

		//Interpret client request
*/

//		char* requestRobotID = getRobotID(clientBuffer);
//		if(strcmp(robotID, requestRobotID) != 0) {
//			fprintf(stderr, "invalid request - robot ID's don't match\n");
//			exit(1);
//		}

//		plog("Requested robot ID: %s", requestRobotID);


		//char* requestStr = getRequestStr(clientBuffer);
		//char* robotPort = getRobotPortForRequestStr(requestStr);
		
		
//		unsigned int numberOfMessages = (unsigned int) clientBuffer[4];
//		unsigned int messageIndex = (unsigned int) clientBuffer[8];

//		cout << "First command length " << clientBuffer[12] << endl;


	
//		cout << clientBuffer << endl;	
//		cout << "Request string: " << requestStr << endl;
//		plog("Calculated port: %s", robotPort);

//TODO: Add 12+length(robot_ID) maybe +1 more once header correct to clientBuffer here	
		int position = strlen(RobotIDstr)+13; 
		int commandIndex = 0;
		do {
			//get current command 
			string command = getNextCommand(received, position);
			string sub = command.substr(0, 4);
			

			cout << "command string: " << command << " first 4: " << sub << endl;
			char* robotPort = getRobotPortForRequestStr(command.c_str());
			
			cout << "robotPort: " << robotPort << endl;

			//Send HTTP request to robot
			int robotSock;
			if((robotSock = setupClientSocket(robotAddress, robotPort, SOCKET_TYPE_TCP)) < 0) {
				quit("could not connect to robot");
			}	
			
			plog("Set up robot socket: %d", robotSock);
			
			double lengthOrDegrees = 0; //needed to wait proper time 
			char* httpRequest = generateHTTPRequest(robotAddress, robotID, &command[0u], imageID, lengthOrDegrees);
			cout << endl << "Testing proper gen " << lengthOrDegrees << endl;


			cout << "Created http request: " << httpRequest;
			
			double timeSpent = getTime();
			if(write(robotSock, httpRequest, strlen(httpRequest)) != strlen(httpRequest)) {
				quit("could not send http request to robot - write() failed");
			}
			timeSpent = getTime() - timeSpent;
			plog("Sent http request to robot");
			
			free(httpRequest);
			
			plog("freed http request");
	
			//Waits for robot on certain commands
			double sleepTime; 
			int waitSeconds, waitUSeconds;
			if (sub == "MOVE") {
				if (lengthOrDegrees > timeSpent) {
					commandIndex = 0;
					sleepTime = lengthOrDegrees - timeSpent;
					waitSeconds = (int) sleepTime;
					sleepTime -= waitSeconds;
					waitUSeconds = (int) (sleepTime*1000000);
					sleep(waitSeconds);
					usleep(waitUSeconds);
				}
			}
			else if (sub == "TURN") {
				const double actualSpeed = .89*M_PI/4;
				if(lengthOrDegrees/actualSpeed > timeSpent) {
					sleepTime = lengthOrDegrees/actualSpeed - timeSpent;
        			waitSeconds = (int) sleepTime;
         			sleepTime -= waitSeconds;
         			waitUSeconds = (int) (sleepTime*1000000);
         			//Wait until robot turns to correct orientation.
         			sleep(waitSeconds);
         			usleep(waitUSeconds);
      			}	
			}

			//Read response from Robot
			int pos = 0;
			char* httpResponse = (char *) malloc(MAXLINE);
			int n;
			char recvLine[MAXLINE+1]; //holds one chunk of read data at a time
			while((n = read(robotSock, recvLine, MAXLINE)) > 0) {
				memcpy(httpResponse+pos, recvLine, n);
				pos += n;
				httpResponse = (char *) realloc(httpResponse, pos+MAXLINE);
			}
	
			plog("Received http response of %d bytes", pos);
			plog("http response: ");
			#ifdef DEBUG
			int j;
			for(j = 0; j < pos; j++)
				fprintf(stderr, "%c", httpResponse[j]);
			#endif
			
			//Parse Response from Robot
			char* httpBody = strstr(httpResponse, "\r\n\r\n")+4;
			int httpBodyLength = (httpResponse+pos)-httpBody;
			
			cout << "http body of bytes" << httpBodyLength;
			plog("http body: ");
			#ifdef DEBUG
			for(j = 0; j < httpBodyLength; j++)
				fprintf(stderr, "%c", httpBody[j]);
			#endif

			
	
			//Send response back to the UDP client
			sendResponse(clientSock, &clientAddress, sizeof(clientAddress), ID, httpBody, httpBodyLength, commandIndex);
			commandIndex++;		
		
			free(httpResponse);
		
			plog("End loop to handle client request"); 
		} while(received[position] != '\0');
	}
	//Loop for each client request
}

char* getRobotID(char* msg) {
	return msg+4;
}

uint32_t getRequestID(char* msg) {
	return ntohl(*((uint32_t*)msg));
}

char* getRequestStr(char* msg) {
	char* robotID = getRobotID(msg);

	return msg+5+strlen(robotID);
}



char* generateHTTPRequest(char* robotAddress, char* robotID, char* requestStr, char* imageID, double &lengthOrDegrees) {
	char* URI = (char *) malloc(MAXLINE);
	memset(URI, 0, MAXLINE);
	double x;
	printf("%s", requestStr);
	if(sscanf(requestStr, "MOVE %lf %lf", &x, &lengthOrDegrees) == 2) {
		sprintf(URI, "/twist?id=%s&lx=%lf", robotID, x);
	}
	else if(sscanf(requestStr, "TURN %lf %lf", &x, &lengthOrDegrees) == 2) {
		sprintf(URI, "/twist?id=%s&az=%lf", robotID, x);
	}
	else if(strstr(requestStr, "STOP") != NULL) {
		sprintf(URI, "/twist?id=%s&lx=0",robotID);
	}
	else if(strstr(requestStr, "GET IMAGE") != NULL) {
		sprintf(URI, "/snapshot?topic=/robot_%s/image&width=600&height=500", imageID);
	}
	else if(strstr(requestStr, "GET GPS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	}
	else if(strstr(requestStr, "GET DGPS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	}
	else if(strstr(requestStr, "GET LASERS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	}
	else {
		free(URI);
		return NULL;
	}

	char* httpRequest = (char *) malloc(MAXLINE);
	memset(httpRequest, 0, MAXLINE);
	strcat(httpRequest, "GET ");
	strcat(httpRequest, URI);
	strcat(httpRequest, " HTTP/1.1\r\n");
	
	//Host
	strcat(httpRequest, "Host: ");
	strcat(httpRequest, robotAddress);
	strcat(httpRequest, ":");
	strcat(httpRequest, getRobotPortForRequestStr((const char*)requestStr));
	strcat(httpRequest, "\r\n");
	
	//Connection: close
	strcat(httpRequest, "Connection: close\r\n");
	
	strcat(httpRequest, "\r\n");

	free(URI);
	printf("httpreq %s\n", httpRequest);
	fflush(stdout);
	return httpRequest;
}

//Gets TCP Port
char* getRobotPortForRequestStr(const char* requestStr) {
	if(strstr(requestStr, "MOVE") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "TURN") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "STOP") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "GET IMAGE") != NULL) {
		return "8081";
	} else if(strstr(requestStr, "GET GPS") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "GET DGPS") != NULL) {
		return "8084";
	} else if(strstr(requestStr, "GET LASERS") != NULL) {
		return "8083";
	} else {
		return NULL;
	}
}

/* This routine contains the data printing that must occur before the program 
*  quits after the CNTC signal. */
void flushBuffersAndExit(int x) {
	fflush(stdout);
	exit(0);
}

string getNextCommand(string message, int &position)
{
    int commandLength = (int) (unsigned char) message[position];

//  int end = ++position + commandLength;
	string ret = message.substr(++position, commandLength);
    position += commandLength;
/*
    ret.reserve(commandLength + 1);

    while(position != end)
    {
        ret += message[position++];
    }
    ret += '\0';
*/

    return ret;
}

double getTime() {
   struct timeval curTime;
   (void) gettimeofday(&curTime, (struct timezone *)NULL);
   return (((((double) curTime.tv_sec) * 10000000.0)
      + (double) curTime.tv_usec) / 10000000.0);
}
