#include "VirtualServer.hpp"
#include <sys/socket.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include "Location.hpp"
#include "Tokenizer.hpp"

VirtualServer::VirtualServer() {}

VirtualServer::~VirtualServer() {}

VirtualServer::SocketAddr::SocketAddr() {};

void VirtualServer::pushLocation(Tokens &token, Tokens &end)
{
	Location location;

	this->globalConfig.validateOrFaild(token, end);
	location.setPath(this->globalConfig.consume(token, end));
	if (token == end || *token != "{")
		throw Tokenizer::ParserException("Unexpected Token: " + *token);
	token++;
	while (token != end && *token != "}")
		location.parseTokens(token, end);

	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	token++;
	this->routes.insert(location);
}
void VirtualServer::deleteRoutes()
{
	this->routes.deleteNode();
}

int parseNumber(std::string &str)
{
	int number = 0;

	if (str.empty())
		return (-1);
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] < '0' || str[i] > '9')
			return (-1);
		number = number * 10 + str[i] - 48;
		if (number > (1 << 16))
			return (-1);
	}
	return (number);
}

void VirtualServer::setListen(Tokens &token, Tokens &end)
{
	SocketAddr socketAddr;

	this->globalConfig.validateOrFaild(token, end);
	size_t pos = token->find(':');
	if (pos == 0 || pos == token->size() - 1)
		throw Tokenizer::ParserException("Unvalid [host:]port " + *token);

	socketAddr.host = "0.0.0.0";
	if (pos != std::string::npos)
		socketAddr.host = token->substr(0, pos);
	else
		pos = 0;
	socketAddr.port = token->substr(pos + (pos != 0), token->size() - pos);
	listen.insert(socketAddr);
	token++;
	this->globalConfig.CheckIfEnd(token, end);
}

std::set<VirtualServer::SocketAddr> &VirtualServer::getAddress()
{
	return (this->listen);
}

bool VirtualServer::isListen(const SocketAddr &addr) const
{
	return (listen.find(addr) != listen.end());
}

void VirtualServer::setServerNames(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);

	while (token != end && *token != ";")
		serverNames.insert(this->globalConfig.consume(token, end));
	this->globalConfig.CheckIfEnd(token, end);
}

void VirtualServer::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	if (*token == "listen")
		this->setListen(token, end);
	else if (*token == "server_name")
		this->setServerNames(token, end);
	else if (*token == "location")
		this->pushLocation(token, end);
	else
		globalConfig.parseTokens(token, end);
}

const std::set<std::string> &VirtualServer::getServerNames()
{
	return (this->serverNames);
}

Location *VirtualServer::getRoute(const std::string &path)
{
	return (this->routes.findPath(path));
}

void VirtualServer::init()
{
	this->routes.init(this->globalConfig);
}
