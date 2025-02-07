#include "CGIProcess.hpp"
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include "DataType.hpp"
#include "Event.hpp"

bool CGIProcess::IsFileExist()
{
	size_t offset = response->location->globalConfig.getAliasOffset() ? response->location->getPath().size()
																	  : 0; // offset for alais
	std::string scriptPath = response->location->globalConfig.getRoot() + response->path.substr(offset);

	if (access(scriptPath.c_str(), F_OK) == -1)
		return (this->response->setHttpResError(404, "Not Found"), 1);
	if (access(scriptPath.c_str(), R_OK) == -1)
		return (this->response->setHttpResError(403, "Forbidden"), 1);
	return (0);
}

void CGIProcess::closePipe(int fd[2])
{
	close(fd[1]);
	close(fd[0]);
}

int CGIProcess::redirectPipe()
{
	int fd = open("/dev/null", O_RDWR);
	dup2(fd, STDERR_FILENO); //may faild but it not a major error
	close(this->pipeOut[0]);
	if (dup2(pipeOut[1], STDOUT_FILENO) < 0)
		return (-1);
	close(this->pipeOut[1]);
	if (this->response->strMethod != "POST")
		return (0);
	int body_fd = open(this->response->bodyFileName.data(), O_RDONLY);
	if (body_fd < 0)
		return (-1);
	if (dup2(body_fd, STDIN_FILENO) < 0)
		return (close(body_fd), -1);
	close(body_fd);
	return (0);
}

std::string ToEnv(std::map<std::string, std::string>::iterator &header)
{
	std::string str = "HTTP_";
	int idx = 0;
	for (size_t i = 0; i < header->first.size(); i++, idx++)
	{
		char c = header->first[i];
		if (c == '-')
			c = '_';
		str.push_back(std::toupper(c));
	}
	str.push_back('=');
	str += header->second;
	return (str);
}

void CGIProcess::loadEnv()
{
	std::map<std::string, std::string> env;
	std::stringstream ss;
	std::map<std::string, std::string>::iterator it = response->headers.begin();
	for (int i = 0; it != response->headers.end(); it++, i++)
		this->env.push_back(ToEnv(it));
	ss << response->server_port;
	env["SERVER_SOFTWARE"] = "42webserv";
	env["REDIRECT_STATUS"] = ""; // make php a happy cgi
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_PORT"] = ss.str(); // need
	env["SERVER_NAME"] = response->server_name;
	env["REQUEST_METHOD"] = response->strMethod;
	env["SCRIPT_NAME"] = response->path;
	env["QUERY_STRING"] = response->queryStr;
	env["PATH_INFO"] = response->path_info;
	env["HTTP_HOST"] = response->headers["Host"];
	env["CONTENT_TYPE"] = response->headers["Content-Type"];
	env["SCRIPT_FILENAME"] = this->cgi_file;
	if (response->strMethod == "POST")
		env["CONTENT_LENGTH"] = response->headers["Content-Length"];
	else
		env["CONTENT_LENGTH"] = "0";

	it = env.begin();
	for (int i = 0; it != env.end(); it++, i++)
		this->env.push_back(it->first + "=" + it->second);
}

void CGIProcess::child_process()
{
	try
	{
		size_t offset = response->location->globalConfig.getAliasOffset() ? response->location->getPath().size()
																		  : 0; // offset for alais
		this->cgi_file = response->location->globalConfig.getRoot() + response->path.substr(offset);

		this->cgi_bin =
			response->location->getCGIPath("." + response->getExtension(response->path)); // INFO: make this dynamique
																						  //
		if (this->chdir())
			throw CGIProcess::ChildException();
		this->loadEnv();
		const char *args[3] = {this->cgi_bin.data(), cgi_file.data(), NULL};
		char **envp = new char *[env.size() + 1];
		size_t i = 0;
		for (; i < env.size(); i++)
			envp[i] = (char *)this->env[i].data();
		envp[i] = NULL;
		if (this->redirectPipe())
		{
			closePipe(this->pipeOut);
			throw CGIProcess::ChildException();
		}
		execve(*args, (char *const *)args, envp);
		throw CGIProcess::ChildException();
	}
	catch (std::exception &e)
	{
		throw CGIProcess::ChildException();
	}
}

Proc CGIProcess::RunCGIScript(HttpResponse &response)
{
	Proc proc;
	this->response = &response;
	if (this->IsFileExist())
		return (proc);
	else if (pipe(this->pipeOut) < 0)
		return (this->response->setHttpResError(500, "Internal Server Error"), proc);
	proc.pid = fork();
	if (proc.pid < 0)
	{
		closePipe(this->pipeOut);
		return (this->response->setHttpResError(500, "Internal Server Error"), proc);
	}
	else if (proc.pid == 0)
		this->child_process();
	close(this->pipeOut[1]);
	proc.fout = this->pipeOut[0];
	return (proc);
}
int CGIProcess::chdir()
{
	size_t pos = this->cgi_file.rfind('/');
	if (pos == std::string::npos)
		return (-1);
	std::string dir = this->cgi_file.substr(0, pos);
	if (::chdir(dir.data()) < 0)
		return (-1);
	this->cgi_file = this->cgi_file.substr(pos + 1);
	return (0);
}

CGIProcess::ChildException::ChildException() throw() {}

const char *CGIProcess::ChildException::what() const throw()
{
	return "child";
}
