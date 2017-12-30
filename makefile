all:
	g++ ../nphw3/server.cc -o webserver
	g++ hw4.cc -o root/hw4.cgi
	g++ nslookup.cc -o root/nslookup.cgi
server:
	g++ ../nphw3/server.cc -o webserver
cgi:
	g++ hw4.cc -o root/hw4.cgi
nslookup:
	g++ nslookup.cc -o root/nslookup.cgi
	
