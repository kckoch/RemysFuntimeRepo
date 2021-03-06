// Compression.cpp
// Written and designed by TJ Wills
// Credit for solution stolen by Kyra Koch

/*
TO ENCODE A MESSAGE:

A message sent using this library takes the following format:
* [Dictionary][Encoded message to be sent]

The dictionary takes the following format:

[ASCII value of length of entry][Entry][ASCII value of length of entry][Entry][ASCII value of length of entry][Entry]...[\0]


Say you want to send a string variable called "SEND"

Divide the string into substrings of length 4 (truncate last substring if string length mod 4 is not 0)
Keep track of how many times each substring appears
Discard substrings that appear less than 6 times

* Create dictionary to put at start of message
For each substring that remains,
	Append to SEND: [single ASCII character with value of length of entry (at this point, always 0x04)]
	Append to SEND: [entry]
Dictionary ends with the null character, so append to SEND: '\0'

(This method uses an escape character to signify encoding and decoding commands. Right now, the escape character is 0xFE, but it might change in the future.)

Start reading message you want to send.
	When a non-escape character appears, it will be copied directly (unless it's part of a substring which appears in the dictionary)
	Any time the escape character appears in the message you want to send, it will be entered twice.
	Any time a substring appears that exists in the dictionary, replace it with the following:
		[escape character]
		[ASCII character which represents the index of the dictionary entry. For example, the first entry in the dictionary has an index of 0]
		[ASCII character which represents the number of times the sequence repeats, up to [ESCAPECHARACTER - 1] times

EXAMPLE:

Message to send:

Hellooo 123412341234123412341234123412341234 <> ABCDABCDABCDABCDABCDABCDABCD********1234,[ESCAPECHAR]world!

Dictionary created:
0 - 1234
1 - ABCD

(The substring "****" does not appear in the dictionary because it appears less than 6 times)

Final message to send:
[4]1234[4]ABCD[0]Hellooo [ESCAPECHAR][0][9] <> [ESCAPECHAR][1][7]********[ESCAPECHAR][0][1],[ESCAPECHAR][ESCAPECHAR]world!


NOTES:
Be careful using C++ strings. They don't like the null character, which is a valid character in data transfer.
*/

#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <utility>
#include <set>

#include "Compression.h"

using namespace std;


const string EXAMPLESTRING =
				"  COMPRESSION BEGINS )}>"
				"1234123412341234123412341234123412341234"
				"ABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCD"
				"6660666066606660666066606660666066606660666066606660666066606660666066606660666066606660666066606660"
				"<{( COMPRESSION ENDS";

const char ESCAPECHAR = 254;

map<string, int> createDictionary(string input);


// Read contents of file into string (include null bytes)
string stringFromFile(string fileName);

// Adds "count" characters from *add (character pointer) to &target, including null bytes
void addToString(string &target, void *add, int count);

// Adds the contents of &add into target, including null bytes
void addToString(string &target, const string &add);

// Add "length" bytes from "add" at position "position" to "target"
void addSubstringToString(string &target, const string &add, int position, int length);

// Filters out all but top ESCAPECHAR entries from dictionary
void trimDictionary(map<string, int> &dictionary);


// Debug functions
string debugString(string s);
void printString(string s);
void elaborateString(string s);



string debugString(string s)
{
	string ret;
	ret.reserve(s.length());

	char c;

	for(int i = 0; i < s.length(); i++)
	{
		c = s[i];
		if(c == '\r')
		{
			ret += "{\\r}";
		}
		else if(c == '\n')
		{
			ret += "{\\n}";
		}
		else if(c != '\0')
		{
			ret += c;
		}
		else
		{
			ret += "{\\0}";
		}
	}

	return ret;
}

void printString(string s)
{
	for(int i = 0; i < s.length(); i++)
	{
		cout << s[i];
	}
}

void elaborateString(string s)
{
	for(int i = 0; i < s.length(); i++)
	{
		string SI;
		if(s[i] != '\0')
			SI += s[i];
		else
			SI += "{\\0}";
		cout << i << "E | " << SI << " | " << (int) (unsigned char) s[i] << endl;
	}
}

// Read contents of file into string (include null bytes)
string stringFromFile(string fileName)
{
	ifstream ifs(fileName.c_str(), ios::binary);
	return string( (std::istreambuf_iterator<char>(ifs) ),
	                       (std::istreambuf_iterator<char>()    ) );
}

// Adds "count" characters from *add (character pointer) to &target, including null bytes
void addToString(string &target, void *add, int count)
{

//	DEBUG("Target before = " << debugString(target) << endl);

//	DEBUG("Add = " << (char *)add);
//	DEBUG(endl << "Count = " << count << endl);
//	DEBUG("TARGET.length() = " << target.length() << " | " << target << endl);

	int start = target.length();
//	DEBUG("inserting into start " << count)
	target.insert(start, count, 'Z');

	char *addition = (char *) add;



	for(; start < target.length(); start++)
	{
//		DEBUG(start << " | " << target.length() << " | " << (int) (addition)[0] << " | " << (addition)[0] << endl);
		target[start] = (addition++)[0];
	}

//	DEBUG("Target after  = " << debugString(target) << endl);
}

// Adds the contents of &add into target, including null bytes
void addToString(string &target, const string &add)
{
//	DEBUG("@@@target.length() = " << target.length() << endl);
	addToString(target, (void *) add.c_str(), add.length());

//	char *temp = new char[add.length() + 1];
//
//	for(int i = 0; i < add.length(); i++)
//	{
//		temp[i] = add[i];
//	}
//	temp[add.length()] = '\0';
//
//	addToString(target, (void *) temp, add.length());
//
//	delete[] temp;
}

// Add "length" bytes from "add" at position "position" to "target"
void addSubstringToString(string &target, const string &add, int position, int length)
{
	int start = target.length();

	target.insert(target.length(), length, 'Z');

	for(; start < target.length(); start++)
	{
		target[start] = add[position++];
	}
}

// Filters out all but top ESCAPECHAR entries from dictionary
void trimDictionary(map<string, int> &dictionary)
{

	// Handle case dictionary does not need trimming - ie, size of dictionary < ESCAPECHAR
	if(dictionary.size() < (unsigned int) (unsigned char) ESCAPECHAR)
	{
//		DEBUG("dictionary.size() < ESCAPECHAR " << dictionary.size() << " < " << (int) (unsigned char) ESCAPECHAR << endl);
		return;
	}

	set<pair<int, string> > filter;

	for(map<string, int>::iterator it = dictionary.begin(); it != dictionary.end();)
	{
//		DEBUG("Filter " << it->second << " | " << debugString(it->first) << "\n");
		filter.insert(pair<int, string>(it->second, it->first));
		it++;
	}

	dictionary.clear();


	set<pair<int, string> >::reverse_iterator it = filter.rbegin();

	for(set<pair<int, string> >::reverse_iterator it2 = filter.rbegin(); it2 != filter.rend();)
	{
		it2++;
	}

	for(int i = 0; i < ((int) (unsigned char) ESCAPECHAR) && (it != filter.rend()); i++)
	{
//		DEBUG(i << endl);
//		DEBUG(it->first << " | " << debugString(it->second) << endl);
		dictionary[it->second] = it->first;
		it++;
	}

//	DEBUG("FINAL dictionary size = " << dictionary.size() << endl);
}

/*
int main()
{
	string messageReceived = EXAMPLESTRING;
	// Alternately, read in file to read
	string read;
	cout << "Enter file name to read :";
	getline(cin, read);
	messageReceived = stringFromFile(read);

	cout << "READING FILE: " << read << endl;
	cout << "TESTING: C implementation\n";


	// Encode message
//	string send = encodeMessage(messageReceived, dictionary);
	// Test C
	int receivedSize;
	char *sendChar = encodeMessage(messageReceived.c_str(), messageReceived.size(), &receivedSize);
	string send = "";


	for(int i = 0; i < receivedSize; i++)
	{
		send += sendChar[i];
	}

//	send.resize(receivedSize);
////	send.insert(0, receivedSize, 'Z');
//	for(int i = 0; i < receivedSize; i++)
//	{
//		send[i] = sendChar[i];
//	}

	// Decode message
//	string decoded = decodeMessage(send);
	// Test C
	int decodedSize;
	char *decodedChar = decodeMessage(send.c_str(), send.size(), &decodedSize);
	string decoded; decoded.resize(decodedSize);
	for(int i = 0; i < decodedSize; i++)
	{
		decoded[i] = decodedChar[i];
	}

	DEBUG(messageReceived.length() << " | " << decoded.length() << " | Sizes\n");
	DEBUG("Received message == decoded message??? : " << (decoded == messageReceived ? "TRUE!!!" : "false...") << endl);

	// Will never happen in final version of code
	if(decoded != messageReceived)
	{
		cout << "\n\n\nFATAL ERROR!!!!! decoded != messageReceived!!!!\n\n\n";
		cout.flush();

		int firstError = 0;
		for(firstError = 0; firstError < min(decoded.length(), messageReceived.length()); firstError++)
		{
				if(decoded[firstError] != messageReceived[firstError])
				{
					break;
				}
		}

		DEBUG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		cout << "First error occurs: " << firstError << endl;
		elaborateString(decoded.substr(max(0, firstError-4), 16));
		cout << endl;
		elaborateString(messageReceived.substr(max(0, firstError-4), 16));
		DEBUG("@@@@@@@@@@@@@@@@@@\n");

//		for(map<string, int>::iterator i = dictionary.begin(); i != dictionary.end();)
//		{
//			DEBUG("- " << i->first << " | " << i->second << endl);
//			i++;
//		}


		cout << "\n\n\nFATAL ERROR!!!!! decoded != messageReceived!!!!\n\n\n";
				cerr.flush();
	}

	DEBUG("    Original message length: " << messageReceived.length() << endl);
	DEBUG("     Encoded message length: " << send.length() << endl);
	DEBUG("Percent throughput increase: " << ((((double) messageReceived.length()) / send.length()) - 1.0) * 100 << "%" << endl);

	return 0;
}
*/


map<string, int> createDictionary(string input)
{
//	DEBUG(debugString(input) << endl);


	map<string, int> dictionary;

	// Parse input into sections of 4 characters long, and
	// count how many times each section appears
	for(int i = 4; i < input.length(); i += 4)
	{
		string token = input.substr(i-4, 4);
		dictionary[input.substr(i-4, 4)] += 1;
	}



	// Find tokens worth trimming, ie, tokens that appear more than 5 times
	// (Any less than 5 times, and it would be more expensive to keep them in
	for(map<string, int>::iterator i = dictionary.begin(); i != dictionary.end();)
	{
//		DEBUG("- " << debugString(i->first) << " | " << i->second << endl);
		if(i->second < 6)
		{
			dictionary.erase(i++);
		}
		else
		{
			i++;
		}
	}



//	for(map<string, int>::iterator i = dictionary.begin(); i != dictionary.end(); i++)
//	{
//		DEBUG("- " << debugString(i->first) << " | " << i->second << endl);
//	}

	// Trim all but first [ESCAPECHAR] entries
	trimDictionary(dictionary);


	return dictionary;
}

// FORMAT OF ENCODED MESSAGE:
// <Dictionary section><Original message (with compression applied)>
//
// Dictionary section:
//		{[length of entry][entry itself]}{[length of entry][entry itself]}...\0
//
// Original message compressed:
// 	Any instance of ESCAPECHAR in the original message has been replaced with two instances of ESCAPECHAR
// 	Any time a string from the dictionary appears, that string is replaced with:
//			ESCAPECHAR + <Entry number in dictionary> + <number of times string repeats (max of (ESCAPECHAR - 1) times)>
string encodeMessage(const string &message, map<string, int> dictionary)
{
	string ret;
	// Unfortunate scenario where no compression can take place
	if(dictionary.size() == 0)
	{
		ret.reserve(1 + message.size());
		ret += '\0';

		addToString(ret, message);
//		ret += message;
		return ret;
	}




//	for(int i = 0; i < 10; i++)
//	{
//		DEBUG(i << " | " << message[i] << " | " << (int) (unsigned char) message[i] << endl);
//	}

	int position = 4;
	ret.reserve(message.size());


	// Keys is a map that maps the entry with a number. This number is used to decode the message later
	map<string, int> keys;



	int counter = 0;
	for(map<string, int>::iterator i = dictionary.begin(); i != dictionary.end();)
	{
		ret += i->first.length();

		// From when it was believed to be bad to have \0 or ESCAPECHAR in a dictionary entry
//		for(int x = 0; x < i->first.length(); x++)
//		{
//			char c = i->first[x];
//			if(c == ESCAPECHAR)
//			{
//				ret += c;
//				ret += c;
//			}
//			else if(c == '\0')
//			{
//				ret += ESCAPECHAR;
//				ret += c;
//			}
//			else
//			{
//				ret += c;
//			}
//		}

//		DEBUG("Addint to string " << i->first << endl);
		addToString(ret, i->first);
//		ret += i->first;

		keys[i->first] = counter++;
		i++;
	}




	// Terminate dictionary with \0
	ret += '\0';

	// Search for tokens of 4 length which appear in the dictionary
	while(position < message.length())
	{
		string token = message.substr(position - 4, 4);

		// If string was not found, print as normal
		if(dictionary.find(token) == dictionary.end())
		{
			ret += token[0];

			// Add ESCAPECHAR twice, as per the protocol
			if(token[0] == ESCAPECHAR)
			{
				ret += token[0];
			}

			position++;
		}

		// If string was found, determine dictionary entry and number of repeats
		else
		{
////			DEBUG("DICTIONARY TOKEN FOUND: " << token << endl);
			ret += ESCAPECHAR;
			ret += (unsigned char) keys[token];

			string target = token;
			int repeats = 1;
			position += 4;

			// Determine how many times the string repeats
			while(repeats < ((unsigned char) (ESCAPECHAR) -1) && position < message.length())
			{
				token = message.substr(position - 4, 4);
				if(token != target)
				{
////					DEBUG(token << " | " << target << endl);
					break;
				}
				repeats++;
				position += 4;
			}
			ret += repeats;
////			DEBUG(token << " repeats " << repeats << " times.\n");
		}
	}

	// Add final portion of message, part that only exists when message is not divisible by 4
	position = position - 4;
	while(position < message.length())
	{
		ret += message[position++];
	}

	return ret;
}

string decodeMessage(string message)
{
	// Handle unfortunate case where there was no compression
	if(message[0] == '\0')
	{
		return message.substr(1);
	}

	string ret;
	ret.reserve(message.size());

	vector<string> keys;

	int pos = 0;

	// Parse dictionary
	while(message[pos] != '\0')
	{
		int nextKeyLen = message[pos++];
		string temp;

		// From when it was though that having ESCAPECHAR or \0 in dictionary was bad
//		for(int i = 0; i < nextKeyLen; i++)
//		{
//			char c = message[pos++];
//
//			if(c != ESCAPECHAR)
//			{
//				temp += c;
//			}
//			else
//			{
//				temp += message[pos++];
//			}
//		}
		addSubstringToString(temp, message, pos, nextKeyLen);
		keys.push_back(temp);
//		keys.push_back(message.substr(pos, nextKeyLen));

//		DEBUG(pos << " | Adding to dictionary: " << message.substr(pos, nextKeyLen) << endl);
		pos += nextKeyLen;
	}

//	DEBUG("DICTIONARY PARSED: " << keys.size() << endl);

	pos++;

	while(pos < message.length())
	{
		// Print out all characters except ESCAPECHAR as normal
		if(message[pos] != ESCAPECHAR)
		{
//			DEBUG(pos << " | Nothing of interest\n");
			ret += message[pos++];
		}
		else
		{
			// If two ESCAPECHAR's appear, the decoding is a single ESCAPECHAR
			if(message[++pos] == ESCAPECHAR)
			{
//				DEBUG(pos << " | One escape char\n");
				ret += ESCAPECHAR;
				pos++;
			}
			// If anything else appears, the decoding is based on the next few values
			else
			{
				// Value to add is keys[ (next ascii value) ]
				string target = keys[(int) (unsigned char) message[pos++]];

				// Number of times to add it is (ascii value of the character after that)
				int repeats = (unsigned char) message[pos++];

				for(int i = 0; i < repeats; i++)
				{
					addToString(ret, target);
//					ret += target;
				}
			}
		}

	}

	return ret;
}

/* These two work - C Implementations */
char *encodeMessage(char const *message, int count, int *messageSize)
{
	string messageString;
	messageString.resize(count);

	for(int i = 0; i < count; i++)
	{
		messageString[i] = message[i];
	}

	map<string, int> dictionary = createDictionary(messageString);

	string returnString = encodeMessage(messageString, dictionary);

	*messageSize = returnString.size();
	char *ret = new char[returnString.size()];

	for(int i = 0; i < returnString.size(); i++)
	{
		ret[i] = returnString[i];
	}

	return ret;
}


char *decodeMessage(const char *message, int count, int *messageSize)
{
	string messageString;
	messageString.resize(count);

	for(int i = 0; i < count; i++)
	{
		messageString[i] = message[i];
	}

	string decodedMessage = decodeMessage(messageString);

	*messageSize = decodedMessage.size();

	char *ret = new char[decodedMessage.size()];

	for(int i = 0; i < decodedMessage.size(); i++)
	{
		ret[i] = decodedMessage[i];
	}

	return ret;
}


/* THIS SHIT DOES NOT WORK EVNE THOUGH IT'S EXACTLY THE SAME AS THE ABOVE */
//// C implementation - count = length of *message
//char *encodeMessage(const char *message, int count, int *messageSize)
//{
//	//	string messageString(message, count);
//	string messageString;
//	messageString.resize(count);
//	DEBUG("Resized to " << count << " | " << messageString.size() << endl);
////	messageString.insert(0, count, 'Z');
//
//	for(int i = 0; i < count; i++)
//	{
//		messageString[i] = message[i];
//	}
//
//	map<string, int> dictionary = createDictionary(messageString);
//
//	DEBUG("Dictionary = " << dictionary.size() << endl);
//
//	string retString = encodeMessage(messageString, dictionary);
//
//
//	*messageSize = retString.size();
//
//	DEBUG("@@@@here\n");
//	char *ret = new char(retString.size());
//	for(int i = 0; i < retString.size(); i++)
//	{
//		ret[i] = retString[i];
//	}
//
//	return ret;
//}
//
//// C implementation - count = length of *message
//char *decodeMessage(const char *message, int count, int *messageSize)
//{
//	string messageString;
////	messageString.insert(0, count, 'Z');
//	messageString.resize(count);
//
//	for(int i = 0; i < count; i++)
//	{
//		messageString[i] = message[i];
//	}
//
//	string retString = decodeMessage(messageString);
//
//	*messageSize = retString.size();
//
//	char *ret = new char[retString.size()];
//	for(int i = 0; i < retString.size(); i++)
//	{
//		ret[i] = retString[i];
//	}
//
//	return ret;
//}
