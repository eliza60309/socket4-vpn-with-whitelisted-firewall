#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

using namespace std;
struct sockaddr_in cli_addr;
int newsockfd;

void handle_socket()
{
char buffer[1000000];
unsigned char request[1000];
unsigned char reply[8];

read(newsockfd, request, 1000);

unsigned char VN = request[0] ;
unsigned char CD = request[1] ;
unsigned int DST_PORT = request[2] << 8 | request[3];
unsigned int DST_IP = request[4] << 24 | request[5] << 16 | request[6] << 8 | request[7];
char* USER_ID = (char*) (request + 8);

if(VN!=0x04) exit(0);

ifstream firewall("socks.conf");

string permit, mode, addr, rule;
unsigned char IP[4];
while(getline(firewall, rule))
{
stringstream parse(rule);
parse >> permit >> mode >> addr;
if(permit[0] == '#') continue;

bool all[4];
for(int i = 0; i < 4; ++i) all[i] = false;

stringstream ss(addr);
//cout << "fire addr: " << addr << endl;
for(int i = 0; i < 4; ++i)
{
string num;
getline(ss, num, '.');
if(num == "*") all[i] = true;
else
{
IP[i] = stoi(num);
//cout << "parse: " << (int)IP[i] << " " << (int)request[i+4] << endl;
}
}
if( (mode == "c" && CD == 1) || (mode == "b" && CD == 2) )
{
if( (all[0] == true || (IP[0] == request[4])) &&
(all[1] == true || (IP[1] == request[5])) &&
(all[2] == true || (IP[2] == request[6])) &&
(all[3] == true || (IP[3] == request[7])) )
{
reply[1] = 90;
break;
}
else
{
reply[1] = 91;
}
}
else reply[1] = 91;
}


cout << "VN = " << (int)VN << " CD = " << (int)CD << " USER_ID = " << USER_ID << endl;
cout << "<S_IP>t:" << inet_ntoa(cli_addr.sin_addr) << endl;
cout << "<S_PORT>t:" << cli_addr.sin_port << endl;
cout << "<D_IP>t:" << (int)request[4] << "." << (int)request[5] << "." << (int)request[6] << "." << (int)request[7] << endl;
cout << "<D_PORT>t:" << DST_PORT << endl;
cout << "<Command>t:" << ((CD == 1) ? "CONNECT" : "BIND") << endl;
cout << "<Reply>t:" << ((reply[1] == 90) ? "Accept" : "Reject") << endl;

if(CD == 1) // Connect Mode
{
reply[0] = 0;
reply[2] = DST_PORT / 256;
reply[3] = DST_PORT % 256;
reply[4] = DST_IP >> 24;
reply[5] = (DST_IP >> 16) & 0xFF;
reply[6] = (DST_IP >> 8) & 0xFF;
reply[7] = DST_IP & 0xFF;
write(newsockfd, reply, 8);
if(reply[1] == 91) return;

int dest_sockfd;
struct sockaddr_in dest_addr;
dest_sockfd = socket(AF_INET,SOCK_STREAM, 0);
bzero((char *) &dest_addr, sizeof(dest_addr));
dest_addr.sin_family = AF_INET;
dest_addr.sin_addr.s_addr = htonl(DST_IP);
dest_addr.sin_port = htons(DST_PORT);

while(connect(dest_sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1)
{
cout << "Connect web server fail" << endl;
continue;
}

fd_set rfds;
fd_set afds;
int nfds = getdtablesize();
int len;

FD_ZERO(&afds);
FD_SET(dest_sockfd, &afds);
FD_SET(newsockfd, &afds);

while(1)
{
memcpy(&rfds, &afds, sizeof(rfds));
if ( select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0 ) return;
if( FD_ISSET(newsockfd, &rfds) )
{
bzero(buffer, sizeof(buffer));
len = read(newsockfd, buffer, sizeof(buffer));
if (len == 0)
{
cout << "Browser Endnn" << endl;
FD_CLR(newsockfd, &afds);
break;
}
else if(len == -1)
{
perror("Browser Error");
FD_CLR(newsockfd, &afds);
break;
}
else
{
len = write(dest_sockfd, buffer, len);
cout << "Write to dest: " << len << endl;
}
}
if( FD_ISSET(dest_sockfd, &rfds) )
{
bzero(buffer, sizeof(buffer));
len = read(dest_sockfd, buffer, sizeof(buffer));
if (len == 0)
{
cout << "Webserver End" << endl;
FD_CLR(dest_sockfd, &afds);
break;
}
else if (len == -1)
{
perror("Webserver Error");
FD_CLR(dest_sockfd, &afds);
break;
}
else
{
len = write(newsockfd, buffer, len);
cout << "Write to socks: " << len << endl;
}
}
}
close(newsockfd);
close(dest_sockfd);
}
else if(CD == 2) // Bind Mode
{
int bind_sockfd;
struct sockaddr_in bind_addr;

bind_sockfd = socket(AF_INET, SOCK_STREAM, 0);
bind_addr.sin_family = AF_INET;
bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
bind_addr.sin_port = 0;


bind(bind_sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

struct sockaddr_in local_sockaddr;
socklen_t local_len = sizeof(struct sockaddr_in);
getsockname(bind_sockfd, (sockaddr*)&local_sockaddr, &local_len);

listen(bind_sockfd, 5);

reply[0] = 0;
reply[2] = (unsigned char)(ntohs(local_sockaddr.sin_port) / 256);
reply[3] = (unsigned char)(ntohs(local_sockaddr.sin_port) % 256);
reply[4] = 0;
reply[5] = 0;
reply[6] = 0;
reply[7] = 0;
write(newsockfd, reply, 8);
if(reply[1] == 91) return;

int ftp_sockfd;
struct sockaddr_in ftp_addr;
socklen_t ftp_len = sizeof(ftp_addr);
ftp_sockfd = accept(bind_sockfd, (struct sockaddr *)&ftp_addr, (socklen_t*)&ftp_len);
write(newsockfd, reply, 8);

fd_set rfds;
fd_set afds;
int nfds = getdtablesize();
int len;
FD_ZERO(&afds);
FD_SET(ftp_sockfd, &afds);
FD_SET(newsockfd, &afds);

while(1)
{
memcpy(&rfds, &afds, sizeof(rfds));
if ( select(nfds, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0 ) return;
if( FD_ISSET(newsockfd, &rfds) )
{
bzero(buffer, sizeof(buffer));
len = read(newsockfd, buffer, sizeof(buffer));
if (len == 0)
{
cout << "Socks End" << endl;
FD_CLR(newsockfd, &afds);
break;
}
else if(len == -1)
{
perror("Socks Error");
FD_CLR(newsockfd, &afds);
break;
}
else
{
len = write(ftp_sockfd, buffer, len);
cout << "Write to FTp : " << len << endl;
}
}
if( FD_ISSET(ftp_sockfd, &rfds) )
{
bzero(buffer, sizeof(buffer));
len = read(ftp_sockfd, buffer, sizeof(buffer));
if (len == 0)
{
cout << "FTP End" << endl;
FD_CLR(ftp_sockfd, &afds);
break;
}
else if (len == -1)
{
perror("FTP Error");
FD_CLR(ftp_sockfd, &afds);
break;
}
else
{
len = write(newsockfd, buffer, len);
cout << "Write to socks: " << len << endl;
}
}
}
close(newsockfd);
close(ftp_sockfd);
}
}
void child_exit(int)
{
wait(nullptr);
}
int main(int argc, char *argv[])
{
if(argc != 2)
{
cout << "wrong input" << endl;
exit(0);
}
signal(SIGCHLD, child_exit);

int sockfd;
struct sockaddr_in serv_addr;

sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd < 0) perror("ERROR opening socket");
bzero((char *) &serv_addr, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port = htons(atoi(argv[1]));

int optval = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
{
perror("ERROR on binding");
exit(0);
}
listen(sockfd,512);

socklen_t clilen;
while(1)
{
clilen = sizeof(cli_addr);
newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

pid_t pid;
pid = fork();
// child process
if (pid == 0)
{
close(sockfd);
handle_socket();
return 0;
}
else close(newsockfd);
}
}
