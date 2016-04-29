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

#include <string>
#include "server.cpp"
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

string getNextCommand(string &message, int &position)
{
	int commandLength = (int) (unsigned char) message[position];
//	DEBUG("Substring of size " << commandLength << endl);
//	int end = ++position + commandLength;

	string ret = message.substr(++position, commandLength);
	position += commandLength;


	return ret;
}

void getImage()
{
	DEBUG("getImage() START\n");
	
}

void getGPS(char* robotAddress, char* robotID, char* requestStr)
{
	DEBUG("getGPS() START\n");
	char* portForRequestStr = "8082"
	int robotSock;
	//TODO: Just put this at the end of the parse loop prob
	//Send HTTP request to robot
	if((robotSock = setupClientSocket(robotAddress, robotPort, SOCKET_TYPE_TCP)) < 0) {
            quit("could not connect to robot");
        } 
	char* httpRequest = generateHTTPRequest(robotAddress, robotID, requestStr, portForRequestStr);

	if(write(robotSock, httpRequest, strlen(httpRequest)) != strlen(httpRequest)) {
            quit("could not send http request to robot - write() failed");
        }

	free(httpRequest);
	//recieve response from robot
	int pos = 0;
        char* httpResponse = (char *) malloc(MAXLINE);
        int n;
        char recvLine[MAXLINE+1]; //holds one chunk of read data at a time
        while((n = read(robotSock, recvLine, MAXLINE)) > 0) {
            memcpy(httpResponse+pos, recvLine, n);
            pos += n;
            httpResponse = (char *) realloc(httpResponse, pos+MAXLINE);
        }

	//Parse Response from Robot
        char* httpBody = strstr(httpResponse, "\r\n\r\n")+4;
        int httpBodyLength = (httpResponse+pos)-httpBody;

	//Send response back to the UDP client
        uint32_t requestID = getRequestID(clientBuffer);
        sendResponse(clientSock, &clientAddress, clientAddressLen, requestID, httpBody, httpBodyLength);
        
		cout << "DAMMMMMNNNNNNN WORKED" << endl;       
 
        free(httpResponse);

}

void getDGPS()
{
	DEBUG("getDGPS() START\n");
	
}

void getLasers()
{
	DEBUG("getLasers() START\n");
	
}

void stop()
{
	DEBUG("stop() START\n");
	
}

void move(float &velocity, float &length)
{
	DEBUG("move(" << velocity << ", " << length << ") START\n");
	
}

void turn(float &velocity, float &degrees)
{
	DEBUG("turn(" << velocity << ", " << degrees << ") START\n");
	
}

// Perform each command from message and send an appropriate response to the client
void parseMessage(char* robotAddress, char* robotID, string &message, int &clientSock, struct sockaddr_in &localAddress)
{
	int position = 0;
	do
	{
		cout << position << " | " << (unsigned int) (char) message[position] << " | " << message[position];
		string command = getNextCommand(message, position);

		string sub = command.substr(0, 4);

		cout << "command = " << command << endl << sub << endl;

		// Handle our getSnapshot() requests
		if(sub == "GET ")
		{
			if(command == "GET IMAGE")
			{
				DEBUG("HANDLING GET IMAGE\n");
				getImage();

			}
			else if(command == "GET GPS")
			{
				DEBUG("HANDLING GET GPS\n");
				getGPS(robotAddress, robotID, sub);

			}
			else if(command == "GET DGPS")
			{
				DEBUG("HANDLING GET DGPS\n");
				getDGPS();

			}
			else if(command == "GET LASERS")
			{
				DEBUG("HANDLING GET LASER\n");
				getLasers();

			}
			else 
			{
				string q = "UNKNOWN COMMAND " + command;
				DEBUG(q << endl);
	//			quit(q.c_str());
			}
		}
		// Hanle move, turn, and stop commands
		else
		{
			if(command == "STOP")
			{
				DEBUG("HANDLING STOP\n");
				stop();
				
			}
			else if(sub == "MOVE")
			{
				DEBUG("HANDLING MOVE\n");

				float velocity, length;

				int endIndex = 5;
				while(command[endIndex++] != ' ');;

				sub = command.substr(5, endIndex - 5);
				velocity = atof(sub.c_str());

				sub = command.substr(endIndex);
				length = atof(sub.c_str());

				move(velocity, length);

//				DEBUG("Velocity = " << velocity << ", length = " << length << endl);

			}
			else if(sub == "TURN")
			{
				DEBUG("HANDLING TURN\n");

				float velocity, degrees;

				int endIndex = 5;
				while(command[endIndex++] != ' ');;

				sub = command.substr(5, endIndex - 5);
				velocity = atof(sub.c_str());

				sub = command.substr(endIndex);
				degrees = atof(sub.c_str());

				turn(velocity, degrees);

				
			}
			else
			{
				string q = "UNKNOWN COMMAND " + command;
				DEBUG(q << endl);
	//			quit(q.c_str());
			}
		}

	} while(message[position] != '\0');

}






























