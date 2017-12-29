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


int main()
{
	fstream f("socks.conf");
	vector<vector<int> >firewall;
	while(!f.eof())
	{
		string in;
		f >> in;
		if(in == "permit")
		{
			f >> in;
			if(in == "c")
			{
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
			}
		}
	}
	signal(SIGCHLD, waitfor);
	int serv_tcp_port = SERV_TCP_PORT;
	struct sockaddr_in cli_addr, serv_addr;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
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
		int bin = bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
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
			cout << "[LOG] Connection mode: " << (int)packet[cursor++] << endl;
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
				for(int j = 0; j < 4; j++)
				{
					if(firewall[i][j] == destip[j] || firewall[i][j] == -1)cnt++;
				}
				if(cnt == 4)permission = 1;
			}
			stringstream ss;
			for(int i = 0; i < 4; i++)ss << (i? ".": "") << destip[i];
			cout << ss.str();
			cout << endl;
			if(permission)cout << "[LOG] Request approved" << endl;
			else cout << "[LOG] Request denied" << endl;
			int userid = 0;
			while(packet[cursor] != 0)
			{
				userid *= 256;
				userid += packet[cursor++];
			}
			cout << "[LOG] User id: " << userid << endl << endl;
			unsigned char reply[1024] = {};
			cursor = 0;
			reply[cursor++] = 0;
			reply[cursor++] = 90 + !permission;
			reply[cursor++] = destport / 256;
			reply[cursor++] = destport % 256;
//			stringstream sp;
//			sp << destport;
			for(int i = 0; i < 4; i++)reply[cursor++] = destip[i];
			write(newsock, reply, 10);
			if(!permission)
			{
				close(newsock);
				return 0;
			}
			struct sockaddr_in connect_addr;
			bzero((char *) &connect_addr, sizeof(connect_addr));
			connect_addr.sin_family = AF_INET;
			connect_addr.sin_addr.s_addr = inet_addr(ss.str().c_str());
			connect_addr.sin_port = htons(destport);
			int connectfd = socket(PF_INET, SOCK_STREAM, 0);
			int flag = fcntl(newsock, F_GETFL, 0);
			fcntl(newsock, F_SETFL, flag | O_NONBLOCK);
			flag = fcntl(connectfd, F_GETFL, 0);
			fcntl(connectfd, F_SETFL, flag | O_NONBLOCK);
			if(connectfd < 0)
			{
				cout << "[ERR] Cannot open web-server socket" << endl;
				return 0;
			}
			if(connect(connectfd, (struct sockaddr *)&connect_addr, sizeof(connect_addr)) < 0 && errno != EINPROGRESS)
			{
				cout << "[ERR] Cannot connect to web-server " << errno << endl;
				return 0;
			}
			//int k, error;
			cout << "[LOG] Connecting..." << endl;
			int nfds = FD_SETSIZE;
			//int size = 0;
			//char sigpacket = {};
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(newsock, &rfds);
			FD_SET(connectfd, &rfds);
			int state = 0;
			int time = 0;
			while(1)
			{
				if(state == 0)
				{
					if(select(nfds, &rfds, NULL, NULL, NULL) < 0)
					{
						cout << "[ERR] Select error" << endl;
						return 0;
					}
					int error, k;
					if(getsockopt(connectfd, SOL_SOCKET, SO_ERROR, &error, (unsigned int *)&k) < 0 || error != 0)
					{
						cout << "[ERR] Connection error" << endl;
						return 0;
					}
					else 
					{
						cout << "[LOG] Connection established" << endl;
						state = 1;
					}
				}
				else
				{
					char sigpacket[1024 * 1024] = {};
					int ans = read(newsock, sigpacket, 1024 * 1024);
					if(ans > 0)
					{
						state = 2;
						cout << "[LOG] Browser packet" << endl;
						cout << sigpacket << endl;
						int wcursor = 0;
						while(1)
						{
							int wans = write(connectfd, sigpacket + wcursor, ans - wcursor);
							//cout << (int)(sigpacket + wcursor)[0] << endl;
							if(wans > 0)wcursor += wans;
							if(wcursor >= ans)break;
						}
					}
					ans = read(connectfd, sigpacket, 1024 * 1024);
					if(ans < 0 && errno == EAGAIN)
					{
						cout << "[LOG] TIME = " << time << endl;
						if(state == 3)
						{
							time++;
							if(time >= 1000)break;
						}
						else
						{
							state = 3;
							time = 0;
						}
					}
					if(ans > 0)
					{
						cout << "[LOG] Web server packet" << endl;
						cout << sigpacket << endl;
						int wcursor = 0;
						while(1)
						{
							int wans = write(newsock, sigpacket + wcursor, ans - wcursor);
							//cout << (int)(sigpacket + wcursor)[0] << endl;
							if(wans > 0)wcursor += wans;
							if(wcursor >= ans)break;
						}
					}
					//else if(ans < 0 && errno != EINTR && errno != EAGAIN)break;
				}
			}
			close(connectfd);
			close(newsock);
			cout << "[LOG] closed socket" << endl;
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
