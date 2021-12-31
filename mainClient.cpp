/*Author: Deanna Gierzak
Class: ECE6122
Last Date Modified: 11/13/2020
Description:

	This is the main entry point for the UDP SERVER application. The UDP SERVER
		uses the ECE_UDPSocket class to send and receive messages, and interact with
		the clients that send messages to it. It also uses the class to store messages
		and create the composite message.

	The UDP CLIENT uses the ECE_UDPSocket class to send and receive messages,
		and interact with the server.

	Take as command line argument the Port Number on which the
		UDP SERVER will receive messages using the ECE_UDPSocket class
			  e.g.      ./a.out 51715

	Take as command line argument the Host Name and Port Number to which the
		UDP CLIENT will connect to/send messages using the ECE_UDPSocket class
			  e.g.      ./a.out localhost 51715


*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <iostream>

#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 

#include <list>
#include <string>
#include <mutex>

#include <vector>
#include <atomic>
#include <sstream>
#include <map>


#include "ECE_UDPSocket.h"

using namespace std;
/*
#ifdef _WIN32
// See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  // Windows XP.
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else
// Assume that any non-Windows platform uses POSIX-style sockets instead.
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  // Needed for getaddrinfo() and freeaddrinfo()
#include <unistd.h> // Needed for close()

typedef int SOCKET;
#endif
*/


// g++ server_main.cpp ECE_UDPSocket.cpp -o server.out
// g++ client_main.cpp ECE_UDPSocket.cpp -o client.out

// g++ main.cpp ECE_UDPSocket.cpp -o Lab5.out
// Lab5.out 65112
// Lab5.out localhost 65112
void terminateApplication(ECE_UDPSocket& activeSocket)
{
	// join the thread
	// close socket
	// quit the socket
	
	activeSocket.sockClose();
	activeSocket.sockQuit();
}

void doServerInteractions(ECE_UDPSocket &activeSocket)
{
	bool bStop = false;
	int nCommand{ 0 };
	do
	{
		// Handle the interactions with the user
		cout << "Please enter a command: ";
		cin >> nCommand;

		switch (nCommand)
		{
		case 0: // the server immediately sends to all clients the current 
				//composite message and clears out the composite message
			activeSocket.sendCompositeMsg();
			activeSocket.clearCompositeMsg();
			break;
		case 1: // the server immediately clears out the composite message.
			activeSocket.clearCompositeMsg();
			break;
		case 2: // the server immediately displays to the console the 
				// composite message but takes no other actions.
			activeSocket.displayCompositeMsg();
			break;
		case 3:
			terminateApplication(activeSocket);
			bStop = true;
			break;
		}
	} while (!bStop);

	exit(0);
}

void doClientInteractions(ECE_UDPSocket& activeSocket, string ipAddress, unsigned short portNum)
{
	bool bStop = false;
	std::string inStr;
	int nCommand{ 0 };
	udpMessage outMsg;
	do
	{
		// Handle the interactions with the user
		cout << "\nPlease enter command: ";
		std::getline(std::cin, inStr);			// save everything the user typed into inStr until a breakline is reached
												// Parse the inStr
		vector<string> strings;					// create a vector of strings for parsing out the inStr
		istringstream f(inStr);					// create a streamstream object to read through the inStr
		string s;								// this is each string to push back into the vector of strings 
		while (getline(f, s, ' ')) {			// read each string separated by a space from the f stringstream object into the s string object
			strings.push_back(s);				// push back each string into the vector of strings
		}
		// Handle the following user interactions with the client:
		// v #
		// t # # message string
		// q
		if (strings.size() == 1 && strings[0].at(0) == 'q')			// if the total size of the strings is 1 and its value is "q"
		{
			terminateApplication(activeSocket);							// quit the application
			bStop = true;
		}
		else if (strings.size() == 2 && strings[0].at(0) == 'v')	// if the number of strings in the vector is 2 and the first string is "v"
		{
			outMsg.nVersion = atoi(strings[1].c_str());					// then set the version number of this udpMessage variable to the value of the second string in vector
		}
		else if (strings.size() > 3 && strings[0].at(0) == 't')		// if the number of strings is greater than 3 and the first string is "t"
		{
			string outStr;												// then this indicates it is a new UDP Message to send to the server - create a string for the message output, outStr
			outMsg.nType = atoi(strings[1].c_str());					// set the nType of updMessage object to the second string value
			outMsg.lSeqNum = atoi(strings[2].c_str());					// set the lSeqNum of updMessage object to the third string value
			for (int ii = 3; ii < (int)strings.size(); ii++)			// for the remaining strings in the vector
			{
				outStr += strings[ii];								// add the next string in vector to the message output string, outStr
				if (ii != strings.size() - 1)						// if the iterator is not equal to the last value			
					outStr += " ";										// add a space (to retain the user's inputted message with spaces) since the space delimiting above removes these
			}
			outStr += " ";
			outMsg.nMsgLen = (unsigned short)outStr.length();						// set the nMsgLen (length of the message) equal to the new output message string length
			memcpy(outMsg.chMsg, outStr.c_str(), outStr.length());	// copy over the string to the char type chMsg of the udpMessage object outMsg
			activeSocket.sendMessage(ipAddress, portNum, outMsg);	// after all of this has been done, send the message to the host IP Address and it's port number from input to this function
		}
	} while (!bStop);

	exit(0);
	
}

int main(int argc, char** argv)
{
	bool bIsServer = false;
	std::string strLine;
	unsigned short usPortNum{ 0 };

	string hostAddress;
	unsigned short hostPortNum;

	// Get the command line parameters to determine if this is server or a client
	if (argc == 2)
	{
		// This is a server
		bIsServer = true;
		usPortNum = atoi(argv[1]);		// set the port number to the server's port number
	}
	else if (argc == 3)
	{
		// This is a client
		bIsServer = false;
		hostAddress = argv[1];
		hostPortNum = atoi(argv[2]);
	}
	else
	{
		// This is a mistake
		cout << "You entered a incorrect number of command line arguments" << endl;
		cout << "For a server enter just the port number!" << endl;
		cout << "For a client enter the IP Address and the port number!" << endl;
	}

	// Setup socket
	ECE_UDPSocket udpSocket(usPortNum);

	if (bIsServer)
	{
		doServerInteractions(udpSocket);
	}
	else
	{
		doClientInteractions(udpSocket, hostAddress, hostPortNum);
	}

	return 0;
}








/*
int mainOld(int argc, char** argv)
{
bool bIsVersionCommand = false;
bool bIsTypeCommand = false;
bool bIsQuitCommand = false;

udpMessage newUDPMessage;

bool bIsServer = false;
std::string strLine;
unsigned short usPortNum{ 0 };

// variable to hold remote  IPaddress
struct sockaddr_in serv_addr;

// SERVER
if (argc == 2)
{
// This is a server
bIsServer = true;
usPortNum = atoi(argv[1]);
}
//CLIENT
else if (argc == 3)
{
// This is a client
struct hostent* server;
unsigned short portNum = atoi(argv[2]);

bIsServer = false;
server = gethostbyname(argv[1]);
if (server == NULL)
{
fprintf(stderr, "ERROR, no such host exists/n");
exit(0);
}

// Zero out serv_addr variable
memset((char*)&serv_addr, sizeof(serv_addr), 0);

serv_addr.sin_family = AF_INET;

memmove((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addrtype, server->h_length);

serv_addr.sin_port = htons(portNum);

// Socket class needs a function for sending a message

}
else
{
// This is for mistakes/errors in input
cout << "Incorrect number of command line arguments" << endl;
cout << "For a server enter just a port number" << endl;
cout << "For a client, enter the IP address and the port number" << endl;
}

// Setup socket
//ECE_UDPSocket udpSocket(usPortNum);
//udpSocket::udpMessage;
// Get the command line parameters
// save the host name and port number for this socket
//udpSocket.server = gethostbyname(argv[1]);
//udpSocket.portno = atoi(argv[2]);
string stringInput;
string stringOutput;
stringstream ss(stringInput);
string firstInput;
int x = 1;

// Setup socket
ECE_UDPSocket udpSocket(usPortNum);
int serverCommand;
string clientCommand;

int n = 0;

do
{
// Handle the interactions with the user
cout << "Please enter a command: ";

// How to handle interactions if the server
if (bIsServer)
{
cin >> serverCommand;
switch (serverCommand)
{
case 0:
// server sends current composite message to all clients
udpSocket.sendCompositeMsg();
// server clears out the composite message
udpSocket.sortedMessages.clear();
break;
case 1:
// server clears out the composite message
udpSocket.sortedMessages.clear();
break;
case 2:
// server displays to the console the composite message and takes no other actions
std::cout << "Composite Msg: ";
for (auto entry : udpSocket.sortedMessages)
{
std::cout << entry.second;
}
std::cout << "\n";
break;
case 3:
// server terminates the application
//udpSocket.
udpSocket.quit = true;
//sockClose(udpSocket.sockfd);
udpSocket.sockClose();
break;
}
}

// How to handle interactions if a client
else
{
// for the client only
std::getline(std::cin, stringInput);
udpSocket.receiveClientCommands(stringInput);


}
} while (true);




return 0;
}
*/


/*

n = stringInput.length();

char * charArrClientCommand = new char[n + 1];
// char str[] = "- This, a sample string.";

char * pch;

pch = strtok(charArrClientCommand, " ");
while (pch != NULL)
{
printf("%s\n", pch);
pch = strtok(NULL, " ,.-");
if (pch == "v")
{
bIsVersionCommand = true;
}
else if (pch == "t")
{
bIsTypeCommand = true;
}
else if (pch == "q")
{
// quit the socket & close
bIsQuitCommand = true;
udpSocket.quit = true;
sockClose(udpSocket.sockfd);
}
// this is the value to set the version # after v in command: v #
else if (bIsVersionCommand == true && x == 2)
{
// set version #
newUDPMessage.nVersion = (unsigned short)atoi(pch);
}
// this is the second value to set type after t in command: t # # message
else if (bIsTypeCommand == true && x == 2)
{
// set type #
newUDPMessage.nType = (unsigned short)atoi(pch);
}
// this is the third value to set sequence number after t in command: t # # message
else if (bIsTypeCommand == true && x == 3)
{
// set sequence #
newUDPMessage.lSeqNum = (unsigned short)atoi(pch);
}
// this is the last value for the message after t in command: t # # message
else if (bIsTypeCommand == true && x == 4)
{
// set message
//newUDPMessage.nMsgLen = ;
//newUDPMessage.chMsg = ;
}

x++;
}
bIsVersionCommand = false;
bIsTypeCommand = false;
bIsQuitCommand = false;
x = 1;
*/



// If the composite message becomes larger than 1000, then the composite message (up to 1000) 
//      is immediately sent out to all clients and any remaining characters are used to start
//      a new composite message with a lSeqNum = 0.



/*
do
{
// Handle the interactions with the user:
// For the UDP server, this means it will do the above actions for commands 0 to 3. For other commands, it will ignore them
cout << "Please enter command: ";

// moved swithc-case statement to above


} while (udpSocket.quit == false);
*/

// for the UDP client:
// example user inputs:
// v # 
//      The user enters v, a space, and then a version number. 
//      This version number is now used in all new messages
// t # # message string
//      The user enters t, a space, then a type number, then a sequence number,
//      followed by a message (if any). Be able to handle the spaces in the message
// q
//      The user enters a q, which causes the socket to be closed and 
//      the program to terminate.

// Any messages received should be displayed as follows:
// Received Msg Type: 1, Seq: 33, Msg: message received
//  (displayed by the server? or by the client receiving 
//   something from the server e.g. "Got your message!")



/*
http://www.linuxhowtos.org/C_C++/socket.htm
A simple server in the internet domain using TCP
The port number is passed as an argument
*/


/*
string temp = "cat";
char * tab2 = new char[temp.length() + 1];
strcpy(tab2, temp.c_str());
*/
/*
string s = "hello there";
int m = s.length();
//char p[s.length()];				// doesn't work
char * test = new char[m+1];	// proper way
*/
/*
char str[] = "- This, a sample string.";
char * pch;
printf("Splitting string \"%s\" into tokens:\n", str);
pch = strtok(str, " ,.-");
while (pch != NULL)
{
printf("%s\n", pch);
pch = strtok(NULL, " ,.-");
}
*/