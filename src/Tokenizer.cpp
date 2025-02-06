#include "Tokenizer.hpp"
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include "DataType.hpp"
#include "ServerContext.hpp"
#include "VirtualServer.hpp"

Tokenizer::Tokenizer()
{
}

Tokenizer::~Tokenizer()
{
}

void Tokenizer::readConfig(const std::string path)
{
	std::string line;
	std::ifstream configFile;

	configFile.open(path.c_str());
	if (!configFile.is_open())
		throw ParserException("WebServ: could not open file: " + path);
	while (std::getline(configFile, line))
	{
		line.push_back('\n'); // getline trim last \n in line
		this->config += line;
	}
	if (!configFile.eof())
	{
		configFile.close();
		throw ParserException("WebServ: could not read all file: " + path);
	}
	configFile.close();
}

bool Tokenizer::IsId(char c)
{
	return (c == '{' || c == '}' || c == ';');
}

bool Tokenizer::IsSpace(char c) const
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

std::string Tokenizer::getNextToken()
{
	static size_t offset;
	std::string token = "";

	while (offset < this->config.size() && IsSpace(this->config.at(offset)))
		offset++;
	if (offset < this->config.size() && IsId(this->config.at(offset)))
		return std::string(1, this->config.at(offset++));
	while (offset < this->config.size() && !IsSpace(this->config.at(offset)) && !this->IsId(this->config.at(offset)))
		token.push_back(this->config[offset++]);
	return token;
}

void Tokenizer::CreateTokens()
{
	std::string token;
	while ((token = this->getNextToken()) != "")
		this->tokens.push_back(token);
}

void Tokenizer::parseConfig(ServerContext *context)
{
	Tokens token; // typedef of vector<string>::it;
	Tokens end;

	end = this->tokens.end();
	if (this->tokens.size() == 0)
		throw ParserException("Error: empty config");
	for (token = this->tokens.begin(); token != end;)
	{
		if (*token == "server")
			context->pushServer(token, end);
		else if (*token == "types")
			context->pushTypes(token, end);
		else if (*token == "keepalive_timeout")
			context->setKeepAlive(token, end);
		else if (*token == "cgi_timeout")
			context->setCGITimeout(token, end);
		else
			context->parseTokens(token, end);
	}
}

Tokenizer::ParserException::ParserException(std::string msg)
{
	message = msg;
}

Tokenizer::ParserException::ParserException() : message("") {}

const char *Tokenizer::ParserException::what() const throw()
{
	return (message.c_str());
}

Tokenizer::ParserException::~ParserException() throw() {}
