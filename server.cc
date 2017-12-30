#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <map>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <fcntl.h>
#include <netinet/in.h>
#include <vector>
//#include "shell.cc"

#define SERV_TCP_PORT 9487

using namespace std;

//string get_file_type(string file);
string read_until(const char *arr, const char *terminal, int &cursor);

void waitfor(int sig)
{
	int wstat;
	int pid = wait(&wstat);
}

void killme(int sig)
{
	signal(SIGINT, SIG_DFL);
	kill(getpid(), SIGINT);
}

int main()
{
	fstream f("socks.conf");
	vector<vector<int> >firewall;
	vector<int> cd;
	while(!f.eof())
	{
		string in;
		f >> in;
		if(in == "permit")
		{
			int mode;
			f >> in;
			if(in == "c" || in == "b")
			{
				if(in == "c")mode = 1;
				else mode = 2;
				f >> in;
				int cursor = 0;
				vector<int> tmp;
				for(int i = 0; i < 4; i++)
				{
					string s = read_until(in.c_str(), ".", cursor);
					if(s == "*")tmp.push_back(-1);
					else
					{
						int byte = 0;
						for(int i = 0; i < s.size(); i++)
						{
							byte *= 10;
							byte += s[i] - '0';
						}
						tmp.push_back(byte);
					}
				}
				firewall.push_back(tmp);
				cd.push_back(mode);
			}
		}
	}
	signal(SIGCHLD, waitfor);
	signal(SIGINT, killme);
	int serv_tcp_port = SERV_TCP_PORT;
	struct sockaddr_in cli_addr, serv_addr;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		cout << "[ERR] Cannot open socket" << endl;
		return 0;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cout << "[LOG] Binding port 9487 ...| ";
	int mv = 0;
	while(1)
	{
		serv_addr.sin_port = htons(serv_tcp_port);
		int bin = bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		if(bin >= 0)break;
		mv = (mv + 1) % 4;
		switch(mv)
		{
			case 0:
				cout << "\b\b";
				cout << "| ";
			break;
			case 1:
				cout << "\b\b";
				cout << "\\ ";
			break;
			case 2:
				cout << "\b\b";
				cout << "- ";
			break;
			case 3:
				cout << "\b\b";
				cout << "/ ";
			break;
			default:
			break;
		}
		//serv_tcp_port++;
	}
	cout << endl;
	cout << "[LOG] Bind port: " << serv_tcp_port << endl;
	listen(sock, 5);
	while(1)
	{
		unsigned int clilen = sizeof(cli_addr);
		char hbuf[1024], sbuf[1024];
		int newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
		getnameinfo((struct sockaddr *)&cli_addr, clilen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
		cout << "[LOG] Connection accepted: " << hbuf << "/" << sbuf << endl;
		int child;
		if((child = fork()) < 0)
		{
			cout << "[ERR] Fork error" << endl;
			return 0;
		}
		else if(child == 0) 
		{
			signal(SIGCHLD, SIG_DFL);
			close(sock);
			unsigned char packet[1024 * 1024] = {};
			int cursor = 0;
			int receive = read(newsock, packet, 1024 * 1024);
			cout << "[LOG] Received request: " << receive << " bytes" << endl;
			cout << "[LOG] Socket version: " << (int)packet[cursor++] << endl;
			int mode = (int)packet[cursor++];
			cout << "[LOG] Connection mode: " << mode << endl;//<< (int)packet[cursor++] << endl;
			int destport = 0;
			destport += packet[cursor++];
			destport *= 256;
			destport += packet[cursor++];
			cout << "[LOG] Dest port: " << destport << endl;
			int destip[4] = {};
			for(int i = 0; i < 4; i++)destip[i] = packet[cursor++];
			cout << "[LOG] Dest ip: ";
			int permission = 0;
			for(int i = 0; i < firewall.size(); i++)
			{
				int cnt = 0;
				if(cd[i] != mode)continue;
				for(int j = 0; j < 4; j++)
				{
					if(firewall[i][j] == destip[j] || firewall[i][j] == -1)cnt++;
				}
				if(cnt == 4)permission = 1;
			}
			stringstream ss;
			for(int i = 0; i < 4; i++)ss << (i? ".": "") << destip[i];
			cout << ss.str() << endl;
			if(permission)cout << "[LOG] Request approved" << endl;
			else cout << "[LOG] Request denied" << endl;
			int userid = 0;
			while(packet[cursor] != 0)
			{
				userid *= 256;
				userid += packet[cursor++];
			}
			cout << "[LOG] User id: " << userid << endl;
			unsigned char reply[1024] = {};
			cursor = 0;
			reply[cursor++] = 0;
			reply[cursor++] = 90 + !permission;
			reply[cursor++] = destport / 256;
			reply[cursor++] = destport % 256;
			for(int i = 0; i < 4; i++)reply[cursor++] = destip[i];
			if(mode == 1)
			{
				write(newsock, reply, 8);
				if(!permission)return 0;
				struct sockaddr_in connect_addr;
				bzero((char *) &connect_addr, sizeof(connect_addr));
				connect_addr.sin_family = AF_INET;
				connect_addr.sin_addr.s_addr = inet_addr(ss.str().c_str());
				connect_addr.sin_port = htons(destport);
				int connectfd = socket(AF_INET, SOCK_STREAM, 0);
				if(connectfd < 0)
				{
					cout << "[ERR] Cannot open web-server socket" << endl;
					return 0;
				}
				while(connect(connectfd, (struct sockaddr *)&connect_addr, sizeof(connect_addr)) < 0)cout << "[ERR] Cannot connect to web-server " << errno << endl;
				int nfds = getdtablesize();
				fd_set rfds, afds;
				FD_ZERO(&afds);
				FD_SET(newsock, &afds);
				FD_SET(connectfd, &afds);
				char buffer[1024];
				int size = 0;
				while(1)
				{
					memcpy(&rfds, &afds, sizeof(&rfds));
					if(select(nfds, &rfds, NULL, NULL, 0) < 0)
					{
						cout << "[ERR] select error" << endl;
						return 0;
					}
					if(FD_ISSET(newsock, &rfds))
					{
						bzero(buffer, 1024);
						size = read(newsock, buffer, 1024);
						if(size == 0)
						{
							cout << "[LOG] Client side ended" << endl;
							break;
						}
						else if(size == -1)cout << "[ERR] Client read error:" << errno << endl;
						else size = write(connectfd, buffer, size);
					}
					if(FD_ISSET(connectfd, &rfds))
					{
						bzero(buffer, 1024);
						size = read(connectfd, buffer, 1024);
						if(size == 0)
						{
							cout << "[LOG] Client side ended" << endl;
							break;
						}
						else if(size == -1)cout << "[ERR] Client read error:" << errno << endl;
						else size = write(newsock, buffer, size);
					}
				}
				close(connectfd);
				close(newsock);
				cout << "[LOG] closed socket" << endl;
			}
			else 
			{
				int bindfd;
				struct sockaddr_in bind_addr;
				bindfd = socket(AF_INET, SOCK_STREAM, 0);
				bind_addr.sin_family = AF_INET;
				bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
				bind_addr.sin_port = htons(0);
				bind(bindfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
				struct sockaddr_in sockin_addr;
				int someint;
				getsockname(bindfd, (struct sockaddr *)&sockin_addr, (socklen_t *)&someint);
				listen(bindfd, 5);
				cursor = 0;
				reply[cursor++] = 0;
				reply[cursor++] = 90;
				reply[cursor++] = (unsigned char)(ntohs(sockin_addr.sin_port) / 256);
				reply[cursor++] = (unsigned char)(ntohs(sockin_addr.sin_port) % 256);
				reply[cursor++] = 0;
				reply[cursor++] = 0;
				reply[cursor++] = 0;
				reply[cursor++] = 0;
				write(newsock, reply, 8);
				if(!permission)return 0;
				struct sockaddr newaddr;
				unsigned int len = sizeof(newaddr);
				int newsock2 = accept(bindfd, (struct sockaddr *)&newaddr, &len);
				write(newsock, reply, 8);
				int nfds = getdtablesize();
				fd_set rfds, afds;
				FD_ZERO(&afds);
				FD_SET(newsock2, &afds);
				FD_SET(newsock, &afds);
				char buffer[1024];
				int size = 0;
				while(1)
				{
					memcpy(&rfds, &afds, sizeof(&rfds));
					if(select(nfds, &rfds, NULL, NULL, 0) < 0)
					{
						cout << "[ERR] select error" << endl;
						return 0;
					}
					if(FD_ISSET(newsock, &rfds))
					{
						bzero(buffer, 1024);
						size = read(newsock, buffer, 1024);
						if(size == 0)
						{
							cout << "[LOG] Client side ended" << endl;
							break;
						}
						else if(size == -1)cout << "[ERR] Client read error:" << errno << endl;
						else size = write(newsock2, buffer, size);
					}
					if(FD_ISSET(newsock2, &rfds))
					{
						bzero(buffer, 1024);
						size = read(newsock2, buffer, 1024);
						if(size == 0)
						{
							cout << "[LOG] Client side ended" << endl;
							break;
						}
						else if(size == -1)cout << "[ERR] Client read error:" << errno << endl;
						else size = write(newsock, buffer, size);
					}
				}
				close(newsock2);
				close(newsock);
				cout << "[LOG] closed socket" << endl;
			}
			return 0;
		}
		close(newsock);
	}
}
/*
string get_file_type(string file)
{
	bool flag = false;
	for(int i = file.size() - 1; i >= 0; i--)
	{
		if(file[i] == '.')return string(file.c_str() + i + 1);
	}
	return string("");
}
*/
string read_until(const char *arr, const char *terminal, int &cursor)
{
	char str[1024] = {};
	for(int i = 0; i < 1024; i++)
	{
		if(arr[cursor] == '\0')return string(str);
		for(int j = 0;terminal[j] != '\0'; j++)
		{
			if(arr[cursor] == terminal[j])
			{
				cursor++;
				return string(str);
			}
		}
		str[i] = arr[cursor];
		cursor++;
	}
}
