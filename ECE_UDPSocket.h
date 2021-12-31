/*
Author: Deanna Gierzak
Class: ECE6122
Last Date Modified: 11/13/2020
Description:

	The UDP SERVER uses the ECE_UDPSocket class to send and receive messages, and interact with
		the clients that send messages to it. It also uses the class to store messages
		and create the composite message. The UDP CLIENT uses the ECE_UDPSocket class to send and 
		receive messages, and interact with the server.
	The UDP SERVER collects parts of a larger composite message from CLIENTS.
	It collects these message parts from different clients and must assemble
		them in order, based on the lSeqNum.
	The UDP server keeps a running list of all clients that have sent a message
		to it and will broadcast the final composite message to all clients when
		commanded.
	The UDP server is able to receive data from clients without blocking the main application thread.
	The program is able respond to user input while handling socket communications at the same time.

*/

#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <thread>
#include <list>
#include <string>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <iostream>
#include <vector>
#include <atomic>
#include <sstream>
#include <map>
#include <string.h>


using namespace std;


#ifdef _WIN32
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */

typedef int SOCKET;
#endif

// The messages being sent back and forth use the following packet structure:
struct udpMessage
{
	unsigned short nVersion;		// htons()
	unsigned short nType;			// htons()
	unsigned short nMsgLen;			// htons()
	unsigned long lSeqNum;			// htons()
	char chMsg[1000];
};

class ECE_UDPSocket
{
public:
	ECE_UDPSocket() = delete;		        // remove default constructor
	~ECE_UDPSocket();						// destructor
	ECE_UDPSocket(unsigned short usPort);	// constructor

	bool getNextMessage(udpMessage& msg);			// get next message in the list of messages (true) until there are no more left (false)
	void sendMessage(const std::string& strTo, unsigned short usPortNum, const udpMessage& msg);	// for the client to send a message to the server

	void clearCompositeMsg();						// clear the composite message
	void displayCompositeMsg();						// display the composite message
	void addSource(const sockaddr_in &from);		// Add source to the list

	// these seem like the same, investigate difference
	//void processMessage(const udpMessage &inMsg);	// Add new udpMessage to the list
	bool storeMessage(udpMessage& msg);				// store the new udpMessage into the list of messages


	unsigned int getMessageLen();					// get the length of the composite message

	// these seem like the same, investigate the difference
	void sendCompositeMsg();						// Send the composite message out to everybody
	void broadcastMessage(udpMessage &inMsg);		// broadcast the composite message to all the clients


	void buildCompositeMessage(udpMessage&msg, bool trim = false);		// build the composite message


	int sockInit(void);				// Initialize the socket
	int sockClose();				// Close down the socket
	int sockQuit(void);				// Quit the sockets
	void error(const char* msg);	// Error in sockets


	// not used / scrapped functions
	//void receiveClientCommands(std::string newCommand);
	//void receiveServerCommands(std::string newCommand);
	//void bindToSocket();
	//void joinThreads();


	bool isServ = false;					// know whether the socket using this class is a server or client
	bool isServer() { return isServ; };		// function called to retrieve the boolean variable that tells whether server or client
	int sockfd;								// socket id
//private:
	unsigned short m_usPort;
	std::thread m_recvThread;               // each instance of class will have a thread associated with it that receives the messages

	std::list<udpMessage> listSortedMsgs;           // list of the udpMessage packet data structures
	std::list<sockaddr_in> listSources;    // list of sources (for the server-side), list of all the people that have sent server messages        
	
	std::mutex mMutex;                     // 

											// m_mutex to protect list of inbound messages 
											// worker thread sitting waiting to receive messages - work thread pushing messages into m_msgs
											// then main thread that can also access the messages - main thread can also request from m_msgs


};

