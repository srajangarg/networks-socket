#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_NUM 5557
#define IP_ADDR  "192.168.0.14"
#define BACKLOG 10

int main()
{	
	int main_socket, accepted_socket;
	int max_socket, recv_bytes, yes = 1;
	char recv_buffer[128];
	unsigned int sin_size = sizeof(sockaddr);
	fd_set master, reads;
	std::set<int> active_clients, active_workers;
	std::set<int>::iterator ite;

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

		for(int curr_socket = 0; curr_socket <= max_socket; ++curr_socket)
		{	
			if (not FD_ISSET(curr_socket, &reads))
				continue;

			if (curr_socket == main_socket)	// handle new connections	
			{
				accepted_socket = accept(main_socket, (sockaddr*) &socket_adr, &sin_size);

				if (accepted_socket == -1)
					std::cout<<"Could not accept new socket!\n";

				FD_SET(accepted_socket, &master);
				std::cout<<"A remote host connected!\n";

				if (accepted_socket > max_socket)
					max_socket = accepted_socket;

			}
			else							// old connections, new data
			{
				recv_bytes = recv(curr_socket, recv_buffer, sizeof(recv_buffer), 0);

				if(recv_bytes <= 0)			// errors + disconnects
				{
					if (recv_bytes < 0)
					{
						std::cout<<"Error receiving data!\n";
					}
					else if (recv_bytes == 0)
					{	
						ite = active_workers.find(curr_socket);
						if (ite != active_workers.end())
						{
							active_workers.erase(ite);
							std::cout<<"A worker disconnected!\n";
						}
						else
						{
							ite = active_clients.find(curr_socket);
							active_clients.erase(ite);
							std::cout<<"A client disconnected!\n";
						}
					}

					close(curr_socket);
					FD_CLR(curr_socket, &master);
				}
				else						// actual server work
				{
					// recv_buffer has the recieved data, from the curr_socket
					switch (recv_buffer[0])
					{
						case 'i':

							if (recv_buffer[1] == 'c')
							{
								active_clients.insert(curr_socket);
								std::cout<<"A new client connected!\n";
							}
							else if (recv_buffer[1] == 'w')
							{
								active_workers.insert(curr_socket);
								std::cout<<"A new worker connected!\n";
							}
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
