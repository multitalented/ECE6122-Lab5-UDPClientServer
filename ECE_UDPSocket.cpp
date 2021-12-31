/*
Author: Deanna Gierzak
Class: ECE6122
Last Date Modified: 11/12/2020
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
#include "ECE_UDPSocket.h"
using namespace std;

bool thRunning = true;

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: RECEIVE MESSAGES ON THE SOCKET
//	INPUTS: ECE_UDPSocket* udpSock by pointer, the socket to receive messages on
//  OUTPUT: NONE
//****************************************************************************
// Used by a detached thread to continuously wait for and read messages on
//		the socket, while it does other functionality.
// Initialized upon creation of the ECE_UDPSocket object.
// Note: consulted with classmates for help
void receiveSocketMsgs(ECE_UDPSocket*udpSock)
{
	// Loop that waits on incoming messages
	udpMessage newMsg;
	while (thRunning)
	{
		if (udpSock->getNextMessage(newMsg))
		{
			// handle specific server functions if applicable
			if (udpSock->isServer())
			{
				switch (newMsg.nType)
				{
					case 0: 
					{
						udpSock->clearCompositeMsg();
					} break;
					case 1: 
					{
						udpSock->clearCompositeMsg();
						udpSock->storeMessage(newMsg);
					} break;
					case 2: 
					{
						udpSock->storeMessage(newMsg);
					} break;
					case 3: 
					{
						udpMessage compMsg;

						//udpSock->sendCompositeMsg();
						udpSock->buildCompositeMessage(compMsg);
						udpSock->broadcastMessage(compMsg);
						udpSock->clearCompositeMsg();
					} break;
				}
				// if the composite message is greater than 1000 bytes, 
				if (udpSock->getMessageLen() > 1000)
				{
					// then output composite message of the first 1000 and create new sequence with remaining bytes 
					// (buildCompositeMessage takes care of this with the bool input of true to indicate the composite message must be trimmed)
					udpMessage compMsg;
					udpSock->buildCompositeMessage(compMsg, true);
					udpSock->broadcastMessage(compMsg);
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: GET THE LENGTH OF THE COMPOSITE MESSAGE
//	INPUTS: NONE
//  OUTPUT: unsigned int totalLength: the number of characters of the composite message
//****************************************************************************
// Add the nMsgLen of every updMessage in the list of sorted messages to get
//		the total length of the composite message
unsigned int ECE_UDPSocket::getMessageLen()
{
	// Loop through the sorted list of updMessages
	int totalLength = 0;
	for (auto message : listSortedMsgs)
	{
		totalLength += message.nMsgLen;		// add their .nMsgLen together to get the total composite message length
	}
	return totalLength;
}
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: BROADCAST THE MESSAGE OUT TO CLIENTS
//	INPUTS: updMessage inMsg: the message to be sent out
//  OUTPUT: NONE
//****************************************************************************
// Send the message to all in the list of sources
void ECE_UDPSocket::broadcastMessage(udpMessage &inMsg)
{
	// loop through the list of collected clients in the list of sources and 
	// send (broadcast) the specified inMsg to each of them
	int n;
	for (auto source : listSources)
	{
		n = sendto(sockfd, (char*)&inMsg, sizeof(udpMessage), 0, (struct sockaddr*)&source, sizeof(struct sockaddr_in));
	}
}

// Constructor that takes in a value to determine whether it is a client or server
ECE_UDPSocket::ECE_UDPSocket(unsigned short usPort) :m_usPort(usPort)
{
	struct sockaddr_in servAddr;                         // Address structures (predefined)
	servAddr.sin_family = AF_INET;						// Initialize the serv_addr in the family communication domain of AF_INET for IPv4
	servAddr.sin_addr.s_addr = INADDR_ANY;				// Can listen from any address
	servAddr.sin_port = htons(usPort);					// Convert port number from host to network
	sockInit();									// initialize the socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);	// create socket (domain, type, protocol), type SOCK_DGRAM is used for UDP, 0 is for IP

	// If the input port number value is not zero
	if (usPort != 0)
	{
		// Then this is a server
		isServ = true;
		// So, bind the socket to the specified port number
		if (::bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		{
			error("ERROR on binding");
		}
	}
	//printf("Waiting on messages...\n");

	// Creating the thread for our class, and giving it a pointer to ourself
	//      so that it has access to those messages
	// Start thread that waits for messages 
	// thread calls on receiveSocketMsgs()
	m_recvThread = std::thread(receiveSocketMsgs, this);	// second argument is the input to receiveSocketMsgs() which is this ECE_UDPSocket object
	m_recvThread.detach();
};

// Deconstructor is called to close the socket and stop the thread once
//		the user quits the program
ECE_UDPSocket::~ECE_UDPSocket()
{
	thRunning = false;
	if (sockfd > 0)
		sockClose();
}
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: SEND A MESSAGE
//	INPUTS: string strTo: the host name / IP Address to send to
//			unsigned short usPortNum: the Port Number to send to
//			const udpMessage inMsg by reference: the UDP data struct to send
//  OUTPUT: NONE
//****************************************************************************
// Send a UDP data structure to a given IP Address of specified port number
void ECE_UDPSocket::sendMessage(const std::string& strTo, unsigned short usPortNum, const udpMessage& inMsg)
{
	struct hostent* server;						// initialize the server variable
	struct sockaddr_in servAddr;				// initialize the socket address type variable for server address 
	server = gethostbyname(strTo.c_str());		// get the server/host name from strTo input and set server to this

	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host exists\n");	// error if the server value is null (strTo is erroneous input or not found)
	}
	
	//void * memset(void * ptr, int value, size_t num);    // how it actually should be
	memset((char *)&servAddr, 0, sizeof(servAddr));											// zero out server address
	servAddr.sin_family = AF_INET;															// domain family is AF_INET
	memmove((char*)&servAddr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);		// now copy this server's address to serv_addr variable's IP address
	servAddr.sin_port = htons(usPortNum);													// convert the unsigned short port number value we want to send to from host to network and set to the server's port number

	// send the inMsg we want to send to the specified server address we declared with strTo and usPortNum
	int n = sendto(sockfd, (char*)&inMsg, sizeof(udpMessage), 0, (struct sockaddr*)&servAddr, sizeof(servAddr));	// servAddr variable gets popoulated by this command

	if (n < 0)
		error("ERROR writing to socket");		// error if there was a problem in sending
};
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: BUILD THE COMPOSITE MESSAGE
//	INPUTS: udpMessage inMsg: the message struct to build into a composite message
//			bool trim: boolean to tell whether message is larger than 1000 bytes
//  OUTPUT: NONE
//****************************************************************************
// Put the sorted messages together and create a new composite message
//		If it is larger than 1000 bytes, then store the overflow part into
//		the beginning of the new list of messages after creating the 1000-max
//		composite message and emptying the list
// Note: consulted with classmates for help
void ECE_UDPSocket::buildCompositeMessage(udpMessage &inMsg, bool trim)
{
	// Setup the message structure for a composite message
	inMsg.lSeqNum = 0;					// its sequence number is irrelevant, set = 0
	inMsg.nVersion = 1;					// its version number is 1
	inMsg.nType = 4;					// its type number = 4 to represent composite message
										// void * memset ( void * ptr, int value, size_t num );
	memset(inMsg.chMsg, 0, 1000);		// set 1000 bytes of memory pointed to by the chMsg of inMsg data struct to 0 (e.g. iniitalizing it to empty message) 
	string compMsg = "";				// initalize an empty string to nothing to build the composite message into
	mMutex.lock();						// lock the mutex for protection of the segment of code

	// Build the composite message from the list of messages into the locally defined empty string
	for (udpMessage m : listSortedMsgs)		// For each udpMessage in the list of collected & sorted udpMessages
	{
		compMsg += string(m.chMsg);			// add the chMsg of each udpMessage to the total composite message
	}
	mMutex.unlock();					// unlock the mutex 

	// If trimming is required (i.e. msgsize > 1000), then we create a new overflow updMessage 
	//		to store at the start of the new composite message
	if (trim == true)
	{
		clearCompositeMsg();					// First clear the composite message
		udpMessage overflowMsg;					// then, initialize a new message starting at lSeqNum = 0
		overflowMsg.lSeqNum = 0;				// its sequence number = 0 to keep it at the start of the new composite message when new messages come in
		overflowMsg.nVersion = 1;				// its version number = 1 to keep it valid
		overflowMsg.nType = 2;					// its type number = 2 (meaning it is added to composite message)
		memset(overflowMsg.chMsg, 0, 1000);		// set 1000 bytes of memory pointed to by the chMsg of inMsg data struct to 0 (e.g. iniitalizing it to empty message) 

		// Move all bytes after 1000 and over to new sequence (all of the overflow of the compMsg composite message we just built above)
																			// string substr(size_t pos = 0, size_t len = npos) const;
		string newMsgStr = compMsg.substr(1000);							// all after 1000 in the string are part of the substring and copied to newMsg string
		memcpy(overflowMsg.chMsg, newMsgStr.c_str(), newMsgStr.size());		// now copy over the entire overflow message string to the char type chMsg of the new udpMessage overflowMsg
		overflowMsg.nMsgLen = (unsigned short)newMsgStr.size();								// and now set the nMsgLen of this udpMessage to the size of the new string 
		compMsg.erase(compMsg.begin() + 1000, compMsg.end());				// and erase the overflow portion of the compMsg string we built above, starting at 1000 and ending at the end of the string

		storeMessage(overflowMsg);		// now we can store this overflow message into the composite message since we just cleared it and it will always be at the start of the new composite message with its seq num = 0 
	}
	// copy the composite message string to message buffer
	memcpy(inMsg.chMsg, compMsg.c_str(), compMsg.size());		// store the current composite message in the chMsg of the inMsg udpMessage
	inMsg.nMsgLen = (unsigned short)compMsg.size();								// and set the nMsgLen to the size of the composite message string
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: SEND A COMPOSITE MESSAGE
//	INPUTS: NONE
//  OUTPUT: NONE
//****************************************************************************
// This builds and broadcasts the current composite message to all clients
void ECE_UDPSocket::sendCompositeMsg()
{
	// only server will use this to send out a composite message to all of the clients
	udpMessage compositeMsg;				// initialize an empty composite message udpMessage object
	buildCompositeMessage(compositeMsg);	// call function to build the composite message into this empty object from the list of messages member variable
	broadcastMessage(compositeMsg);			// then call function to broadcast the message out to all the clients in the list of sources member variable
};
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: CLEAR THE COMPOSITE MESSAGE
//	INPUTS: NONE
//  OUTPUT: NONE
//****************************************************************************
// Protect and clear the list of sorted messages 
//		(which make up a composite message)
void ECE_UDPSocket::clearCompositeMsg()
{
	mMutex.lock();
	listSortedMsgs.clear();
	mMutex.unlock();
}
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: DISPLAY THE COMPOSITE MESSAGE
//	INPUTS: NONE
//  OUTPUT: NONE
//****************************************************************************
// Loop through the list of sorted messages to display them to the console
void ECE_UDPSocket::displayCompositeMsg()
{
	cout << "Composite Msg: ";
	// iterate through the list of udpmessages to display;
	for (auto entry : this->listSortedMsgs)
	{
		cout << entry.chMsg;
	}
	cout << endl;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: STORE THE MESSAGE
//	INPUTS: updMessage& inMsg: the message to be stored
//  OUTPUT: bool: true if the message is stored, false otherwise
//****************************************************************************
// Store the message in the proper place in the list of sorted messages,
//		unless the sequence number is already taken, then do not store
// Note: consulted with classmates for help
bool ECE_UDPSocket::storeMessage(udpMessage& inMsg)
{
	auto it = listSortedMsgs.begin();	// iterator 
	for (auto m : listSortedMsgs)
	{
		// Check for correct location in the list to sort properly upon receipt, by checking sequence numbers
		if (inMsg.lSeqNum < m.lSeqNum)
		{
			mMutex.lock();
			listSortedMsgs.insert(it, inMsg);
			mMutex.unlock();
			return true;		// return true, message is stored
		}
		// If the sequence number used for this inMsg already exists, don't store this new message
		else if (inMsg.lSeqNum == m.lSeqNum)
		{
			return false;		// return false and don't store the message
		}
		++it;	// increment to align with current m message in for loop
	}
	// if this is reached, it means sequence number was larger than all others
	// so insert new message at the end
	mMutex.lock();
	listSortedMsgs.insert(it, inMsg);
	mMutex.unlock();
	return true;				// return true, message is stored
}
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: ADD NEW SOURCE TO LIST OF SENDERS
//	INPUTS: const sockaddr_in from: the address of the sender
//  OUTPUT: NONE
//****************************************************************************
// Add a sender to the list if new (not already in list)
void ECE_UDPSocket::addSource(const sockaddr_in& from)
{
	// order of this doesn't really matter I think, 
	//	it is just for keeping track of who to send the composite message to 
	bool bIsPresent = false;

	// m_lstSources.unique  // could also use this somehow?
	// Iterate through list check if the new source is already present
	for (auto source : listSources)
	{
		// If the from sockaddr and the source sockaddr are equal, and their port numbers are ALSO equal
		if (from.sin_addr.s_addr == source.sin_addr.s_addr && from.sin_port == source.sin_port)
		{
			bIsPresent = true;			// then this new sender is already present in the list of sources
			break;
		}
	}
	// if the source is not present
	if (!bIsPresent)
	{
		listSources.push_back(from);		// add new source to list
	}

}
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: GET THE NEXT MESSAGE
//	INPUTS: updMessage msg: struct to be filled by the message to be received
//  OUTPUT: bool: true if received and valid (version=1), otherwise false
//****************************************************************************
// Read a new message that has just been received (from other source, not user)
// Note: consulted with classmates for help
bool ECE_UDPSocket::getNextMessage(udpMessage& msg)
{
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	memset(msg.chMsg, 0, 1000);				// set char chMsg variable of struct to nothing

	// Read the message into the updMessage msg struct
	int n = recvfrom(sockfd, (char*)&msg, sizeof(udpMessage), 0, (struct sockaddr *)&from, &fromlen);
	
	// Return false if not the right version (only 1 accepted)
	if (msg.nVersion != 1)
		return false;

	// Store the source information if a server
	if (isServer())
	{
		addSource(from);
	}

	// Return false if there is a receive error
	if (n < 0)
	{
		return false;
	}
	msg.chMsg[msg.nMsgLen] = 0;		// null end the message sequence
	printf("\nReceived Msg Type: %d, Seq: %d, Msg: %s\nPlease enter command: ", msg.nType, msg.lSeqNum, msg.chMsg);
	//printf("\nReceived Msg Type: %d, Seq: %d, Msg: %s", msg.nType, msg.lSeqNum, msg.chMsg);
	return true;
}


// Cross-platform function to initialize socket
int ECE_UDPSocket::sockInit(void)
{
#ifdef _WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
	return 0;
#endif
}

// Cross-platform function to quit socket
int ECE_UDPSocket::sockQuit(void)
{
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

// Cross-platform function to close socket
int ECE_UDPSocket::sockClose()
{

	int status = 0;

#ifdef _WIN32
	status = shutdown(sockfd, SD_BOTH);
	if (status == 0)
	{
		status = closesocket(sockfd);
	}
#else
	status = shutdown(sockfd, SHUT_RDWR);
	if (status == 0)
	{
		status = close(sockfd);
	}
#endif

	// Set to show socket closed
	sockfd = -1;
	// Wait for thread to shut down.
	return status;

}

// Output error message and exit
void ECE_UDPSocket::error(const char* msg)
{
	perror(msg);
	// Make sure any open sockets are closed before calling exit

	exit(1);
}






///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//         scrapped code and stuff down below


/*
void ECE_UDPSocket::processMessage(const udpMessage &inMsg)
{
// Determine what to do with the message
// If we need to add the message to the list then
// Iterate through list and insert msg based on seq num
// do some kind of sorting based on lseqnum?
// to insert udpMessage at the correct place in the list
// based on lSeqNum of udpMessage list m_lstMsgs
// insert at appropriate place

udpMessage tmpMsg;
std::list<udpMessage>::iterator it;

for (it = listSortedMsgs.begin(); it != listSortedMsgs.end(); ++it)
{
tmpMsg = *it;

if (inMsg.lSeqNum < tmpMsg.lSeqNum)
{
//m_lstMsgs.insert(inMsg);
--it;
listSortedMsgs.emplace(it, inMsg);
break;
}
}

}
*/



/*
// Function called by thread
//used getters or setters for the port number
// this function won't have access to the private member variables
//  so I could make it a friend function or use getters and setters
//  or just make them public
// so that it can push them onto the list we created

void receiveSocketMsgsOLD(ECE_UDPSocket* pUDpSocket)
{
	// called on by each thread
	// Loop that waits on incoming messages

	udpMessage inMsg;
	sockaddr_in from;
	socklen_t fromlen{ sizeof(struct sockaddr_in) };
	int n;

	do
	{
		//this->n = recvfrom(this->sockfd, this->.buffer, 1023, 0, this->from, this->fromlen);
		//pUDpSocket->n = recvfrom(pUDpSocket->sockfd, pUDpSocket->buffer, 1023, 0, &pUDpSocket->from, &pUDpSocket->fromlen);

		n = recvfrom(pUDpSocket->sockfd, (char*)&inMsg, sizeof(udpMessage), 0, (struct sockaddr*)&from, &fromlen);
		//n = recvfrom(pUDpSocket->m_sockfd, (char*)&inMsg, sizeof(inMsg), 0, (struct sockaddr*)&from, &fromlen);

		if (n < 0)
		{
			break;
		}
		else
		{
			if (inMsg.nVersion == 1)
			{
				pUDpSocket->processMessage(inMsg);
				pUDpSocket->addSource(from);
			}
		}

		// m_lstMsgs


		
		//if (pUDpSocket->n < 0)
		//error("recvfrom");
		//pUDpSocket->buffer[pUDpSocket->n] = 0;		// null terminate
		//printf("Received a datagram: %s", pUDpSocket->buffer);

		//printf("Received Msg Type: %s", pUDpSocket->buffer);
		//printf(", Seq: %s", pUDpSocket->buffer);
		//printf(", Msg: %s", pUDpSocket->buffer);
	} while (true); //(!pUDpSocket->quit);

					// Loop that waits on incoming messages
					// Insert message into list based on sequence number
};
*/


/*
Would it be correct to say that the only messages received by the client application
are composite messages from the server, and therefore the client will only receive messages of nType=4?
Additionally all composite messages should be printed out as:
Received Msg Type: 4, Seq #, Msg: composite message received
*/
/*
To get the received message to show up without hitting enter,
you need to flush the cout buffer with std::cout << std::flush.
To have the prompt show up again without having to hit enter,
just print it again after printing the received message (before the flush).
*/


/*
void ECE_UDPSocket::clearCompositeMsg()
{
m_lstMsgs.clear();
}
*/




/*
Notes from Piazza post @547

ECE_UDPSocket creates a thing that at ALL times is able to receive messages.

When it receives a message it pops it into a special container that you can access whenever you'd like.
In addition to storing the message it saves who sent that message (e.g. so a pair of values? Map?)
You can also make this thing send messages to wherever you want whenever you want.

// packet data structure:
struct udpMessage
{
unsigned short nVersion;
unsigned short nType;
unsigned short nMsgLen;
unsigned long lSeqNum;
char chMsg[1000];
};

Using the special message container std::list<udpMessage> m_msgs we can store the list of packets and
use C++ lists' built-in method to sort:

typedef std::pair<int, int> ipair;
std::list<ipair> thelist;

thelist.sort([](const ipair & a, const ipair & b) {return a.first < b.first; });
????

Sort messages based on sequence number as they come in

*/
// translate the command input with a switch-case statement.. but here, or in class?

/*
void ECE_UDPSocket::receiveServerCommands(std::string newCommand)
{

}
void ECE_UDPSocket::receiveClientCommands(std::string newCommand)
{
bool bIsVersionCommand = false;
bool bIsTypeCommand = false;
bool bIsQuitCommand = false;
int x = 1;

udpMessage newUDPMessage;

n = newCommand.length();

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
//udpSocket.quit = true;
this->quit = true;
sockClose();
//sockClose(udpSocket.sockfd);
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
};
*/

/*
void ECE_UDPSocket::bindToSocket()
{
//::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0
//m_usPort
};

void ECE_UDPSocket::joinThreads()
{

}*/



/*
bool ECE_UDPSocket::getNextMessage(udpMessage& msg)
{
// Get next message from list
// return false if there no more messages
//if(m_lstMsgs)    // if the next message in the list of m_lstMsgs is not NULL?

//Check if end of list
if (m_lstMsgs.empty())
return false;

//If not empty, set msg to front of the list
msg = m_lstMsgs.front();
m_lstMsgs.pop_front();

return true;

};*/


/*
//	n = sendto(sockfd, "Got your message\n", 17, 0, (struct sockaddr *)&from, fromlen);
// from sendCompositeMessage
while (getNextMessage(tmpMessage))
{
// Combine them
//totalMess
//thMutex.lock();
// build message in string


/*
for (udpMessage m : messages)
{
totalMessage += string(m.chMsg);
}
//thMutex.unlock();*/
//}*/

// Just for the server side 
// Loop through m_lstSources
// sendMessage(addrTo, compositeMsg);
/*
// If input (command) is zero, send current composite message to all clients and clear out composite
//if (atoi(input) == 0)
{
udpMessage newMessage;		// Start new message
newMessage.nType = '1';		// Set type = 1
std::string compositeMessage;		// Start new string

// Get composite messages from member variable storing sorted messages and concatenate them
for (auto entry : sortedMessages)
{
compositeMessage = compositeMessage.append(entry.second);
}

newMessage.lSeqNum = htonl(j);		// Set seq number to iterator

m_mutex.lock();
j++;
m_mutex.unlock();

std::copy(compositeMessage.begin(), compositeMessage.end(), newMessage.chMsg);	// Copy composite to char array

newMessage.chMsg[compositeMessage.length()] = 0;							// Null terminate

for (int i = 0; i < actives.size(); i++)							// Loop for sending to all clients
{
n = sendto(sockfd, (char*)&newMessage, sizeof(udpMessage), 0, (struct sockaddr*)&actives[i].first, actives[i].second);
}
//sorted_msg.clear();
}
//std::cout << "Composite Msg: ";
for (auto entry : this->sortedMessages)
{
std::cout << entry.second;
}*/

// from processMessage 
//unsigned long thisSeqNum;
//thisSeqNum = inMsg.lSeqNum;
/*
for (int ii = 0; ii < m_lstMsgs.size(); ++ii)
{
//m_lstMsgs
//if (thisSeqNum > m_lstMsgs.)
}*/