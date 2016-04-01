#include <iostream>
#include <cstdio>
#include <crypt.h>
#include <string>
#include <pthread.h>

using namespace std;

static volatile bool thread_halt = false;

void *findPass(void* arg)
{
	// hash:flag:limit1:limit2 (both inclusive)
	size_t separator;
	string work,hash,salt,flag,limit1,limit2;

	work = *reinterpret_cast<string*>(arg);

	salt = work.substr(0,2);
	separator = work.find(':');
	hash = work.substr(0,separator);
	flag = work.substr(separator+1,3);

	bool numer = (flag[2]=='1');
	bool upper = (flag[1]=='1');
	bool lower = (flag[0]=='1');
	
	work = work.substr(separator+5);

	separator = work.find(':');
	limit1 = work.substr(0,separator);
	limit2 = work.substr(separator+1);

	string password = limit1;
	cout<<"Finding Password"<<endl;

	while(password <= limit2)
	{
		if (hash.compare(crypt(password.c_str(),salt.c_str())) == 0)
		{
			cout<<"Found:"<<password<<endl;
			return NULL;
		}
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
			cout<<"Halted"<<endl;
			return NULL;
		}
	}
	cout<<"Failed"<<endl;
	return NULL;
}

int main()
{
	pthread_t process;
	string s;
	while(true)
	{
		cin>>s;
		if (s=="Halt")
		{
			thread_halt = true;
		}
		else
		{
			thread_halt = false;
			pthread_create(&process, NULL, &findPass, &s);
		}
	}	
	return 0;
}