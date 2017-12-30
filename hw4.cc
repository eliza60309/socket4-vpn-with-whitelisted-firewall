#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <fcntl.h>
#include <netinet/in.h>

#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3
#define F_RDONE 4
#define F_REQUEST 5
#define F_RECEIVE 6

using namespace std;

string read_until(const char *arr, const char *terminal, int &cursor);
string nslookup(string s);
int getinteger(const char *c);
string readline(int sock);
int dump(string s);

class request
{
	public:
	string host;
	string sh;
	string sp;
	string port;
	string file;
	int state;
	int fd;
	string input;
	int cursor;
	unsigned char p_request[9];
};

int main()
{
	cout << "Content-Type: text/html\r\n\r\n";
	string query = getenv("QUERY_STRING");
	//string query = "h1=npbsd2.cs.nctu.edu.tw&p1=1257&f1=t1.txt&h2=npbsd2.cs.nctu.edu.tw&p2=1257&f2=t2.txt&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=";
	map<string, string> args;
	int cursor = 0;
	while(1)
	{
		string name = read_until(query.c_str(), "=", cursor);
		if(name == "")break;
		string cont = read_until(query.c_str(), "&", cursor);
		args[name] = cont;
	}
	vector<request> request_vector;
	vector<string> number;
	number.push_back("1");
	number.push_back("2");
	number.push_back("3");
	number.push_back("4");
	number.push_back("5");
	cout << "<html>" << endl;
	cout << "<head>" << endl;
	cout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\"/>" << endl;
	cout << "<title>Network Programming Homework 3</title>" << endl;
	cout << "</head>" << endl;
	cout << "<body bgcolor=#336699>" << endl;
	cout << "<font face=\"Courier New\" size=2 color=#FFFF99>" << endl;
	cout << "<table width=\"800\" border=\"1\">" << endl;
	cout << getinteger("10") << endl;
	for(int i = 0; i < number.size(); i++)
	{
		if(args["h" + number[i]].size() != 0 && args["p" + number[i]].size() != 0 && args["f" + number[i]].size() != 0 && args["sh" + number[i]].size() != 0 && args["sp" + number[i]].size() != 0)
		{
			//cout << "<! " + number[i] + ">" << endl;
			request request_instance;
			request_instance.host = nslookup(args["h" + number[i]]);
			//cout << "<! " + request_instance.host + ">" << endl;
			unsigned int ip = inet_addr(request_instance.host.c_str());
			request_instance.p_request[8] = 0;
			request_instance.p_request[0] = 4;
			request_instance.p_request[1] = 1;
			request_instance.p_request[7] = ip / (256 * 256 * 256);
			request_instance.p_request[6] = (ip / (256 * 256)) % 256;
			request_instance.p_request[5] = (ip / 256) % 256;
			request_instance.p_request[4] = ip % 256;
			request_instance.file = args["f" + number[i]];
			request_instance.port = args["p" + number[i]];
			request_instance.p_request[2] = getinteger(args["p" + number[i]].c_str()) / 256;
			request_instance.p_request[3] = getinteger(args["p" + number[i]].c_str()) % 256;
			//cout << "<!";
			//for(int j = 0; j < 4; j++)cout << request_instance.p_request[4 + j] << " ";
			//cout << ">" << endl;
			request_instance.sh = nslookup(args["sh" + number[i]]);
			//cout << "<! " + request_instance.sh + ">" << endl;
			request_instance.sp = args["sp" + number[i]];
			request_instance.state = 0;
			int sock = socket(PF_INET, SOCK_STREAM, 0);
			fstream in(request_instance.file.c_str());
			string input;
			input.assign(istreambuf_iterator<char>(in), istreambuf_iterator<char>());
			request_instance.input = input;
			request_instance.cursor = 0;
			request_instance.fd = sock;
			request_instance.state = F_CONNECTING;
			request_vector.push_back(request_instance);
			int flag = fcntl(sock, F_GETFL, 0);
			fcntl(sock, F_SETFL, flag | O_NONBLOCK);
		}
	}
	fd_set rfds;
	fd_set wfds;
	fd_set rs;
	fd_set ws;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	cout << "<tr>" << endl;
	for(int i = 0; i < request_vector.size(); i++)
	{
		cout << "<td>" << endl;
		struct sockaddr_in cli_addr, serv_addr;
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(request_vector[i].sh.c_str());
		serv_addr.sin_port = htons(getinteger(request_vector[i].sp.c_str()));
		if(connect(request_vector[i].fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			if(errno != EINPROGRESS)
			{
				cout << "[ERR] connect error" << endl;
				return -1;
			}
		}
		FD_SET(request_vector[i].fd, &rs);
		FD_SET(request_vector[i].fd, &ws);
		cout << request_vector[i].host << endl;
		cout << "</td>" << endl;
	}
	cout << "<tr>" << endl;
	cout << "</tr>" << endl;
	for(int i = 0; i < request_vector.size(); i++)
	{
		cout << "<td valign=\"top\" id=\"" << "m" << number[i] << "\">" << endl;
		cout << "</td>" << endl;
	}
	cout << "</tr></table>" << endl;
	rfds = rs;
	wfds = ws;
	int nfds = FD_SETSIZE;
	int connection_cnt = request_vector.size();
	int time_out = 100000;
	vector<int>timeout(request_vector.size(), 0);
	vector<int>f(request_vector.size(), 0);
	while(connection_cnt > 0 || time_out == 0)
	{
		memcpy(&rfds, &rs, sizeof(rfds));
		memcpy(&wfds, &ws, sizeof(wfds));
		if(select(nfds, &rfds, &wfds, NULL, NULL) < 0)
		{
			if(errno == EINTR)continue;
			else 
			{
				cout << "[ERR] select error" << endl;
				return 0; 
			}
		}
		for(int i = 0; i < request_vector.size(); i++)
		{
			if(request_vector[i].state == F_DONE)continue;
			if(request_vector[i].state == F_CONNECTING)
			{
				int error, k;
				if(getsockopt(request_vector[i].fd, SOL_SOCKET, SO_ERROR, &error, (unsigned int *)&k) < 0 || error != 0)
				{
					cout << "[ERR] socket non blocking error" << endl;
					return 0;
				}
				cout << "[LOG] connected" << endl;
				FD_CLR(request_vector[i].fd, &wfds);
				request_vector[i].state = F_REQUEST;
			}
			else if(request_vector[i].state == F_REQUEST && FD_ISSET(request_vector[i].fd, &wfds))
			{
				write(request_vector[i].fd, request_vector[i].p_request, 9);
				cout << "requested" << endl;
				request_vector[i].state = F_RECEIVE;
			}
			else if(request_vector[i].state == F_RECEIVE && FD_ISSET(request_vector[i].fd, &rfds))
			{
				unsigned char c[1024 * 1024] = {};
				read(request_vector[i].fd, c, 8);
				for(int j = 0; j < 8; j++)cout << (int)c[j] << " ";
				cout << endl;
				request_vector[i].state = F_READING;
			}
			else if(request_vector[i].state == F_READING && FD_ISSET(request_vector[i].fd, &rfds))
			{
				cout << "reading" << endl;
				char c[1024 * 1024] = {};
				int flag = true;
				int ans = read(request_vector[i].fd, c, 1024 * 1024);
				string inst = c;
				if(ans)
				{
					int cursor = 0;
					while(cursor != inst.size())
					{
						cout << "<script>document.all['m" << number[i] << "'].innerHTML +=\"";
						string s = read_until(inst.c_str(), "\n", cursor);
						if(cursor > 0 && inst.c_str()[cursor - 1] == '\n')s += '\n';
						dump(s);
						cout << "\";</script>" << endl;
					}
				}
				if(inst.find("% ") != string::npos)//cout << "\";</script>" << endl;
				{
					request_vector[i].state = F_WRITING;
					FD_CLR(request_vector[i].fd, &rs);
					FD_SET(request_vector[i].fd, &ws);
				}
				if(f[i])
				{
					request_vector[i].state = F_DONE;
					FD_CLR(request_vector[i].fd, &ws);
					FD_CLR(request_vector[i].fd, &rs);
					connection_cnt--;
				}
			}
			else if(request_vector[i].state == F_WRITING && FD_ISSET(request_vector[i].fd, &wfds))
			{
				string input = read_until(request_vector[i].input.c_str(), "\n", request_vector[i].cursor);
				if(request_vector[i].cursor > 0 && request_vector[i].input.c_str()[request_vector[i].cursor - 1] == '\n')input += '\n';
				cout << "<script>document.all['m" << number[i] << "'].innerHTML += \"";
				if(dump(input))f[i] = true;
				cout << "\";</script>" << endl;
				int n = write(request_vector[i].fd, input.c_str(), input.size());
				FD_CLR(request_vector[i].fd, &ws);
				FD_SET(request_vector[i].fd, &rs);
				if(request_vector[i].cursor == request_vector[i].input.size())f[i] = true;
				request_vector[i].state = F_READING;
			}
		}
	}
	cout << "</font></body></html>" << endl;
	return 0;
}

string nslookup(string s)
{
	int p[0];
	pipe(p);
	if(!fork())
	{
		close(p[0]);
		dup2(p[1], 1);//1 for stdout
		if(!execl("nslookup.cgi", "nslookup.cgi", s.c_str(), NULL))exit(0);
		else exit(1);
	}
	else
	{
		close(p[1]);
		int wstat;
		wait(&wstat);
		char ret[1024];
		read(p[0], ret, 1024);
		close(p[0]);
		return string(ret);
	}
}

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

int getinteger(const char *c)
{
	int num = 0;
	for(int i = 0; i < 20; i++)
	{
		//cout << "??" << (int)c[i] << endl;
		if(c[i] == 0)return num;
		num *= 10;
		num += c[i] - '0';
	}
	return num;
}

string readline(int sock)
{
	int sum = 0;
	string s;
	while(1)
	{
		char c;
		int ret = read(sock, &c, 1);
		if(ret)
		{
			if(c == '\n')
			{
				s += c;
				return s;
			}
			else if(c == '\0')return s;
		}
		else if(ret == -1 && errno != EINTR)return "ERROR IN STRING";
	}
}

int dump(string s)
{
	int flag = false;
	for(int i = 0; i < s.size(); i++)
	{
		if(i >= 3 && s[i - 3] == 'e' && s[i - 2] == 'x' && s[i - 1] == 'i' && s[i] == 't')flag = true;
		if(s[i] == '\n')cout << "<br>";
		else if(s[i] == '\"')cout << "\\\"";
		else if(s[i] == '\'')cout << "\\\'";
		else if(s[i] == '\r');
		else if(s[i] == '<')cout << "&lt";
		else if(s[i] == '>')cout << "&gt";
		else if(s[i] == '&')cout << "&amp";
		else if(s[i] == '\'')cout << "&#39";
		else cout << s[i];
	}
	return flag;
}
