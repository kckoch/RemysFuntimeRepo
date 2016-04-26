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

#define USE_NEW_METHOD 1
#define DEBUG1(X) fprintf(stderr, X);
#define DEBUG2(X, Y) fprintf(stderr, X, Y);
#define DEBUG3(X, Y, Z) fprintf(stderr, X, Y, Z);

void tracePolygon(int numSides, bool clockwise);
void getSnapshot();
double getTime();

int L; //L >= 1
int N; //4 <= N <= 8
int fileCount = 0;

const double COMMAND_TIMEOUT = 0.95;
const double DATA_TIMEOUT = 5.0;

// Returns length of string added
int addToMessage(char *message, const char *add)
{
//	DEBUG2("ADDING TO MESSAGE: {%s}\n", add);
//	DEBUG2("MESSAGE BEFORE: {%s}\n", message);
	int addLen = strlen(add);
	message[0] = addLen;
	strcpy(message+1, add);
//	DEBUG2("MESSAGE AFTER: {%s}\n", message);
//	DEBUG2("MESSAGE %i -> ", *message);
	message += addLen;
//	DEBUG2("%i\n", *message);

	return addLen + 1;

}

int writeGetSnapshot(char *message)
{
	message += addToMessage(message, "GET IMAGE");
	message += addToMessage(message, "GET GPS");
	message += addToMessage(message, "GET DGPS");
	message += addToMessage(message, "GET LASERS");

	return 38;
}

// Generate and send the desired command sequence
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

//	DEBUG2("turnRequest = %s\n", turnRequest);
//	DEBUG1("turnRequest != LOL\n");
//	DEBUG2("turnRequestLen = %i\n", turnRequestLen);

	DEBUG2("moveRequest = %s\n", moveRequest);
//	DEBUG1("turnRequest != LOL\n");
	DEBUG2("moveRequestLen = %i\n", moveRequestLen);


	int numCommands = 4 + (numSides * 8);
	DEBUG2("NumComamnds = %i\n", numCommands);

	int bufferLen = numCommands + 1 + // 1 bytes for each command, + null terminator
			34 /* intiial getSnapshot() */ + (numSides * (48 + turnRequestLen)) /* number of sides * length of commands per side */; // Number of bytes in commands
	char *buffer = (char *) malloc(bufferLen);
	char *writer = buffer;

	DEBUG2("BUFFERLEN = %i\n", bufferLen);

	int i;

	// First iteration - N sides, clockwise
	// Write initial getSnapshot() equivalent
	writeGetSnapshot(writer);
	for(i = 0; i < numSides;i++)
	{
		writer += addToMessage(writer, moveRequest);
		writer += addToMessage(writer, "STOP");
		writer += writeGetSnapshot(writer);
		writer += addToMessage(writer, turnRequest);
		writer += addToMessage(writer, "STOP");
	}
	
	// Second iteration - N-1 sides, counterclockwise
	// Write initial getSnapshot() equivalent
	writeGetSnapshot(writer);
	for(i = 0; i < numSides;i++)
	{
		writer += addToMessage(writer, moveRequest);
		writer += addToMessage(writer, "STOP");
		writer += writeGetSnapshot(writer);
		writer += addToMessage(writer, turnRequest);
		writer += addToMessage(writer, "STOP");
	}
	
	DEBUG2("buffer = %s\n", buffer);
	
	
//	DEBUG3("bufferLength = %i/%i\n", bufferLen, RESPONSE_MESSAGE_SIZE);
//	int segments = bufferLen / RESPONSE_MESSAGE_SIZE + 1;
//	DEBUG2("Segments = %i\n", segments);

	



	
}

// Receive and handle transmissions until all commands have been parsed
void parseResponseSequence()
{
	return;
}

void tracePolygon(int numSides, bool clockwise) {
   int dummy;
   double timeSpent;
   double sleepTime;
   int waitSeconds;
   int waitUSeconds;
   
   plog("tracing polygon of order %d", numSides);
   plog("Clockwise: %d", clockwise);
   
   //Determine the angle the robot should turn.
   double turnAngle = M_PI - ((numSides - 2)*M_PI/numSides);
   
   plog("Turn angle: %lf", turnAngle);
   
   //Create a turn request for pi/4 radians per second.
   char *turnRequest = (char *)malloc(20);
   if(clockwise) sprintf(turnRequest, "TURN %.10f", -M_PI/4);
   else sprintf(turnRequest, "TURN %.10f", M_PI/4);
   
   plog("Turn request: %s", turnRequest);
   
   //Take initial screenshot before making the polygon.
   getSnapshot();

   //Logic for tracing the polygon.
   int i;
   for(i = 0; i < numSides; i++) {
      plog("Iteration: %d", i);
      
      plog("Sending MOVE 1 command");
      //Send a request to begin moving.
      timeSpent = getTime();
      sendRequest("MOVE 1", &dummy, COMMAND_TIMEOUT);
      timeSpent = getTime() - timeSpent;
      
      plog("Time spent sending request and getting response: %lf", timeSpent);

      //Calculate wait time (L - time spent in sendRequest).
      if(L > timeSpent) {
         sleepTime = L - timeSpent;
         
         plog("waiting for %lf seconds", sleepTime);
         
         waitSeconds = (int) sleepTime;
         sleepTime -= waitSeconds;
         waitUSeconds = (int) (sleepTime*1000000);

         //Wait until robot reaches destination.
         sleep(waitSeconds);
         usleep(waitUSeconds);
      }
      
      plog("sending stop command");
      
      //Send a request to stop the robot.
      sendRequest("STOP", &dummy, COMMAND_TIMEOUT);
      
      plog("sent stop command");

      //Take Snapshot after movement has ended.
      getSnapshot();
      
      plog("sending turnRequest: %s", turnRequest);
      //Send a request to begin turning.
      timeSpent = getTime();
      sendRequest(turnRequest, &dummy, COMMAND_TIMEOUT);
      timeSpent = getTime() - timeSpent;
      
      plog("Time spent sending request and getting response: %lf", timeSpent);
      
      /*
      	Manual tests show that the actual speed of the robot is about .89*requested speed
      	(with an absolute maximum speed of ~M_PI/4 radians/second)
      	This is an unavoidable hack since we are working with flawed hardware.
      */
      const double actualSpeed = .89*M_PI/4;
      //Calculate wait time (turnAngle/(M_PI/4) - time spent in sendRequest).
      if(turnAngle/actualSpeed > timeSpent) {
         sleepTime = turnAngle/actualSpeed - timeSpent;
         
         plog("waiting for %lf seconds", sleepTime);
         
         waitSeconds = (int) sleepTime;
         sleepTime -= waitSeconds;
         waitUSeconds = (int) (sleepTime*1000000);

         //Wait until robot turns to correct orientation.
         sleep(waitSeconds);
         usleep(waitUSeconds);
      }
      
      plog("sending stop command");
      //Send a request to stop turning.
      sendRequest("STOP", &dummy, COMMAND_TIMEOUT);
      plog("sent stop command");
   }
}

void getSnapshot() {
   int length;
   char *data;

   FILE *imageFile;       //File for the image data received.
   FILE *positionFile;    //File for the position data received.

   char *imageFileName = (char *)malloc(50);
   char *positionFileName = (char *)malloc(50);

   sprintf(imageFileName, "image-%d.jpg", fileCount);
   sprintf(positionFileName, "position-%d.txt", fileCount);
   ++fileCount;

   imageFile = fopen(imageFileName, "w+");
   positionFile = fopen(positionFileName, "w+");

   //Get the image and write the data to the image file created.
   data = (char *)sendRequest("GET IMAGE", &length, DATA_TIMEOUT);
   if(fwrite(data, 1, length, imageFile) != length) quit("fwrite failed");
   
   //The imageFile is no longer needed.
   free(data);
   fclose(imageFile);

   //Get GPS data from robot and print to positionFile
   data = (char *)sendRequest("GET GPS", &length, DATA_TIMEOUT);
   fprintf(positionFile, "GPS ");
   if(fwrite(data, 1, length, positionFile) != length) quit("fwrite failed");

   fprintf(positionFile, "\n");

   free(data);

   //Get DGPS data from robot and print to positionFile
   data = (char *)sendRequest("GET DGPS", &length, DATA_TIMEOUT);
   fprintf(positionFile, "DGPS ");
   if(fwrite(data, 1, length, positionFile) != length) quit("fwrite failed");

   fprintf(positionFile, "\n");

   //Get LASER data from robot and print to positionFile
   data = (char *)sendRequest("GET LASERS", &length, DATA_TIMEOUT);
   fprintf(positionFile, "LASERS ");
   if(fwrite(data, 1, length, positionFile) != length) quit("fwrite failed");
   
   fprintf(positionFile, "\n");

   free(data);
   fclose(positionFile);
}

double getTime() {
   struct timeval curTime;
   (void) gettimeofday(&curTime, (struct timezone *)NULL);
   return (((((double) curTime.tv_sec) * 10000000.0)
      + (double) curTime.tv_usec) / 10000000.0);
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

	if(USE_NEW_METHOD == 1)
	{
		sendCommandSequence(N, true, L);
		parseResponseSequence();

		sendCommandSequence(N-1, false, L);
		parseResponseSequence();


	}
	else
	{
		tracePolygon(N, true);
		tracePolygon(N-1, false);
	}

	return 0;
}
