#include "Event.hpp"
#include <netdb.h>
#include <sys/_endian.h>
#include <sys/_types/_errno_t.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "CGIProcess.hpp"
#include "Client.hpp"
#include "Connections.hpp"
#include "DataType.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "VirtualServer.hpp"

Event::Event(int max_connection, int max_events, ServerContext *ctx)
	: connections(ctx, -1), MAX_CONNECTION_QUEUE(max_connection), MAX_EVENTS(max_events)
{
	this->ctx = ctx;
	this->evList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}

Event::~Event()
{
	delete this->evList;
	delete this->eventChangeList;

	VirtualServerMap_t::iterator it = this->virtuaServers.begin();
	for (; it != this->virtuaServers.end(); it++)
		close(it->first);
	std::map<int, VirtualServer *>::iterator it2 = this->defaultServer.begin();
	for (; it2 != this->defaultServer.end(); it2++)
	{
		if (this->virtuaServers.find(it->first) == this->virtuaServers.end())
			close(it2->first);
	}
	ProcMap_t::iterator kv = this->procs.end();
	for (; kv != this->procs.end(); kv++)
	{
		kv->second.die();
		kv->second.clean();
	}
	if (this->kqueueFd >= 0)
		close(this->kqueueFd);
}

void Event::InsertDefaultServer(VirtualServer *server, int socketFd)
{
	std::map<int, VirtualServer *>::iterator dvserver = this->defaultServer.find(socketFd);
	if (dvserver != this->defaultServer.end())
	{
		if (dvserver->second->getServerNames().size() == 0 && server->getServerNames().size() == 0)
		{
			std::cerr << "conflict default server (uname server) already exist on port\n";
			return;
		}
	}
	this->defaultServer[socketFd] = server;
}

void Event::insertServerNameMap(ServerNameMap_t &serverNameMap, VirtualServer *server, int socketFd)
{
	const std::set<std::string> &serverNames = server->getServerNames();
	if (serverNames.size() == 0) // if there is no server name  uname will be the
		this->InsertDefaultServer(server, socketFd); // default server in that port
	else
	{
		std::set<std::string>::iterator it = serverNames.begin();
		for (; it != serverNames.end(); it++)
		{
			if (serverNameMap.find(*it) != serverNameMap.end())
			{
				std::cerr << "confilict: server name already exist on some port host: " << *it << std::endl;
				continue;
			}
			serverNameMap[*it] = server;
		}
	}
	if (this->defaultServer.find(socketFd) == this->defaultServer.end())
		this->InsertDefaultServer(server, socketFd);
}

void Event::init()
{
	std::vector<VirtualServer> &virtualServers = ctx->getServers(); // get all  VServer
	for (size_t i = 0; i < virtualServers.size(); i++)
	{
		SocketAddrSet_t &socketAddr = virtualServers[i].getAddress();
		if (socketAddr.empty())
			throw std::runtime_error("Error: server should listen atleast one port\n");
		SocketAddrSet_t::iterator it = socketAddr.begin();
		for (; it != socketAddr.end(); it++)
		{
			int socketFd;
			if (this->socketMap.find(*it) == this->socketMap.end()) // no need to create socket if it already exists
			{
				socketFd = this->CreateSocket(it);
				this->socketMap[*it] = socketFd;
			}
			else
				socketFd = this->socketMap.at(*it);
			VirtualServerMap_t::iterator it = this->virtuaServers.find(socketFd);
			if (it == this->virtuaServers.end()) // create empty for socket map if not exist
			{
				this->virtuaServers[socketFd] = ServerNameMap_t();
				it = this->virtuaServers.find(socketFd); // take the reference of server map;
			}
			ServerNameMap_t &serverNameMap = it->second;
			this->insertServerNameMap(serverNameMap, &virtualServers[i], socketFd);
		}
	}
}

static void error_panic(const std::string &func, int sock, struct addrinfo *result)
{
	if (result)
		freeaddrinfo(result);
	if (sock >= 0)
		close(sock);
	throw std::runtime_error("Error: " + func + " " + strerror(errno));
}
int Event::CreateSocket(SocketAddrSet_t::iterator &address)
{
	int optval = 1;

	struct addrinfo *result, hints;
	hints.ai_family = AF_INET; //  IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP socket
	hints.ai_flags = AI_PASSIVE; // for binding
	hints.ai_protocol = 0; // TCP/IP proto
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	int r = getaddrinfo(address->host.data(), address->port.data(), &hints, &result);
	if (r != 0)
		throw std::runtime_error("Error :getaddrinfo: " + std::string(gai_strerror(r)));
	int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (socket_fd < 0)
		error_panic("socket", -1, result);
	if (this->setNonBlockingIO(socket_fd, true) < 0)
		error_panic("setNonBlockingIO", socket_fd, result);
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		error_panic("setsockopt", socket_fd, result);
	if (bind(socket_fd, result->ai_addr, result->ai_addrlen) < 0)
		error_panic("bind", socket_fd, result);
	this->numOfSocket++;
	this->sockAddrInMap[socket_fd] = *result->ai_addr;
	freeaddrinfo(result);
	return (socket_fd);
}

int Event::setNonBlockingIO(int sockfd, bool sockserver)
{
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK | FD_CLOEXEC) < 0)
		return (-1);
	if (sockserver)
		return (0);
	int sock_buf_size = SOCK_BUFFER_SIZE; // set socket send and recv buffer size
	int result = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sock_buf_size, sizeof(sock_buf_size));
	if (result < 0)
		return (-1);
	result = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sock_buf_size, sizeof(sock_buf_size));
	if (result < 0)
		return (-1);
	result = setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &sock_buf_size, sizeof(sock_buf_size));
	if (result < 0)
		return (-1);
	return (0);
}

bool Event::Listen(int socketFd)
{
	if (listen(socketFd, this->MAX_CONNECTION_QUEUE) < 0)
		return (false);
	return (true);
}

bool Event::Listen()
{
	SocketMap_t::iterator it = this->socketMap.begin();

	for (; it != this->socketMap.end(); it++)
	{
		if (!this->Listen(it->second))
			throw std::runtime_error(
				"could not listen: " + it->first.host + ":" + it->first.port + " " + strerror(errno));
		std::cout << "server listen on " << it->first.host + ":" + it->first.port << std::endl;
	}
	return true;
}

void Event::CreateChangeList()
{
	SockAddr_in::iterator it = this->sockAddrInMap.begin();
	for (int i = 0; it != this->sockAddrInMap.end(); it++, i++)
		EV_SET(&this->eventChangeList[i], it->first, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(this->kqueueFd, this->eventChangeList, this->numOfSocket, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent failed: could not regester server event: " + std::string(strerror(errno)));
	delete this->eventChangeList;
	this->eventChangeList = NULL;
}

void Event::initIOmutltiplexing()
{
	this->evList = new struct kevent[this->MAX_EVENTS];
	this->eventChangeList = new struct kevent[this->numOfSocket];
	this->kqueueFd = kqueue();
	if (this->kqueueFd < 0)
		throw std::runtime_error("kqueue faild: " + std::string(strerror(errno)));
	this->CreateChangeList();
}
int Event::newConnection(int socketFd, Connections &connections)
{
	int newSocketFd = accept(socketFd, NULL, NULL);
	if (newSocketFd < 0)
		return (-1);
	if (this->setNonBlockingIO(newSocketFd, false))
		return (close(newSocketFd), -1);
	struct kevent ev_set[3];
	EV_SET(&ev_set[0], newSocketFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	EV_SET(&ev_set[1], newSocketFd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);
	EV_SET(
		&ev_set[2],
		newSocketFd,
		EVFILT_TIMER,
		EV_ADD | EV_ENABLE | EV_ONESHOT,
		NOTE_SECONDS,
		this->ctx->getKeepAliveTime(),
		NULL);
	if (kevent(this->kqueueFd, ev_set, 3, NULL, 0, NULL) < 0)
		return (close(newSocketFd), -1);
	connections.addConnection(newSocketFd, socketFd);
	return (newSocketFd);
}

void Event::setWriteEvent(Client *client, uint16_t flags)
{
	if (client->writeEventState == flags)
		return;
	struct kevent ev;
	EV_SET(&ev, client->getFd(), EVFILT_WRITE, EV_ADD | flags, 0, 0, NULL);
	if (kevent(this->kqueueFd, &ev, 1, NULL, 0, NULL) < 0)
		throw Event::EventExpection("kevent : client write event :");
	client->writeEventState = flags;
}

void Event::ReadEvent(const struct kevent *ev)
{
	if ((ev->flags & EV_EOF) && ev->data <= 0)
		connections.closeConnection(ev->ident);
	else
	{
		Client *client = connections.requestHandler(ev->ident, ev->data);
		if (!client)
			return;
		while (!client->request.eof && client->request.state != REQ_ERROR)
		{
			client->request.feed();
			if (!client->request.data.back()->isRequestLineValidated() && client->request.state == BODY)
			{
				client->request.decodingUrl();
				client->request.splitingQuery();
				client->request.location = this->getLocation(client);
				client->request.validateRequestLine();
				client->request.data.back()->isRequestLineValid = 1;
				this->KeepAlive(client);
			}
		}
		client->request.eof = 0;
		this->setWriteEvent(client, EV_ENABLE);
	}
}

int Event::RegisterNewProc(Client *client)
{
	HttpResponse &response = client->response;
	CGIProcess cgi;
	Proc proc = cgi.RunCGIScript(response);
	if (proc.pid < 0)
		return (-1);
	struct kevent ev[3];
	int evSize = 3;
	EV_SET(&ev[0], proc.pid, EVFILT_PROC, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_EXIT, 0, NULL);
	EV_SET(
		&ev[1],
		proc.pid,
		EVFILT_TIMER,
		EV_ADD | EV_ENABLE | EV_ONESHOT,
		NOTE_SECONDS,
		this->ctx->getCGITimeOut(),
		(void *)(size_t)proc.pid);
	EV_SET(&ev[2], proc.fout, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(size_t)proc.pid);
	if (kevent(this->kqueueFd, ev, evSize, 0, 0, NULL) < 0)
	{
		proc.die();
		throw Event::EventExpection("Kevent: CGI process: ");
	}
	client->cgi_pid = proc.pid;
	proc.client = client->getFd();
	proc.input = client->request.data.front()->bodyHandler.bodyFile;
	this->procs[proc.pid] = proc;
	this->setWriteEvent(client, EV_DISABLE);
	return (0);
}

void Event::WriteEvent(const struct kevent *ev)
{
	if ((ev->flags & EV_EOF )&& ev->data <= 0)
	{
		connections.closeConnection(ev->ident);
		return;
	}
	ClientsIter kv = connections.clients.find(ev->ident);
	if (kv == connections.clients.end())
		return;
	Client *client = &(kv->second);
	if (client->request.data.size() == 0
		|| (client->request.data.front()->state != REQUEST_FINISH && client->request.data.front()->state != REQ_ERROR))
		return;
	if (client->response.state == START)
	{
		client->response.location = this->getLocation(client);
		client->respond(ev->data, 0);
		client->response.logResponse();
	}
	if (client->response.state == START_CGI_RESPONSE)
		client->respond(ev->data, 0);
	if (client->response.state == CGI_EXECUTING && !this->RegisterNewProc(client))
		return;
	if (client->response.state == WRITE_BODY)
	{
		client->response.eventByte = ev->data;
		client->response.sendBody(client->response.bodyType);
	}
	if (client->response.state == ERROR)
		client->handleResponseError();
	if (client->response.state == END_BODY)
	{
		bool conn = (!client->response.keepAlive || client->response.state == ERROR);
		client->response.clear();
		delete client->request.data[0];
		client->request.data.erase(client->request.data.begin());
		if (conn)
			this->connections.closeConnection(client->getFd());
		else if (client->request.data.size() == 0)
		{
			this->setWriteEvent(client, EV_DISABLE);
			this->KeepAlive(client);
		}
	}
}

void Event::ReadPipe(const struct kevent *ev)
{
	const char seq[4] = {'\r', '\n', '\r', '\n'};
	if ((ev->flags & EV_EOF) && ev->data == 0)
		return;
	ProcMap_t::iterator p = this->procs.find((size_t)ev->udata);
	Proc &proc = p->second;
	Client *client = this->connections.getClient(proc.client);
	if (!client)
		return proc.die();
	HttpResponse *response = &client->response;
	int read_size = std::min(ev->data, CGI_BUFFER_SIZE);
	int r = read(ev->ident, proc.buffer.data() + proc.offset, read_size);
	if (r < 0)
		return response->setHttpResError(500, "Internal server Error"), proc.die();
	read_size += proc.offset;
	proc.offset = 0;
	if (proc.outToFile)
	{
		if (proc.writeBody(proc.buffer.data(), read_size) < 0)
			return proc.die(); // kill cgi
		return;
	}
	else if (read_size < 4)
	{
		proc.offset = read_size;
		return;
	}
	std::vector<char> &buffer = proc.buffer;
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.begin() + read_size, seq, seq + 4);
	if (it != (buffer.begin() + read_size))
	{
		it = it + 4;
		response->CGIOutput.insert(response->CGIOutput.end(), buffer.begin(), it); 
		if (proc.writeBody(&(*it), buffer.begin() + read_size - it) < 0)
			return response->setHttpResError(500, "Internal server Error"), proc.die(); // kill cgi
		proc.outToFile = true;
	}
	else
	{
		response->CGIOutput.insert(response->CGIOutput.end(), buffer.begin(), buffer.begin() + read_size - 3);
		int j = 0;
		for (int i = read_size - 3; i < read_size; i++)
			proc.buffer[j++] = proc.buffer[i];
		proc.offset = 3;
	}
}

void Event::PorcTimerEvent(const struct kevent *ev)
{
	ProcMap_t::iterator p = this->procs.find((size_t)ev->udata);
	if (p == this->procs.end())
		return;
	Proc &proc = p->second;
	proc.state = Proc::TIMEOUT;
	proc.die();
}

int Event::waitProc(int pid)
{
	int status;
	int signal;

	waitpid(pid, &status, 0); // this event only run if process has finish so waitpid would not block
	signal = WIFSIGNALED(status); // check if process exist normally
	status = WEXITSTATUS(status); // check exist state
	struct kevent event;
	EV_SET(&event, pid, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
	kevent(this->kqueueFd, &event, 1, NULL, 0, NULL); // if timer has not been trigger better to delete it
	return (signal || status);
}

void Event::ProcEvent(const struct kevent *ev)
{
	int status = this->waitProc(ev->ident);
	ProcMap_t::iterator p = this->procs.find(ev->ident);
	if (p == this->procs.end())
		return;
	Proc &proc = p->second;
	Client *client = this->connections.getClient(proc.client);
	if (!client)
		return this->deleteProc(p);
	this->setWriteEvent(client, EV_ENABLE);
	if (proc.state == Proc::TIMEOUT)
		return (
			client->response.setHttpResError(504, "Gateway Timeout"),
			client->response.logResponse(),
			this->deleteProc(p));
	else if (status)
		return (
			client->response.setHttpResError(502, "Bad Gateway"), client->response.logResponse(), this->deleteProc(p));
	client->response.state = START_CGI_RESPONSE;
	proc.clean();
	client->response.cgiOutFile = proc.output;
	this->deleteProc(p);
}

static void serverError(const char *error)
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
	std::cerr << red;
	std::cerr << ss.str() << " ";
	std::cerr << "Intrenal Server Error: " << error << "\n";
	std::cerr << _reset;
}

void Event::eventLoop()
{
	int nev;

	connections.init(this->ctx, this->kqueueFd);
	while (Event::Running(0))
	{
		nev = kevent(this->kqueueFd, NULL, 0, this->evList, MAX_EVENTS, NULL);
		if (nev < 0)
			throw std::runtime_error("kevent failed: " + std::string(strerror(errno)));
		for (int i = 0; i < nev; i++)
		{
			const struct kevent *ev = &this->evList[i];
			if (this->checkNewClient(ev->ident))
			{
				if (this->newConnection(ev->ident, connections) < 0)
					serverError("Accept faild");
				continue;
			}
			try
			{
				if (ev->fflags & EV_ERROR)
					this->connections.closeConnection(ev->ident);
				else if (ev->filter == EVFILT_READ && ev->udata)
					this->ReadPipe(ev);
				else if (ev->filter == EVFILT_READ)
					this->ReadEvent(ev);
				else if (ev->filter == EVFILT_WRITE)
					this->WriteEvent(ev);
				else if (ev->filter == EVFILT_PROC)
					this->ProcEvent(ev);
				else if (ev->filter == EVFILT_TIMER && ev->udata)
					this->PorcTimerEvent(ev);
				else if (ev->filter == EVFILT_TIMER)
					this->connections.closeConnection(ev->ident);
			}
			catch (const CGIProcess::ChildException &e)
			{
				serverError(e.what());
				throw;
			}
			catch (const std::exception &e)
			{
				serverError(e.what());
				this->connections.closeConnection(ev->ident);
			}
		}
	}
}

bool Event::checkNewClient(int socketFd)
{
	return (this->sockAddrInMap.find(socketFd) != this->sockAddrInMap.end());
}

void Event::deleteProc(ProcMap_t::iterator &it)
{
	Proc &p = it->second;
	p.clean();
	this->procs.erase(it);
}
Location *Event::getLocation(Client *client)
{
	VirtualServer *Vserver;
	int serverfd;
	bool IsDefault = true;

	serverfd = client->getServerFd();
	const std::string &path = client->getPath();
	std::string host = client->getHost();
	ServerNameMap_t serverNameMap = this->virtuaServers.find(serverfd)->second; // always exist
	ServerNameMap_t::iterator _Vserver = serverNameMap.find(host);
	if (_Vserver == serverNameMap.end())
	{
		Vserver = this->defaultServer.find(client->getServerFd())->second;
		IsDefault = true;
	}
	else
		Vserver = _Vserver->second;
	Location *location = Vserver->getRoute(path);
	if (!location)
		return (NULL);
	else if (IsDefault)
		client->response.server_name = *Vserver->getServerNames().begin();
	struct sockaddr *addr = &this->sockAddrInMap.find(client->getServerFd())->second;
	struct sockaddr_in *addr2 = (struct sockaddr_in *)addr;
	client->response.server_port = ntohs(addr2->sin_port);
	return (location);
}

void Event::KeepAlive(Client *client)
{
	struct kevent ev;

	EV_SET(
		&ev,
		client->getFd(),
		EVFILT_TIMER,
		EV_ADD | EV_ENABLE | EV_ONESHOT,
		NOTE_SECONDS,
		this->ctx->getKeepAliveTime(),
		NULL);
	if (kevent(this->kqueueFd, &ev, 1, NULL, 0, NULL) < 0)
		throw Event::EventExpection("kevent : client Timer: ");
}

Event::EventExpection::EventExpection(const std::string &msg) throw()
{
	this->msg = msg + strerror(errno);
}

const char *Event::EventExpection::what() const throw()
{
	return (this->msg.data());
}

Event::EventExpection::~EventExpection() throw() {}

bool Event::Running(bool state)
{
	static bool _state = true;
	if (state)
		_state = !_state;
	return (_state);
}
