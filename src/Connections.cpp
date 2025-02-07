#include "Connections.hpp"
#include <sys/event.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "Client.hpp"

Connections::Connections(ServerContext *ctx, int kqueueFd) : ctx(ctx), kqueueFd(kqueueFd) {}

Connections::~Connections()
{
	for (ClientsIter it = clients.begin(); it != clients.end(); ++it)
	{
		close(it->first);
	}
}

void log(const std::string &str)
{
	time_t now = time(NULL);
	struct tm *timeinfo = localtime(&now);

	std::stringstream ss;
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	ss << "[" << days[timeinfo->tm_wday] << " " << months[timeinfo->tm_mon] << " " << std::setfill('0') << std::setw(2)
	   << timeinfo->tm_mday << " " << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":" << std::setfill('0')
	   << std::setw(2) << timeinfo->tm_min << ":" << std::setfill('0') << std::setw(2) << timeinfo->tm_sec << " "
	   << (1900 + timeinfo->tm_year) << "]";
	std::cout << green;
	std::cout << ss.str() << " ";
	std::cout << str;
	std::cout << _reset << "\n";
}
void Connections::closeConnection(int fd)
{
	log("client disconnected");
	close(fd); // after close  fd all event will be clear
	clients.erase(fd);
}

void Connections::addConnection(int fd, int server)
{
	log("client connect");
	this->clients.insert(std::make_pair(fd, Client(fd, server, this->ctx)));
}

Client *Connections::requestHandler(int fd, int data)
{
	ClientsIter clientIter = this->clients.find(fd);
	if (clientIter == this->clients.end())
		return (NULL);
	clientIter->second.request.readRequest(data);
	return (&(clientIter->second));
}

void Connections::init(ServerContext *ctx, int kqueueFd)
{
	this->ctx = ctx;
	this->kqueueFd = kqueueFd;
}

Client *Connections::getClient(int fd)
{
	std::map<int, Client >::iterator kv = this->clients.find(fd);
	if (kv == this->clients.end())
		return (NULL);
	return (&(kv->second));
}
