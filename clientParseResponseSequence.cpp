// clientParseResponseSequence - written by TJ Wills

#include <vector>
#include "string"

#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>	//for uint32_t
#include <sys/time.h>	//for setitimer()
#include <string.h>     //for memset()
#include <sys/socket.h> //for socket(), connect(), sendto(), and recvfrom()
#include <unistd.h>     //for close()
#include <netdb.h>		//for addrinfo
#include <signal.h>		//for signal
#include <assert.h>		//for assert()
#include <iostream>
#include <fstream>
#include "Compression.cpp"

#define DEBUG1(X) fprintf(stderr, X);
#define DEBUG2(X, Y) fprintf(stderr, X, Y);
#define DEBUG3(X, Y, Z) fprintf(stderr, X, Y, Z);
//#define DEBUG(X) cout << "{" << __LINE__ << "} | " << X; cout.flush();

#define IMAGE 2
#define GPS 3
#define DGPS 4
#define LASERS 5

using namespace std;

char example[] = "\0\0\0\0" // Client ID 1
		"\0\0\0\2" // 2 UDP Messages
		"\0\0\0\1" // Message Index 1
		"\0\0\0\27" // Command index 23 (\27 in octal)
		"world!"; // Message
char example2[] = "\0\0\0\0" // Client ID 1
		"\0\0\0\2" // 2 UDP Messages
		"\0\0\0\0" // Message Index 0
		"\0\0\0\27" // Command index 23 (\27 in octal)
		"Hell\0, "; // Message

char exampleImage[] = "\0\0\0\0" // Client ID 1
		"\0\0\0\1" // 1 UDP Message
		"\0\0\0\0" // Message Index 0
		"\0\0\0\26" // Command index 22 (\26 in octal)
		; // No message - to be appended later

extern const int RESPONSE_MESSAGE_SIZE;
extern int sock;

static uint32_t ID;

bool firstLoop = false;
int numSides = 0;
bool positionFileWritten[17];

vector<vector <string> > responses;

int extractMessageID(void* message);
int extractNumMessages(void* message);
int extractSequenceNum(void* message);

///// FROM CLIENTMESSENGER.CPP //////////////////
/*
int extractMessageID(void* message) {
	return ntohl(*((uint32_t*) message));
}

int extractNumMessages(void* message) {
	return ntohl(*(((uint32_t*) message)+1));
}

int extractSequenceNum(void* message) {
	return ntohl(*(((uint32_t*) message)+2));
}
///////////////////////////////////////////////
*/

// Debug
/*
void elaborateString(string str)
{
	for(int i = 0; i < str.size(); i++)
	{
		DEBUG(i << " | " << (int) (unsigned char) str[i] << " | " << str[i] << " | ElaborateString\n");
	}
}
*/

// Returns command index
int extractCommandIndex(void* message) {
	return ntohl(*(((uint32_t*) message)+3));
}


// Extracts the message from message, and returns an equivalent string, including null bytes
string extractMessage(void *message, int len)
{
	char *messageChar = ((char *) message) + 16;
	string ret;
	int size = len - 16;
	ret.resize(size);
	for(int i = 0; i < size; i++)
	{
		ret[i] = messageChar[i];
	}
	return ret;
}

// Appends the specied string to message, including null bytes
void addToString(string &message, string &add)
{
	for(int i = 0; i < add.size(); i++)
	{
		message += add[i];
	}
}

// Sequentially appends each string in vec to the returned string
string extractMessage(vector<string> &vec)
{
	string ret;
	for(int i = 0; i < vec.size(); i++)
	{
		addToString(ret, vec[i]);
	}
	return ret;
}

// Returns true if and only if each element of target is not empty
bool isVectorFull(vector<string> &target)
{
	for(int i = 0; i < target.size(); i++)
	{
		if(target[i].size() == 0)
			return false;
	}
	return true;
//	return (target.size() == target.capacity());
}

// Ensures the given vector has at least newSize elements
void resizeUpTo(vector<vector <string> > &vec, int newSize)
{
	if(vec.size() < newSize)
	{
		vec.resize(newSize);
	}
}

// Ensures the given vector has at least newSize elements
void resizeUpTo(vector<string> &vec, int newSize)
{
	if(vec.size() < newSize)
	{
		vec.resize(newSize);
	}
}

// Appends message to specified output stream, including null bytes
void appendToFile(ofstream &out, const string &message)
{
	DEBUG("Appending to file: {" << message << "}" << endl);
	if(out.good())
	{
		for(int i = 0; i < message.size(); i++)
		{
			out << message[i];
		}
	}
	else
	{
		DEBUG("FATAL ERROR: OUTPUT FILE NO GOOD\n");
		exit(-1);
	}
}

// 0-3 = side 0
// 4-11 = side 1
// 12-19 = side 2
// 20-27 = side 3
// etc
// If it's the second loop, add (numsides + 1) to the return value to compensate for the first loop
int getSideNumber(int commandIndex)
{
	DEBUG("GETSIDENUMBER " << commandIndex << " | " << commandIndex + 4 << " | " << (commandIndex + 4) / 8 << endl);
	if(firstLoop)
		return ( (commandIndex + 4) / 8);
	else
		return ( ( (commandIndex + 4) / 8) + numSides + 1 );
}

// Decompress the information stored in message,
// and print output to an appropriately-named image file
void handleImage(const string &message, int commandIndex)
{
	DEBUG("HANDLEIMAGE START");

	char name[15];
	sprintf(&name[0], "image-%d.jpg", getSideNumber(commandIndex));

	string imageString = decodeMessage(message);

	DEBUG(" - " << name << endl);

	ofstream out(name);
	appendToFile(out, imageString);
}

void writePositionData(const string prepend, const string &message, int commandIndex)
{
	int sideNumber = getSideNumber(commandIndex);

	char name[15];
	sprintf(&name[0], "position-%d.txt", sideNumber);

	DEBUG(" - " << name << endl);

	// If this is the first time writing to this position file, 
	// clear the contents of the file
	if(!positionFileWritten[sideNumber])
	{
		ofstream clear(name);
		positionFileWritten[sideNumber] = true;
		clear.close();
	}

	ofstream out(name, ofstream::app);
	appendToFile(out, prepend);
	appendToFile(out, message);
	out << '\n';
}

// Open an appropriately-named file, and append GPS information to it
void handleGPS(const string &message, int commandIndex)
{
	DEBUG("HANDLEGPS START");
	writePositionData("GPS ", message, commandIndex);

/*
	int sideNumber = getSideNumber(commandIndex);

	char name[15];
	sprintf(&name[0], "position-%d.txt", sideNumber);

	DEBUG(" - " << name << endl);

	// If this is the first time writing to this position file, 
	// clear the contents of the file
	if(!positionFileWritten[sideNumber])
	{
		ofstream clear(name);
		positionFileWritten[sideNumber] = true;
		clear.close();
	}

	ofstream out(name, ofstream::app);
	appendToFile(out, "GPS ");
	appendToFile(out, message);
	out << '\n';
*/
}

// Open an appropriately-named file, and append DGPS information to it
void handleDGPS(const string &message, int commandIndex)
{
	DEBUG("HANDLEDGPS START");
	writePositionData("DGPS ", message, commandIndex);

/*
	char name[15];
	sprintf(&name[0], "position-%d.txt", getSideNumber(commandIndex));

//	DEBUG(" - " << name << endl);

	ofstream out(name, ofstream::app);
	appendToFile(out, "DGPS ");
	appendToFile(out, message);
	out << '\n';
*/
}

// Open an appropriately-named file, and append laser information to it
void handleLasers(const string &message, int commandIndex)
{
	DEBUG("HANDLELASERS START");
	writePositionData("LASERS ", message, commandIndex);

/*
	char name[15];
	sprintf(&name[0], "position-%d.txt", getSideNumber(commandIndex));

//	DEBUG(" - " << name << endl);

	ofstream out(name, ofstream::app);
	appendToFile(out, "LASERS ");
	appendToFile(out, message);
	out << '\n';
*/
}

void handleResponse(int commandIndex)
{
	const string response = extractMessage(responses[commandIndex]);
//	DEBUG("Handling response [" << commandIndex << "] (" << response.size() << ") {");
//	for(int i = 0; i < response.size(); i++)
//	{
//		cout << response[i];
//	}
//	cout << endl;

	// Requests sent always take the following order:
	//
	// * First 4 requests:
	// [image][gps][dgps][lasers
	//
	// * Next commands, repeated up to eight times:
	// [move][stop][image][gps][dgps][lasers][turn][stop]
	//
	// From this order, we can determine using modulus math
	// what kind of data we're handling
	int commandCode = commandIndex;
	if(commandCode <= 4)
	{
		commandCode += 2;
	}
	else
	{
		commandCode += 4;
	}
	commandCode %= 8;

	// IMAGE, GPS, DGPS, and LASERS are #define statments
	if(commandCode == IMAGE)
	{
		DEBUG("HANDLING IMAGE\n");
		handleImage(response, commandIndex);
	}
	else if(commandCode == GPS)
	{
		DEBUG("HANDLING GPS\n");
		handleGPS(response, commandIndex);
	}
	else if(commandCode == DGPS)
	{
		DEBUG("HANDLING DGPS\n");
		handleDGPS(response, commandIndex);
	}
	else if(commandCode == LASERS)
	{
		DEBUG("HANDLING LASERS\n");
		handleLasers(response, commandIndex);
	}
	else
	{
		DEBUG("UNKNOWN CODE " << commandCode << " | " << commandIndex << "\n");
	}
	
}

// Receive all transmissions from the server
// Sort them into appropriate response buffers
// Parse the data from the response buffers when applicable
// Return when "numCommands" commands have successfully been completed and parsed
// Algorithm and supporting functions written by TJ Wills
void parseResponseSequence(int numSidesNEW, bool clockwise)
{
	DEBUG1("PARSERESPONSESEQUENCE START\n");

	// Initialize positionFileWritten to all false
	for(int i = 0; i < 17; i++)
	{
		positionFileWritten[i] = false;
	}

	numSides = numSidesNEW;
	int numCommands = 4 + (numSides * 8);
	firstLoop = clockwise;

/*
// Test handleX() functions
///
	handleGPS("GPS DATA LOL", 5);
	handleDGPS("DGPS DATA LOL", 6);
	handleLasers("LASER DATA LOL", 7);
	handleGPS("GPS DATA LOL", 5);
	handleGPS("GPS DATA LOL", 5);
	handleGPS("GPS DATA LOL", 5);

	string imageLol;
	imageLol += '\6';
	imageLol += "12345 ";
	imageLol += '\0';
	imageLol += 254;
	imageLol += '\0';
	imageLol += '\6';
	addToString(imageLol, "IMAGE LOL");
	handleImage(imageLol, 88);
///
*/	
	// While there are unanswered commands
	while(numCommands > 0)
	{
		char *message = (char *) malloc(RESPONSE_MESSAGE_SIZE);

		int len = recv(sock, message, RESPONSE_MESSAGE_SIZE, 0);

		if(len <= 0)
			quit("server doesn't exist or recv() failed");
	

		if(extractMessageID((void *) message) != ID)
		{
			quit("Improper message ID");
			DEBUG3("%i | %i\n", extractMessageID((void *) message), ID);
			exit(-1);
		}

		// Extract command index and ensure our responses vector
		// has room to hold the cluster of responses for it
		int commandIndex = extractCommandIndex((void *) message);
		resizeUpTo(responses, commandIndex + 1);

		// Extract the number of messages to be sent for this command index, and
		// ensure the appropriate vector has enough room to hold these responses
		int numberMessages = extractNumMessages((void *) message);
		resizeUpTo(responses[commandIndex], numberMessages);

		// Extract message index
		int messageIndex = extractSequenceNum((void *) message);

		// Extract message
		string messageStr = extractMessage((void *) message, len);

		// Save the extracted to the appropriate location
		responses[commandIndex][messageIndex] = messageStr;

		// When we have received a full response for a given command index,
		// perform the appropriate action
		if(isVectorFull(responses[commandIndex]))
		{
			handleResponse(commandIndex);
			responses[commandIndex].clear(); // Free this vector after we're done with it
			numCommands--; // Decrement reamining commands
		}
		else
		{
			
		}

		free(message);
	}

	return;
}

// Receive all transmissions from the server
// Sort them into appropriate response buffers
// Parse the data from the response buffers when applicable
void parseResponseSequenceTest(int numCommands)
{
	DEBUG1("PARSERESPONSESEQUENCETEST START\n");
	DEBUG1("@!@!@!@ THIS FUNCTION WILL NOT WORK, BECAUSE THE LOGIC HAS BEEN CHANGED TO REQUIRE SETTING SOME GLOBAL VARIABLES\n");
	
	

	char *message = (char *) malloc(RESPONSE_MESSAGE_SIZE);
	
	// Test images
	free(message);
	message = (char *)  malloc(6000);

	free(message);
	message = (char *) malloc(310000);
	
	
	// Simulate receiving example
	for(int i = 0; i < 16; i++)
	{
		message[i] = exampleImage[i];
	}

	string image = stringFromFile("snapshot HUGE.jpg.output");
	DEBUG("IMAGE = " << image.size() << "@@@@@@@@\n");
	int len = 16 + image.size();

	for(int i = 0; i < image.size(); i++)
	{
		message[i+16] = image[i];
	}
//	int len = 22;// recv(sock, message, RESPONSE_MESSAGE_SIZE, 0);

	if(extractMessageID((void *) message) != ID)
	{
		DEBUG3("%i | %i\n", extractMessageID((void *) message), ID);
		exit(-1);
	}

	int commandIndex = extractCommandIndex((void *) message);
	DEBUG2("Command index = %i\n", commandIndex);
	resizeUpTo(responses, commandIndex + 1);

	int numberMessages = extractNumMessages((void *) message);
	DEBUG2("Number messages  = %i\n", numberMessages);
	resizeUpTo(responses[commandIndex], numberMessages);

	int messageIndex = extractSequenceNum((void *) message);
	DEBUG2("Message index  = %i\n", messageIndex);

	string messageStr = extractMessage((void *) message, len);
	DEBUG3("Messagestr = {%s} %i\n", messageStr.c_str(), (int) messageStr.size());

	responses[commandIndex][messageIndex] = messageStr;
	if(isVectorFull(responses[commandIndex]))
	{
		handleResponse(commandIndex);
	}
	else
	{
		DEBUG("NOT HANDLING RESPONSE\n");
	}

	free(message);

// Second iteration - only useful if testing example1 with example 2
////////////////////
/*
	message = (char *) malloc(RESPONSE_MESSAGE_SIZE);
	
	// Simulate receiving example
	for(int i = 0; i < 23; i++)
	{
		message[i] = example2[i];
	}
	len = 23;// recv(sock, message, RESPONSE_MESSAGE_SIZE, 0);

	if(extractMessageID((void *) message) != ID)
	{
		DEBUG3("%i | %i\n", extractMessageID((void *) message), ID);
		exit(-1);
	}

	commandIndex = extractCommandIndex((void *) message);
	DEBUG2("Command index = %i\n", commandIndex);
	resizeUpTo(responses, commandIndex + 1);

	numberMessages = extractNumMessages((void *) message);
	DEBUG2("Number messages  = %i\n", numberMessages);
	resizeUpTo(responses[commandIndex], numberMessages);

	messageIndex = extractSequenceNum((void *) message);
	DEBUG2("Message index  = %i\n", messageIndex);

	messageStr = extractMessage((void *) message, len);
	DEBUG3("Messagestr = {%s} %i\n", messageStr.c_str(), (int) messageStr.size());

	responses[commandIndex][messageIndex] = messageStr;
	if(isVectorFull(responses[commandIndex]))
	{
		handleResponse(commandIndex);
	}


	string finalMessage = extractMessage(responses[commandIndex]);
	DEBUG("finalMessage = {" << finalMessage << "}!!! " << finalMessage.size() << "\n");
	elaborateString(finalMessage);
*/
	return;
}



















