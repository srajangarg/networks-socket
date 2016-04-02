#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define IP_ADDR  "127.0.0.1"
#define BACKLOG 10
#define HALT "halt"

void xsend(int socket, std::string mesg, std::string error)
{	
	char send_buffer[128];
	strncpy(send_buffer, mesg.c_str(), mesg.length());	
	send_buffer[mesg.length()] = '\0';

	if(send(socket, send_buffer, mesg.length()+1, 0) == -1)
		std::cout<<"Send error : "<<error<<"\n";

	return;
}
void xerror(std::string x)
{
	std::cout<<x<<std::endl;
	std::exit(1);
}

std::set<std::string> dividework(std::string flag, int passLen, int client_socket, std::string hash)
{
	bool numer = (flag[2] == '1');
	bool upper = (flag[1] == '1');
	bool lower = (flag[0] == '1');

	std::set<std::string> limits;
	std::string lim;

	char lastchar,firstchar;

	lastchar = lower? 'z': upper? 'Z':'9';
	firstchar = numer? '0': upper? 'A':'a';

	std::string lim1end(passLen-1, firstchar);
	std::string lim2end(passLen-1, lastchar);

	if(numer)
	{
		for(char c = '0';c<='9';c++)
		{
			lim = std::to_string(client_socket) + ':' + hash + ':' + flag + ':' + c + lim1end + ':' + c + lim2end;
			limits.insert(lim);
		}
	}
	if(upper)
	{
		for(char c = 'A';c<='Z';c++)
		{
			lim = std::to_string(client_socket) + ':' + hash + ':' + flag + ':' + c + lim1end + ':' + c + lim2end;
			limits.insert(lim);
		}
	}	
	if(lower)
	{
		for(char c = 'a';c<='z';c++)
		{
			lim = std::to_string(client_socket) + ':' + hash + ':' + flag + ':' + c + lim1end + ':' + c + lim2end;
			limits.insert(lim);
		}
	}
	return limits;
	// client:hash:flag:limit1:limit2
}

int main(int argc, char* argv[])
{	
	int main_socket, accepted_socket, client_socket;
	int max_socket, recv_bytes, yes = 1, passlen, worker;
	int portNo;
	char recv_buffer[128];
	unsigned int sin_size = sizeof(sockaddr);
	fd_set master, reads;
	std::string pass, hash, flag, work, piece, xpiece, random;
	std::set<int> active_clients, idle_workers;
	std::map<int, std::set<int> > active_workers;
	std::map<int, int> workers_client;
	std::set<std::string> pieces, new_pieces;

	// read arguments
	if(argc < 2)
	{
		std::cerr<<"Syntax : ./server <server-port>\n";
		return 0;
	}

	portNo = std::stoi(argv[1]);

	sockaddr_in socket_adr;
	socket_adr.sin_family = AF_INET;
	socket_adr.sin_port = htons(portNo);					// custom port
	inet_aton(IP_ADDR, &(socket_adr.sin_addr));				// custom address
	// socket_adr.sin_addr.s_addr = INADDR_ANY;				// current address
	memset(&(socket_adr.sin_zero), '\0', 8);


	main_socket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (main_socket == 0)
		xerror("Could not create socket descriptor!");

	if (bind(main_socket, (sockaddr*) &socket_adr, sin_size) == -1)
		xerror("Could not bind to socket!");

	if (listen(main_socket, BACKLOG) == -1)
		xerror("Could not start listening!");

	FD_SET(main_socket, &master);
	max_socket = main_socket;

	while (true)
	{
		reads = master;
		if (select(max_socket + 1, &reads, NULL, NULL, NULL) == -1)
			std::cout<<"Select failed!\n";

		// iterate over all sockets looking for news
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
				// std::cout<<"Someone connected!\n";

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
							idle_workers.erase(curr_socket);
							workers_client.erase(curr_socket);
							std::cout<<"An idle worker disconnected! Worker "<<curr_socket<<"\n";
						}
						else if (active_clients.find(curr_socket) != active_clients.end())
						{
							active_clients.erase(curr_socket);
							std::cout<<"Client "<<curr_socket<<" disconnected!\n";
						}
						else
						{
							std::cout<<"Error : A non-idle worker disconnected! Worker "<<curr_socket<<"\n";
						}
					}

					close(curr_socket);
					FD_CLR(curr_socket, &master);
					// TODO : update max_socket 
				}
				// actual server work
				else
				{
					// recv_buffer has the recieved data, from the curr_socket
					switch (recv_buffer[0])
					{
						// it's an 'i'ntroducing message!
						case 'i':	
						{
							// client's intro
							if (recv_buffer[1] == 'c')
							{
								active_clients.insert(curr_socket);
								std::cout<<"Client "<<curr_socket<<" connected!\n";
							}
							// worker's intro
							else if (recv_buffer[1] == 'w')
							{
								idle_workers.insert(curr_socket);
								std::cout<<"Worker "<<curr_socket<<" connected!\n";
							}

							break;
						}
						// a 's'uccess recieved from a worker
						case 's':
						{	
							std::cout<<"Worker "<<curr_socket<<" succeeded!\n";

							// extract password
							pass = std::string(recv_buffer + 1);
							client_socket = workers_client[curr_socket];
							std::cout<<"The password of Client "<<client_socket<<" is "<<pass<<"\n";

							// add all workers which were working on client's task to idle
							// also send them a halt message
							for(auto p : active_workers[client_socket])
							{
								xsend(p, HALT, "Halt");
								idle_workers.insert(p);
							}

							// remove remaining pieces from pieces set
							for(auto p : pieces)
							{
								int separator = p.find(":");
								if(std::stoi(p.substr(0, separator)) == client_socket)
									pieces.erase(p);
							}

							// send the password to client and remove the client
							xsend(client_socket, pass, "Password");
							active_clients.erase(client_socket);
							active_workers.erase(client_socket);

							FD_CLR(client_socket, &master);
							// TODO : update max_socket
							break;
						}

						// a worker 'f'ailed
						case 'f':
						{
							// just add it to idle list, after removing from active
							client_socket = workers_client[curr_socket];
							active_workers[client_socket].erase(curr_socket);
							idle_workers.insert(curr_socket);

							break;
						}

						// a client 'r'equest
						case 'r':
						{
							std::cout<<"\nClient "<<curr_socket<<" requested to crack!\n";
							work = std::string(recv_buffer + 1);
							int separator = work.find(':');
							int new_separator = work.find(':', separator + 4);

							hash = work.substr(0, separator);
							flag = work.substr(separator + 1, 3);
							passlen = std::stoi(work.substr(new_separator + 1, work.length() - new_separator - 1));

							std::cout<<"Hash: "<<hash<<", Flag: "<<flag<<", PassLen: "<<passlen<<"\n";

							// new work pieces to be added to main work pieces
							new_pieces = dividework(flag, passlen, curr_socket, hash);
							pieces.insert(new_pieces.begin(), new_pieces.end());

							break;
						}

						default :
						{
							random = std::string(recv_buffer);
							std::cout<<"Recieved "<<random<<"\n";
						}
					}
				}

			}
		}

		// server assigns work to idle workers, from pieces
		while(true)
		{	
			if (pieces.empty() or idle_workers.empty())
				break;

			piece = (*pieces.begin());
			worker = (*idle_workers.begin());

			int separator = piece.find(":");
			client_socket = std::stoi(piece.substr(0, separator));
			xpiece = piece.substr(separator+1, piece.length() - separator - 1);

			// send piece to worker, remove from idle workers and remove piece
			xsend(worker, xpiece, "Could not send piece!");
			// who's worker is this?
			workers_client[worker] = client_socket;
			// add worker to active_workers
			active_workers[client_socket].insert(worker);

			std::cout<<"Assigned Client "<<client_socket<<"'s "<<xpiece<<" to Worker "<<worker<<"\n";

			// erase piece and remove idle worker
			idle_workers.erase(worker);
			pieces.erase(piece);
		}

	}

	return 0;
}
