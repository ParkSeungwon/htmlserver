//tcpip.cc class 구현부
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <chrono>
#include <cassert>
#include "tcpip.h"
using namespace std;

Tcpip::Tcpip(int port) 
{
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

Tcpip::~Tcpip()
{
	close(client_fd);
	close(server_fd);
}
void Tcpip::send(string s) 
{
	write(client_fd, s.c_str(), s.size()+1);
}
void Server::send(string s, int client_fd)
{
	try {
		write (client_fd, s.c_str(), s.size()+1);
	} catch(...) {}
}

string Tcpip::recv()
{
	int i = read(client_fd, buffer, 1023);//error
	buffer[i] = '\0';
	return string(buffer);
}
string Server::recv(int client_fd)
{
	try {
		int i = read(client_fd, buffer, 1023);//error
		buffer[i] = '\0';
	} catch(...) {}
	return string(buffer);
}

Client::Client(string ip, int port) : Tcpip(port) 
{
	server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	if(-1 == connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)))
		cout << "connect() error" << endl;
	else cout << " connecting"  <<endl;
}

Server::Server(int port, unsigned int t, int queue, string e) : Tcpip(port) 
{
	end_string = e;
	time_out = t;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		cout << "bind() error" << endl;
	else cout << "binding" << endl;
	if(listen(server_fd, queue) == -1) cout << "listen() error" << endl;
	else cout << "listening" << endl;
}

void Server::start(function<string(string)> functor)
{
	int cl_size = sizeof(client_addr);
	while(true) {
		int fd = accept(server_fd, (sockaddr*)&client_addr, (socklen_t*)&cl_size);
		if(fd == -1) cout << "accept() error" << endl;
		else {
			cout << "accepting" << endl;
			connections.push_back(thread(&Server::handle_connection, this, functor, fd));
		}
	}
}

void Server::timed_out(int sig)
{
	cout << "time out" << endl;
	exit(0);
}


void Server::handle_connection(function<string(string)> functor, int fd)
{///inside of one connection, every connection has its own q,mtx and so on.
	deque<string> in, out;
	mutex mtx;
	condition_variable cv;
	signal(SIGALRM, timed_out);
	thread th(&Server::qrecv, this, fd, std::ref(in), ref(mtx), ref(cv));

	unique_lock<mutex> lck{mtx, defer_lock};
	string s = "not end";
	while(s != end_string) {
		lck.lock();
		while(in.empty()) cv.wait(lck);
		s = in.front();
		send(functor(s), fd);//return 값을 큐에 넣는 식으로 바꿔야 함.
		assert(!q.empty());
		q.pop_front();
		lck.unlock();
	}
	th.join();
	cout << "ending child" << endl;
}

void Server::qrecv(int fd, deque<string>& q, mutex& mtx, condition_variable& cv)
{
	string s = "not end";
	unique_lock<mutex> lck{mtx, defer_lock};
	while(s != end_string) {
		s = recv(fd);
		alarm(time_out);
		lck.lock();
		q.push_back(s);
		lck.unlock();
		cv.notify_all();
	}
}
Server::~Server()
{
	for(auto& a : connections) a.join();
}
