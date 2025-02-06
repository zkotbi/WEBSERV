#ifndef Server_HPP
#define Server_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <set>
#include <vector>
#include "DataType.hpp"
#include "Location.hpp"
#include "Trie.hpp"

class VirtualServer
{
  public:
	struct SocketAddr
	{
		std::string		host; // wich interface bind to (require ip address )
		std::string		port;
		bool operator<(const SocketAddr &rhs) const // code from the great chatGpt
		{
			if (host != rhs.host)
				return host < rhs.host;
			return port < rhs.port;
		}
		SocketAddr();
	};

  private:
	std::vector<int>		sockets;
	std::set<std::string>	serverNames; // todo as trie
	Trie					routes;
	std::set<VirtualServer::SocketAddr>	listen;

  public:
	GlobalConfig			globalConfig;
	void deleteRoutes();
	~VirtualServer();
	VirtualServer();
	void setListen(Tokens &Token, Tokens &end);
	std::set<SocketAddr> &getAddress();
	const std::set<std::string> &getServerNames();
	bool isListen(const SocketAddr &addr) const;
	void setServerNames(Tokens &Token, Tokens &end);
	void pushLocation(Tokens &tokens, Tokens &end);
	void parseTokens(Tokens &tokens, Tokens &end);
	void insertRoute(Location &location);
	Location *getRoute(const std::string &path);
	void init();
};

#endif
