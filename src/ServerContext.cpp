#include "ServerContext.hpp"
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <sstream>
#include "DataType.hpp"
#include "Tokenizer.hpp"

ServerContext::ServerContext() : globalConfig(0, "/tmp")
{
	std::string types[] = {
		"application/octet-stream",
		"text/plain",
		"text/html",
		"text/html",
		"text/html",
		"text/css",
		"text/xml",
		"image/gif",
		"image/jpeg",
		"image/jpeg",
		"application/javascript",
		"video/mp4",
	};

	const std::string ext[] = {
		".",
		"txt",
		"html",
		"shtml",
		"htm",
		"css",
		"xml",
		"gif",
		"jpeg",
		"jpg",
		"js",
		"mp4",
	};
	for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++)
		this->types[ext[i]] = types[i];
	this->keepAliveTimeout = 75;
	this->CGITimeOut = 13;
}

ServerContext::~ServerContext()
{
	for (size_t i = 0; i < this->servers.size(); i++)
		this->servers[i].deleteRoutes();
}
std::vector<VirtualServer> &ServerContext::getServers()
{
	return this->servers;
}
void ServerContext::parseTokens(Tokens &token, Tokens &end)
{
	this->globalConfig.parseTokens(token, end);
}
void ServerContext::pushServer(Tokens &token, Tokens &end)
{
	token++;
	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	else if (*token != "{")
		throw Tokenizer::ParserException("Unexpact token: " + *token);
	token++;
	this->servers.push_back(VirtualServer()); // push empty VirtualServer to keep
											  // the reference in http object in case of exception to cause memory leak
	VirtualServer &server = this->servers.back(); // grants access to the last element
	while (token != end && *token != "}")
	{
		if (*token == "location")
			server.pushLocation(token, end);
		else
			server.parseTokens(token, end);
	}
	if (token == end || *token != "}")
		throw Tokenizer::ParserException("Unexpected end of file");
	token++;
}

std::vector<VirtualServer> &ServerContext::getVirtualServers()
{
	return this->servers;
}

void ServerContext::addTypes(Tokens &token, Tokens &end)
{
	std::string type = this->globalConfig.consume(token, end);
	std::string ext = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	this->types[ext] = type;
}

void ServerContext::pushTypes(Tokens &token, Tokens &end)
{
	token++;
	if (token == end || *token != "{")
		throw Tokenizer::ParserException("Unexpected  " + ((token == end) ? "end of file" : "token " + *token));
	token++;
	while (token != end && *token != "}")
		this->addTypes(token, end);
	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	token++;
}
const std::string &ServerContext::getType(const std::string &ext)
{
	std::map<std::string, std::string>::iterator kv = this->types.find(ext);

	if (kv == this->types.end())
		return (this->types.find(".")->second);
	return (kv->second);
}
void ServerContext::init()
{
	if (this->servers.size() == 0)
		throw Tokenizer::ParserException("No Virtual Server has been define");

	for (size_t i = 0; i < this->servers.size(); i++)
	{
		this->servers[i].globalConfig.copy(this->globalConfig);
		this->servers[i].init();
	}
}

void ServerContext::setKeepAlive(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
	std::string time = this->globalConfig.consume(token, end);

	std::stringstream ss;
	ss << time;
	ss >> this->keepAliveTimeout;
	if (ss.fail() || !ss.eof() || this->keepAliveTimeout < 3)
		throw Tokenizer::ParserException("invalid keepAlive value " + time);
	this->globalConfig.CheckIfEnd(token, end);
}

void ServerContext::setCGITimeout(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
	std::string time = this->globalConfig.consume(token, end);

	std::stringstream ss;
	ss << time;
	ss >> this->CGITimeOut;
	if (ss.fail() || !ss.eof() || this->CGITimeOut < 3)
		throw Tokenizer::ParserException("invalid CGITimeOut value " + time);
	this->globalConfig.CheckIfEnd(token, end);
}

int ServerContext::getCGITimeOut() const
{
	return (this->CGITimeOut);
}


int ServerContext::getKeepAliveTime() const
{
	return (this->keepAliveTimeout); }
