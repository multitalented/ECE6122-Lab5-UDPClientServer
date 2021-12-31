# ECE6122-Lab5-UDPClientServer

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
