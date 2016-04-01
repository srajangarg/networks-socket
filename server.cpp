#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_NUM 5557
#define IP_ADDR  "192.168.0.14"
#define BACKLOG 10
#define HALT "h"

void xsend(int socket, std::string mesg, std::string error)
{	
	char send_buffer[128];
	strncpy(send_buffer, mesg.c_str(), sizeof(send_buffer));
	if(send(socket, send_buffer, strlen(send_buffer), 0) == -1)
		std::cout<<"Send error : "<<error<<"\n";

	return;
}

int main()
{	
	int main_socket, accepted_socket;
	int max_socket, recv_bytes, yes = 1;
	char recv_buffer[128];
	unsigned int sin_size = sizeof(sockaddr);
	fd_set master, reads;
	std::string pass;
	std::set<int> active_clients, idle_workers;
	std::map<int, std::set<int> > active_workers;

	sockaddr_in socket_adr;
	//! initializing client socket
	socket_adr.sin_family = AF_INET;
	//! port
	socket_adr.sin_port = htons(PORT_NUM);					// custom port
	//! in_addr
	inet_aton(IP_ADDR, &(socket_adr.sin_addr));				// custom address
	// socket_adr.sin_addr.s_addr = INADDR_ANY;				// current address
	memset(&(socket_adr.sin_zero), '\0', 8);


	main_socket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (main_socket == 0)
	{
		std::cout<<"Could not create socket descriptor!\n";
		return 1;
	}

	if (bind(main_socket, (sockaddr*) &socket_adr, sin_size) == -1)
	{
		std::cout<<"Could not bind to socket!\n";
		return 1;
	}

	if (listen(main_socket, BACKLOG) == -1)
	{
		std::cout<<"Could not start listening!\n";
		return 1;
	}

	FD_SET(main_socket, &master);
	max_socket = main_socket;

	while (true)
	{
		reads = master;
		if (select(max_socket + 1, &reads, NULL, NULL, NULL) == -1)
			std::cout<<"Select failed!\n";

		// iterate over all sockets
		for(int curr_socket = 0; curr_socket <= max_socket; ++curr_socket)
		{		
			// if a socket has written to the server, only then move ahead
			if (not FD_ISSET(curr_socket, &reads))
				continue;

			// handle new connections
			if (curr_socket == main_socket)	
			{
				accepted_socket = accept(main_socket, (sockaddr*) &socket_adr, &sin_size);

				if (accepted_socket == -1)
					std::cout<<"Could not accept new socket!\n";

				FD_SET(accepted_socket, &master);
				std::cout<<"A remote host connected!\n";

				if (accepted_socket > max_socket)
					max_socket = accepted_socket;

			}
			// old connections, new data
			else
			{
				recv_bytes = recv(curr_socket, recv_buffer, sizeof(recv_buffer), 0);

				// errors + disconnects
				if(recv_bytes <= 0)
				{
					if (recv_bytes < 0)
					{
						std::cout<<"Error receiving data!\n";
					}
					else if (recv_bytes == 0)
					{	
						if (idle_workers.find(curr_socket) != idle_workers.end())
						{
							idle_workers.erase(idle_workers.find(curr_socket));
							std::cout<<"An idle worker disconnected!\n";
						}
						else if (active_clients.find(curr_socket) != active_clients.end())
						{
							active_clients.erase(active_clients.find(curr_socket));
							std::cout<<"A client disconnected!\n";
						}
						else
						{
							std::cout<<"Error : A non-idle work disconnected!\n";
						}
					}

					close(curr_socket);
					FD_CLR(curr_socket, &master);
				}
				// actual server work
				else
				{
					// recv_buffer has the recieved data, from the curr_socket
					switch (recv_buffer[0])
					{
						// it's an 'i'ntroducing message!
						case 'i':	

							if (recv_buffer[1] == 'c')
							{
								active_clients.insert(curr_socket);
								std::cout<<"A new client connected!\n";
							}
							else if (recv_buffer[1] == 'w')
							{
								idle_workers.insert(curr_socket);
								std::cout<<"A new worker connected!\n";
							}
							break;

						// a 's'uccess recieved from a worker
						case 's':

							// extract password
							pass = std::string(recv_buffer + 1);
							// send all workers a 'h'alt message
							for(auto p : active_workers[curr_socket])
							{
								xsend(p, HALT, "Halt");
								idle_workers.insert(p);
							}
							// send the password to client and remove the client
							xsend(curr_socket, pass, "Password");
							active_clients.erase(curr_socket);
							active_workers.erase(curr_socket);

							break;
					}

					// if(send(curr_socket, recv_buffer, strlen(recv_buffer), 0) == -1)
					// 	std::cout<<"Could not send data to client!\n";
				}

			}
		} 
	}

	return 0;
}
