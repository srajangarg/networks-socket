#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <crypt.h>
#include <pthread.h>
#include <netdb.h>

#define FAIL "fw"
#define HALTSUCCESS "hw"
#define HELLO "iw"

static volatile bool thread_halt = false;
static int worker_socket;

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

void *findPass(void* arg)
{
	// Fomat of arg : hash:flag:limit1:limit2 (both inclusive)
	int separator;
	std::string work, hash, salt, flag, limit1, limit2, password;

	work = *reinterpret_cast<std::string*>(arg);

	// first two characters of hash are same as salt
	separator = work.find(':');
	salt = work.substr(0, 2);
	hash = work.substr(0, separator);
	flag = work.substr(separator+1, 3);

	bool numer = (flag[2] =='1');
	bool upper = (flag[1] =='1');
	bool lower = (flag[0] =='1');
	
	work = work.substr(separator+5);
	separator = work.find(':');
	limit1 = work.substr(0, separator);
	limit2 = work.substr(separator + 1);

	password = limit1;

	while(true)
	{
		if (hash.compare(crypt(password.c_str(),salt.c_str())) == 0)
		{	
			// send the password!
			std::cout<<"Cracked! Password : "<<password<<"\n";
			xsend(worker_socket, "s" + password, "Password");
			return NULL;
		}
		if(password == limit2)
			break;

		// go to lexicographically next passwork
		for(int i = password.size()-1; i>=0; i--)
		{
			if (password[i] == '9')
			{
				if (upper)
				{
					password[i] = 'A';
					break;
				}
				else if (lower)
				{
					password[i] = 'a';
					break;
				}
				else
				{
					password[i] = '0';
				}
			}
			else if (password[i]=='Z')
			{
				if (lower)
				{
					password[i] = 'a';
					break;
				}
				else if (numer)
				{
					password[i] = '0';
				}
				else
				{
					password[i] = 'A';
				}
			}
			else if (password[i]=='z')
			{
				if (numer)
					password[i] = '0';
				else if (upper)
					password[i] = 'A';
				else
					password[i] = 'a';
			}
			else
			{
				password[i] = password[i] + 1;
				break;
			}
		}
		if (thread_halt == true)
		{
			// halt the process
			xsend(worker_socket, HALTSUCCESS, "Halt Success");
			return NULL;
		}
	}

	// Password not in limits
	xsend(worker_socket, FAIL, "Fail");

	return NULL;
}

int main(int argc, char* argv[])
{
	int recv_bytes, yes = 1, s_port_num, ip_status;
	unsigned int sin_size = sizeof(sockaddr);
	char recv_buffer[128], send_buffer[128];
	pthread_t process;
	std::string inp, s_ip_addr;
	sockaddr_in s_socket_adr;
	struct hostent *h;

	// read arguments
	if(argc < 3)
	{
		std::cout<<"Syntax : ./worker <server ip> <server-port>\n";
		return 0;
	}

	s_ip_addr  = std::string(argv[1]);
	s_port_num = std::stoi(argv[2]);

	//! initializing worker socket
	s_socket_adr.sin_family = AF_INET;
	s_socket_adr.sin_port 	= htons(s_port_num);									// server port
	ip_status = inet_pton(AF_INET, s_ip_addr.c_str(), &(s_socket_adr.sin_addr));	// server address

	if(ip_status == -1)
	{
		h = gethostbyname(s_ip_addr.c_str());
		// server address
		inet_pton(AF_INET, inet_ntoa(*((in_addr*)h->h_addr)), &(s_socket_adr.sin_addr));
	}

	memset(&(s_socket_adr.sin_zero), '\0', 8);

	// setting the socket to establish connection
	worker_socket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(worker_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (worker_socket == 0)
		xerror("Could not create socket descriptor!");
	
	if (connect(worker_socket, (sockaddr*) &s_socket_adr, sin_size) == -1)
		xerror("Could not connect to server!");

	xsend(worker_socket, HELLO, "Identity could not be established");

	while(true)
	{	
		// recieve information from server
		recv_bytes = recv(worker_socket, recv_buffer, sizeof(recv_buffer), 0);
		if(recv_bytes <= 0)
		{
			if (recv_bytes < 0)
				std::cout<<"Error receiving data!\n";
			else if (recv_bytes == 0)
				std::cout<<"Server disconnected!\n";

			close(worker_socket);
			return 1;
		}
		else
		{
			inp = std::string(recv_buffer);

			if (inp == "halt")
			{	
				std::cout<<"Received halt from server!\n";
				// static variable indicates thread function to halt
				thread_halt = true;
			}
			else
			{	
				thread_halt = false;
				std::cout<<"Trying "<<inp<<"\n";
				pthread_create(&process, NULL, &findPass, &inp);
			}
		}
	}	
	return 0;
}