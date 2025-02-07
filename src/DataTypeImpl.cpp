#include <Tokenizer.hpp>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "DataType.hpp"
#include <algorithm>

GlobalConfig::GlobalConfig()
{
	this->autoIndex = -1;
	this->errorPages["."] = "";
	this->IsAlias = false;
}

GlobalConfig::GlobalConfig(int autoIndex, const std::string &upload_file_path)
{
	this->autoIndex = autoIndex;
	this->upload_file_path = upload_file_path;
	this->errorPages["."] = "";
	this->IsAlias = false;
}

GlobalConfig::GlobalConfig(const GlobalConfig &other)
{
	*this = other;
}
GlobalConfig &GlobalConfig::operator=(const GlobalConfig &other)
{
	if (this == &other)
		return (*this);
	this->root = other.root;
	this->autoIndex = other.autoIndex;
	this->upload_file_path = other.upload_file_path;
	this->indexes = other.indexes;
	this->IsAlias = other.IsAlias;
	this->errorPages = other.errorPages;
	return *this;
}
void GlobalConfig::copy(const GlobalConfig &other)
{
	if (this == &other)
		return;
	if (root.empty())
	{
		root = other.root;
		this->IsAlias = other.IsAlias;
	}
	if (upload_file_path.empty())
		upload_file_path = other.upload_file_path;
	if (autoIndex == -1)
		autoIndex = other.autoIndex;

	std::map<std::string, std::string>::const_iterator kv = other.errorPages.begin();
	for (; kv != other.errorPages.end(); kv++)
	{
		if (this->errorPages.find(kv->first) == this->errorPages.end())
		{
			this->errorPages[kv->first] = kv->second;
		}
	}
	for (size_t i = 0; i < other.indexes.size(); i++)
	{
		std::vector<std::string>::const_iterator it =
			std::find(this->indexes.begin(), this->indexes.end(), other.indexes[i]);
		if (it == this->indexes.end())
			this->indexes.push_back(other.indexes[i]);
	}
}
GlobalConfig::~GlobalConfig() {}

void GlobalConfig::setRoot(Tokens &token, Tokens &end)
{
	struct stat buf;

	if (!this->root.empty())
		throw Tokenizer::ParserException("Root directive is duplicate");
	validateOrFaild(token, end);
	this->root = consume(token, end);
	if (stat(this->root.c_str(), &buf) != 0)
		throw Tokenizer::ParserException("Root directory does not exist: " + this->root);
	if (S_ISDIR(buf.st_mode) == 0)
		throw Tokenizer::ParserException("Root is not a directory");
	CheckIfEnd(token, end);
}

std::string GlobalConfig::getRoot() const
{
	return this->root; // Return the root
}

void GlobalConfig::setAutoIndex(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);

	if (*token == "on")
		this->autoIndex = true;
	else if (*token == "off")
		this->autoIndex = false;
	else
		throw Tokenizer::ParserException("Invalid value for autoindex");
	token++;
	CheckIfEnd(token, end);
}

bool GlobalConfig::getAutoIndex() const
{
	return this->autoIndex; // Return autoIndex
}

void GlobalConfig::setIndexes(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);
	while (token != end && *token != ";")
		this->indexes.push_back(consume(token, end));
	CheckIfEnd(token, end);
}

bool GlobalConfig::IsId(std::string &token)
{
	return (token == ";" || token == "}" || token == "{");
}

void GlobalConfig::validateOrFaild(Tokens &token, Tokens &end)
{
	token++;
	if (token == end || IsId(*token))
		throw Tokenizer::ParserException("Unexpected token: " + (token == end ? "end of file" : *token));
}

void GlobalConfig::CheckIfEnd(Tokens &token, Tokens &end)
{
	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	else if (*token != ";")
		throw Tokenizer::ParserException("Unexpected `;` found: " + *token);
	token++;
}

std::string &GlobalConfig::consume(Tokens &token, Tokens &end)
{
	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	if (IsId(*token))
		throw Tokenizer::ParserException("Unexpected token: " + *token);
	return *token++;
}
bool GlobalConfig::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw Tokenizer::ParserException("Unexpected end of file");
	else if (*token == "root")
		this->setRoot(token, end);
	else if (*token == "autoindex")
		this->setAutoIndex(token, end);
	else if (*token == "index")
		this->setIndexes(token, end);
	else if (*token == "error_page")
		this->setErrorPages(token, end);
	else if (*token == "alias")
		this->setAlias(token, end);
	else
		throw Tokenizer::ParserException("Invalid token: " + *token);
	return (true);
}

void GlobalConfig::setErrorPages(Tokens &token, Tokens &end)
{
	std::vector<std::string> vec;
	std::string content;

	std::string str;
	this->validateOrFaild(token, end);
	while (token != end && *token != ";")
		vec.push_back(this->consume(token, end));
	if (vec.size() <= 1)
		throw Tokenizer::ParserException("Invalid error page define");
	this->CheckIfEnd(token, end);
	if (access(vec.back().data(), F_OK | R_OK) == -1)
		throw Tokenizer::ParserException("file does not exist" + vec.back());
	for (size_t i = 0; i < vec.size() - 1; i++)
	{
		if (!this->isValidStatusCode(vec[i]))
			throw Tokenizer::ParserException("Invalid status Code " + vec[i]);
		this->errorPages[vec[i]] = vec.back().data();
	}
}

std::vector<std::string> &GlobalConfig::getIndexes()
{
	return (this->indexes);
}

const std::string &GlobalConfig::getErrorPage(const std::string &StatusCode)
{
	const static std::string empty = "";

	const std::map<std::string, std::string>::iterator &kv = this->errorPages.find(StatusCode);
	if (kv == this->errorPages.end())
		return empty;
	return (kv->second);
}

void GlobalConfig::setAlias(Tokens &token, Tokens &end)
{
	struct stat buf;

	if (!this->root.empty())
		throw Tokenizer::ParserException("Alias directive is duplicate");
	validateOrFaild(token, end);
	this->root = consume(token, end);
	if (stat(this->root.c_str(), &buf) != 0)
		throw Tokenizer::ParserException("Alias directory does not exist");
	if (S_ISDIR(buf.st_mode) == 0)
		throw Tokenizer::ParserException("Alias is not a directory");
	this->IsAlias = true;
	CheckIfEnd(token, end);
}
bool GlobalConfig::getAliasOffset() const
{
	return (this->IsAlias);
}

Proc::Proc() : buffer(CGI_BUFFER_SIZE + 3)
{
	this->offset = 0;
	this->output_fd = -1;
	this->outToFile = false;
	this->fout = -1;
	this->pid = -1;
	this->state = NONE;
}

Proc::Proc(const Proc &other)
{
	*this = other;
}
Proc &Proc::operator=(const Proc &other)
{
	if (this == &other)
		return (*this);
	this->pid = other.pid;
	this->fout = other.fout;
	this->state = other.state;
	this->client = other.client;
	this->outToFile = other.outToFile;
	this->offset = other.offset;
	this->input = other.input;
	this->output = other.output;
	return (*this);
}

void Proc::die()
{
	if (this->pid > 0)
		::kill(this->pid, SIGKILL);
	this->pid = -1;
}

void Proc::clean()
{
	if (this->fout >= 0)
		close(this->fout);
	if (this->output_fd >= 0)
		close(this->output_fd);
	this->fout = -1;
	this->output_fd = -1;
	std::remove(this->input.data());
	this->input.clear();
}

std::string Proc::mktmpfileName()
{
	std::stringstream ss;
	time_t now = time(0);
	struct tm *tstruct = localtime(&now);

	ss << "_" << tstruct->tm_year + 1900 << "_";
	ss << tstruct->tm_mon << "_";
	ss << tstruct->tm_mday << "_";
	ss << tstruct->tm_hour << "_";
	ss << tstruct->tm_min << "_";
	ss << tstruct->tm_sec;
	std::string rstr(24, ' ');
	const char charset[] = {

		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',

		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
	int n = sizeof(charset) / sizeof(charset[0]);
	for (int i = 0; i < 24; i++)
	{
		int idx = (std::rand() % n);
		rstr[i] = charset[idx];
	}
	return ("/tmp/" + rstr + ss.str());
}

int Proc::writeBody(const char *ptr, int size)
{
	if (this->output_fd < 0)
	{
		this->output = Proc::mktmpfileName();
		this->output_fd = open(this->output.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}
	if (this->output_fd < 0)
		return (-1);
	return (write(this->output_fd, ptr, size));
}
