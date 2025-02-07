
#include "Location.hpp"
#include <fcntl.h>
#include <sys/_types/_s_ifmt.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include "DataType.hpp"
#include "Tokenizer.hpp"
Location::Location()
{
	this->cgiMap["."] = ""; // mean no cgi
	this->isRedirection = false;
	this->methods = GET; // default method is GET only can be overwritten
	this->upload_file_path = "/tmp/"; // default upload loaction if post method was allowed
	this->maxBodySize = LONG_MAX;
}

Location &Location::operator=(const Location &location)
{
	this->path = location.path;
	this->redirect = location.redirect;
	this->globalConfig.copy(location.globalConfig);
	this->isRedirection = location.isRedirection;
	this->cgiMap = location.cgiMap;
	this->methods = location.methods;
	this->upload_file_path = location.upload_file_path;
	this->maxBodySize = location.maxBodySize;
	return *this;
}
void Location::setPath(std::string &path)
{
	this->path = path;
}

bool GlobalConfig::isValidStatusCode(std::string &str)
{
	std::stringstream ss;
	ss << str;
	int status;
	ss >> status;
	if (ss.fail() || !ss.eof())
		return (false);
	return (status >= 100 && status <= 599);
}

void Location::setRedirect(Tokens &token, Tokens &end)
{
	int code;
	std::stringstream ss;
	this->globalConfig.validateOrFaild(token, end);
	std::string status = this->globalConfig.consume(token, end);
	std::string url = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	ss << status;
	ss >> code;
	if (ss.fail() || !ss.eof() || code < 300 || code > 399)
		throw Tokenizer::ParserException("Invalid  status: " + status + " code in Redirection: should be 3xx code");
	this->redirect.status = status;
	this->redirect.url = url;
	this->isRedirection = true;
}

void Location::parseTokens(Tokens &token, Tokens &end)
{
	if (*token == "return")
		this->setRedirect(token, end);
	else if (*token == "cgi_path")
		this->setCGI(token, end);
	else if (*token == "allow")
		this->setMethods(token, end);
	else if (*token == "client_upload_path")
		this->setUploadPath(token, end);
	else if (*token == "max_body_size")
		this->setMaxBodySize(token, end);
	else
		this->globalConfig.parseTokens(token, end);
}

void Location::setCGI(Tokens &token, Tokens &end)
{
	std::string cgi_path;
	std::string cgi_ext;
	struct stat buf;

	this->globalConfig.validateOrFaild(token, end);
	cgi_ext = this->globalConfig.consume(token, end);
	cgi_path = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	if (cgi_ext[0] != '.')
		throw Tokenizer::ParserException("invalid CGI ext " + cgi_ext);
	if (cgi_path[0] != '/')
		throw Tokenizer::ParserException("Invalid CGI path " + cgi_path + ": " + "path need to be absolute path");
	int r = stat(cgi_path.data(), &buf);
	if (r != 0)
		throw Tokenizer::ParserException("Invalid CGI path " + cgi_path + ": " + std::string(strerror(errno)));
	else if (S_ISDIR(buf.st_mode))
		throw Tokenizer::ParserException("Invalid CGI path " + cgi_path);
	else if (!(buf.st_mode & S_IXUSR))
		throw Tokenizer::ParserException("Invalid CGI path " + cgi_path+ " : not executable");
	this->cgiMap[cgi_ext] = cgi_path;
}

const std::string &Location::getCGIPath(const std::string &ext)
{
	std::map<std::string, std::string>::iterator kv;
	kv = this->cgiMap.find(ext); 
	if (kv == this->cgiMap.end())
		return (this->cgiMap.find(".")->second);
	return (kv->second);
}

bool Location::HasRedirection() const
{
	return (this->isRedirection);
}
const Location::Redirection &Location::getRedirection() const
{
	return (this->redirect);
}

void Location::setMaxBodySize(Tokens &token, Tokens &end)
{
	std::string maxBodySize;
	std::stringstream ss;
	this->globalConfig.validateOrFaild(token, end);
	maxBodySize = this->globalConfig.consume(token, end);
	ss << maxBodySize;
	ss >> this->maxBodySize;
	if (ss.fail() || !ss.eof() || this->maxBodySize <= 0)
		throw Tokenizer::ParserException("Invalid max body size " + maxBodySize);
	this->globalConfig.CheckIfEnd(token, end);
}
long Location::getMaxBody() const
{
	return (this->maxBodySize);
}
bool Location::isMethodAllowed(int method) const
{
	return ((this->methods & method) != 0);
}

void Location::setMethods(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
	this->methods = 0;
	while (token != end && *token != ";")
	{
		if (*token == "GET")
			this->methods |= GET;
		else if (*token == "POST")
			this->methods |= POST;
		else if (*token == "DELETE")
			this->methods |= DELETE;
		else
			throw Tokenizer::ParserException("Invalid or un support method: " + *token);
		token++;
	}
	this->globalConfig.CheckIfEnd(token, end);
	if (this->methods == 0)
		throw Tokenizer::ParserException("empty  method list");
}
const std::string &Location::getPath()
{
	return (this->path);
}

const std::string &Location::getFileUploadPath()
{
	return (this->upload_file_path);
}

void Location::setUploadPath(Tokens &token, Tokens &end)
{
	struct stat buf;

	this->globalConfig.validateOrFaild(token, end);
	this->upload_file_path = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	if (this->upload_file_path[upload_file_path.size() -1] != '/')
		this->upload_file_path.push_back('/');
	if (stat(this->upload_file_path.data(), &buf) != 0)
		throw Tokenizer::ParserException("Upload path does directory does not exist");
	if (S_ISDIR(buf.st_mode) == 0)
		throw Tokenizer::ParserException("Upload Path is not a directory");
	if (access(this->upload_file_path.data(), W_OK) != 0)
		throw Tokenizer::ParserException("invalid Upload path directory");
}
