#ifndef ServerContext_HPP
#define ServerContext_HPP

#include "DataType.hpp"
#include "VirtualServer.hpp"

class ServerContext
{
  private:
	int CGITimeOut;
	typedef std::map<std::string, std::string> Type;
	int keepAliveTimeout;
	GlobalConfig globalConfig;

	std::vector<VirtualServer> servers;
	Type types;
	void addTypes(Tokens &token, Tokens &end);

 public:
	ServerContext();
	~ServerContext();

	void init();
	void pushServer(Tokens &token, Tokens &end);
	void pushTypes(Tokens &token, Tokens &end);
	std::vector<VirtualServer> &getServers();
	void parseTokens(Tokens &token, Tokens &end);
	std::vector<VirtualServer> &getVirtualServers();

	const std::string &getType(const std::string &ext);
	void setKeepAlive(Tokens &token, Tokens &end);
	void setCGITimeout(Tokens &token, Tokens &end);
	int	getKeepAliveTime() const;
	int getCGITimeOut() const;

};

#endif
