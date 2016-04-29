#include "clientMessenger.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include "string"
#include <fstream>

#include "clientParseResponseSequence.cpp"

using namespace std;

int L; //L >= 1
int N; //4 <= N <= 8
int fileCount = 0;

const double COMMAND_TIMEOUT = 0.95;
const double DATA_TIMEOUT = 5.0;

// Returns length of string added
void addToMessage(string &message, const char *add)
{
	int addLen = strlen(add);
	message += addLen;
	for(int i = 0; i < addLen; i++)
	{
		message += add[i];
	}
	return;
}

void writeGetSnapshot(string &buffer)
{
	addToMessage(buffer, "GET IMAGE");
	addToMessage(buffer, "GET GPS");
	addToMessage(buffer, "GET DGPS");
	addToMessage(buffer, "GET LASERS");
}

// Generate and send the desired command sequence
// Algorithm and supporting functions written by TJ Wills
void sendCommandSequence(int numSides, int clockwise, int length)
{
	double turnAngle;
	if(clockwise) turnAngle = M_PI - ((numSides - 2)*M_PI/numSides);
	else turnAngle = M_PI - ((numSides - 2)*M_PI/numSides);

	char *turnRequest = (char *)malloc(40);
	sprintf(turnRequest, "TURN %.10f %.10f", -M_PI/4, turnAngle);
	char *moveRequest = (char *)malloc(40);
	sprintf(moveRequest, "MOVE 1 %i", length);
	
	int turnRequestLen = strlen(turnRequest);
	int moveRequestLen = strlen(moveRequest);

	DEBUG2("moveRequest = %s\n", moveRequest);
	DEBUG2("moveRequestLen = %i\n", moveRequestLen);


	int numCommands = 4 + (numSides * 8);
	DEBUG2("NumComamnds = %i\n", numCommands);

	int bufferLen = numCommands + 1 + // 1 bytes for each command, + null terminator
			34 /* intiial getSnapshot() */ + (numSides * (48 + turnRequestLen)) /* number of sides * length of commands per side */ + 1 /* null terminator */; // Number of bytes in commands
	string buffer;
	buffer.reserve(bufferLen);


	DEBUG2("BUFFERLEN = %i\n", bufferLen);

	int i;

	// N sides, clockwise
	// Write initial getSnapshot() equivalent
	writeGetSnapshot(buffer);
	for(i = 0; i < numSides;i++)
	{
		addToMessage(buffer, moveRequest);
		addToMessage(buffer, "STOP");
		writeGetSnapshot(buffer);
		addToMessage(buffer, turnRequest);
		addToMessage(buffer, "STOP");
	}
	
	buffer += '\0'; // Null-terminate the data
//	DEBUG2("client.c buffer = %s\n", buffer.c_str());

	free(turnRequest);
	free(moveRequest);
	
	// Send request
	char *cStr = (char *)malloc(buffer.size());
	memcpy(cStr, buffer.c_str(), buffer.size());
	sendRequestNoResponse(cStr);
	free(cStr);
}

int main(int argc, char** argv) {

	//get command line args
	if(argc != 6) {
		fprintf(stderr, "Usage: %s <server IP or server host name> <server port> <robot ID> <L> <N>\n", argv[0]);
		exit(1);	
	}
	char* serverHost = argv[1];
	char* serverPort = argv[2];
	char* robotID = argv[3];
	L = atoi(argv[4]);
	if(L < 1) {
		quit("L must be at least 1");
	}
	N = atoi(argv[5]);
	if(N < 4 || N > 8) {
		quit("N must be an integer the range [4, 8]");
	}
		
	plog("Read arguments");
	plog("Server host: %s", serverHost);
	plog("Server port: %s", serverPort);
	plog("Robot ID: %s", robotID);
	plog("L: %d", L);
	plog("N: %d", N);
	
	plog("Setting up clientMessenger");
	
	setupMessenger(serverHost, serverPort, robotID);


	sendCommandSequence(N, true, L);
	parseResponseSequence(N, true);

	sendCommandSequence(N-1, false, L);
	parseResponseSequence(N-1, false);


	return 0;
}
