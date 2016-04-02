#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define S_PORT_NUM 5558
#define S_IP_ADDR  "127.0.0.1"
#define HELLO "ic"


void xsend(int socket, std::string mesg, std::string error)
{	
	char send_buffer[128];
	strncpy(send_buffer, mesg.c_str(), sizeof(send_buffer));

	if(send(socket, send_buffer, strlen(send_buffer), 0) == -1)
		std::cout<<"Send error : "<<error<<"\n";

	return;
}
void xerror(std::string x)
{
	std::cout<<x<<std::endl;
	std::exit(1);
}

int main()
{	
	int client_socket, recv_bytes, yes = 1;
	std::string inp, request;
	unsigned int sin_size = sizeof(sockaddr);
	char recv_buffer[128], send_buffer[128];
	sockaddr_in s_socket_adr;

	//! initializing client socket
	s_socket_adr.sin_family = AF_INET;
	s_socket_adr.sin_port 	= htons(S_PORT_NUM);		// server port
	inet_aton(S_IP_ADDR, &(s_socket_adr.sin_addr));		// server address
	memset(&(s_socket_adr.sin_zero), '\0', 8);

	client_socket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (client_socket == 0)
		xerror("Could not create socket descriptor!");
	
	if (connect(client_socket, (sockaddr*) &s_socket_adr, sin_size) == -1)
		xerror("Could not connect to server!");
	
	xsend(client_socket, HELLO, "Identity could not be established");

	request = "rabFZSxKKdq5s6:111:3";
	xsend(client_socket, request, "Hash");
	std::cout<<"Sent cracking request!\n";

	recv_bytes = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);

	if(recv_bytes <= 0)
	{
		if (recv_bytes < 0)
			std::cout<<"Error receiving data!\n";
		else if (recv_bytes == 0)
			std::cout<<"Server disconnected!\n";

		close(client_socket);
		return 1;
	}
	else
		std::cout<<"Cracked! Password : "<<recv_buffer<<"\n";

	return 0;
}