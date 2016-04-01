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
	int main_socket, bind_s, listen_s, accepted_socket;
	int max_socket, recv_bytes, yes = 1;
	char buffer[128];
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
		std::cout<<"Could not create socket descriptor!"<<std::endl;
		return 1;
	}

	if (bind(main_socket, (sockaddr*) &socket_adr, sin_size) == -1)
	{
		std::cout<<"Could not bind to socket!"<<std::endl;
		return 1;
	}

	if (listen(main_socket, BACKLOG) == -1)
	{
		std::cout<<"Could not start listening!"<<std::endl;
		return 1;
	}

	FD_SET(main_socket, &master);
	max_socket = main_socket;

	while (true)
	{
		reads = master;
		if (select(max_socket + 1, &reads, NULL, NULL, NULL) == -1)
			std::cout<<"Select failed!"<<std::endl;

		for(int curr_socket = 0; curr_socket <= max_socket; ++curr_socket)
		{	
			if (not FD_ISSET(curr_socket, &reads))
				continue;

			if (curr_socket == main_socket)	// handle new connections	
			{
				accepted_socket = accept(main_socket, (sockaddr*) &socket_adr, &sin_size);

				if (accepted_socket == -1)
					std::cout<<"Could not accept new socket!"<<std::endl;

				FD_SET(accepted_socket, &master);
				std::cout<<"A remote host connected!"<<std::endl;

				if (accepted_socket > max_socket)
					max_socket = accepted_socket;

			}
			else							// old connections, new data
			{
				recv_bytes = recv(curr_socket, buffer, sizeof(buffer), 0);

				if(recv_bytes <= 0)			// errors + disconnects
				{
					if (recv_bytes < 0)
					{
						std::cout<<"Error receiving data!"<<std::endl;
					}
					else if (recv_bytes == 0)
					{	
						ite = active_workers.find(curr_socket);
						if (ite != active_workers.end())
						{
							active_workers.erase(ite);
							std::cout<<"A worker disconnected!"<<std::endl;
						}
						else
						{
							ite = active_clients.find(curr_socket);
							active_clients.erase(ite);
							std::cout<<"A client disconnected!"<<std::endl;
						}
					}

					close(curr_socket);
					FD_CLR(curr_socket, &master);
				}
				else						// actual server work
				{
					// buffer has the recieved data, from the curr_socket
					switch (buffer[0])
					{
						case 'i':

							if (buffer[1] == 'c')
							{
								active_clients.insert(curr_socket);
								std::cout<<"A new client connected!"<<std::endl;
							}
							else//buffer[1] == 'w'
							{
								active_workers.insert(curr_socket);
								std::cout<<"A new worker connected!"<<std::endl;
							}
							break;
					}

					// if(send(curr_socket, buffer, strlen(buffer), 0) == -1)
					// 	std::cout<<"Could not send data to client!"<<std::endl;
				}

			}
		} 
	}

	return 0;
}
