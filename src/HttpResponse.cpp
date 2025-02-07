#include "HttpResponse.hpp"
#include <dirent.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include "Event.hpp"
#include <iomanip>
#include "HttpRequest.hpp"
#include "ServerContext.hpp"

void initStatus(std::map<std::string, std::string> &map)
{
	map["300"] = "Multiple Choices";
	map["301"] = "Moved Permanently";
	map["302"] = "Found";
	map["303"] = "See Other";
	map["304"] = "Not Modified";
	map["307"] = "Temporary Redirect";
	map["308"] = "Permanent Redirect";
}

HttpResponse::HttpResponse(int fd, ServerContext *ctx, HttpRequest *request) : ctx(ctx), request(request), fd(fd)
{
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	bodyType = NO_TYPE;
	state = START;
	responseFd = -1;
	isErrDef = 1;
	errorRes.headers =
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Server: Lwla-Dorouf\r\n";
	errorRes.contentLen = "Content-Length: 0\r\n";
	errorRes.bodyHead =
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head><title>";
	errorRes.body =
		"</title></head>\n"
		"<body>\n"
		"<h1>";
	errorRes.bodyfoot =
		"</h1>\n"
		"</body>\n"
		"</html>";
	errorRes.connection = "Connection: Keep-Alive\r\n";
	initStatus(this->redirectionsStatus);
	cgiOutFilestram = NULL;
}

void HttpResponse::clear()
{
	if (responseFd >= 0)
		close(responseFd);
	if (!this->cgiOutFile.empty())
	{
		std::remove(this->cgiOutFile.data());
		this->cgiOutFile.clear();
	}
	responseFd = -1;
	isErrDef = 1;
	if (cgiOutFilestram != NULL)
		delete cgiOutFilestram;
	cgiOutFilestram = NULL;
	errorRes.statusLine.clear();
	errorRes.headers.clear();
	errorRes.connection.clear();
	errorRes.contentLen.clear();
	errorRes.bodyHead.clear();
	errorRes.title.clear();
	errorRes.body.clear();
	errorRes.htmlErrorId.clear();
	errorRes.bodyfoot.clear();
	errorPage.clear();
	CGIOutput.resize(0);
	methode = NONE;
	body.clear();
	status.code = 200;
	status.description = "OK";
	isCgiBool = 0;
	fullPath.clear();
	autoIndexBody.clear();
	writeByte = 0;
	eventByte = 0;
	fileSize = 0;
	sendSize = 0;
	queryStr.clear();
	this->path_info.clear();
	this->server_name.clear();
	strMethod.clear();
	state = START;
	path.clear();
	headers.clear();
	resHeaders.clear();
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	bodyType = NO_TYPE;
	close(responseFd);
	responseFd = -1;
	errorRes.headers =
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Server: Lwla-Dorouf\r\n";
	errorRes.contentLen = "Content-Length: 0\r\n";
	errorRes.bodyHead =
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head><title>";
	errorRes.body =
		"</title></head>\n"
		"<body>\n"
		"<h1>";
	errorRes.bodyfoot =
		"</h1>\n"
		"</body>\n"
		"</html>";
	errorRes.connection = "Connection: Keep-Alive\r\n";
	path_info.clear();
	bodyFileName.clear();
}

HttpResponse::~HttpResponse()
{
	if (cgiOutFilestram != NULL)
		delete cgiOutFilestram;
	cgiOutFilestram = NULL;
	if (responseFd >= 0)
		close(responseFd);
	remove(this->cgiOutFile.data());
}

std::string HttpResponse::getAutoIndexStyle()
{
	static std::string style =
		"* { margin: 0;"
		" padding: 0;"
		"box-sizing: border-box; } body {"
		"font-family: Arial, sans-serif;"
		"background-color: #f4f4f9;"
		"color: #333;"
		"line-height: 1.6;"
		"padding: 20px; } h1 {"
		"font-size: 70px;"
		"color: #ffffff;"
		"background-color: #0056b3;"
		"text-align: center;"
		"margin-bottom: 30px; } "
		"h2 {"
		"font-size: 70px;"
		"text-align: center;"
		"margin-bottom: 0px; margin-top: 0px; border:0px}"
		"a {"
		"text-decoration: none;"
		"color: #007BFF;"
		"font-size: 50px;"
		"margin: 0px 0;"
		"display: block;"
		"transition: color 0.3s ease; } a:hover { color: #0056b3; } div.file-list {"
		"max-width: 600px;"
		"margin: 0 auto;"
		"background-color: #fff;"
		"border-radius: 8px;"
		"padding: 0px;"
		"box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1); } a + a {"
		"margin-top: 10px; } footer {"
		"text-align: center;"
		"margin-top: 50px;"
		"font-size: 40px;"
		"color: #888; }";

	return (style);
}

std::string HttpResponse::getContentLenght()
{
	std::ostringstream oss;
	struct stat s;
	if (errorPage.size() && !stat(errorPage.c_str(), &s))
	{
		isErrDef = 0;
		oss << s.st_size;
		fileSize = s.st_size;
	}
	else
	{
		oss
			<< (errorRes.bodyHead.size() + errorRes.title.size() + errorRes.body.size() + errorRes.htmlErrorId.size()
				+ errorRes.bodyfoot.size() - 2);
	}
	return ("Content-Length: " + oss.str() + "\r\n");
}

void HttpResponse::write2client(int fd, const char *str, size_t size)
{
	if (write(fd, str, size) <= 0)
	{
		state = WRITE_ERROR;
		throw IOException();
	}
	writeByte += size;
}
void HttpResponse::logResponse() const
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
	if (this->status.code < 300)
		std::cout << green;
	else if (this->status.code < 400)
		std::cout << yellow;
	else
		std::cout << red;

	std::cout << ss.str() << " ";
	std::cout << "HTTP/1.1 " << this->strMethod << " " << this->path << " " << this->status.code << " "
			  << this->status.description << "\n";
	std::cout << _reset;
}

HttpResponse::IOException::~IOException() throw() {}

HttpResponse::IOException::IOException() throw()
{
	this->msg = "IOException: " + std::string(strerror(errno));
}

HttpResponse::IOException::IOException(const std::string &msg) throw()
{
	this->msg = msg;
}
const char *HttpResponse::IOException::what() const throw()
{
	return (this->msg.data());
}

std::string HttpResponse::getErrorRes()
{
	std::ostringstream oss;
	oss << status.code;
	std::string errorCode = oss.str();
	if (location)
		this->errorPage = location->globalConfig.getErrorPage(errorCode);
	errorRes.statusLine = "HTTP/1.1 " + oss.str() + " " + status.description + "\r\n";
	errorRes.title = oss.str() + " " + status.description;
	errorRes.htmlErrorId = oss.str() + " " + status.description;
	errorRes.connection = "Connection: Close\r\n";
	errorRes.contentLen = getContentLenght();
	if (this->responseFd >= 0)
		close(responseFd);
	this->responseFd = open(errorPage.c_str(), O_RDONLY);
	if (!isErrDef && responseFd >= 0)
		return (
			this->fullPath = errorPage,
			bodyType = LOAD_FILE,
			state = WRITE_BODY,
			errorRes.statusLine + errorRes.headers + errorRes.connection + errorRes.contentLen + "\r\n");
	return (
		errorRes.statusLine + errorRes.headers + errorRes.connection + errorRes.contentLen + errorRes.bodyHead
		+ errorRes.title + errorRes.body + errorRes.htmlErrorId + errorRes.bodyfoot);
}

HttpResponse &HttpResponse::operator=(const HttpRequest &req)
{
	this->isCgiBool = req.data.front()->bodyHandler.isCgi;
	this->queryStr = req.data.front()->queryStr;
	this->bodyFileName = req.data.front()->bodyHandler.bodyFile;
	this->path_info = req.data.front()->bodyHandler.path_info;
	path = req.data[0]->path;
	headers = req.data[0]->headers;
	status.code = req.data[0]->error.code;
	status.description = req.data[0]->error.description;
	strMethod = req.data[0]->strMethode;
	if (req.data.front()->headers.count("Connection") != 0 && req.data.front()->headers["Connection"] == "Close")
		keepAlive = 0;
	if (req.data[0]->state == REQ_ERROR)
	{
		state = ERROR;
		keepAlive = 0;
	}
	this->created = req.data.front()->bodyHandler.created;
	return (*this);
}

bool HttpResponse::isCgi()
{
	return (this->isCgiBool);
}

void HttpResponse::setHttpResError(int code, const std::string &str)
{
	state = ERROR;
	status.code = code;
	status.description = str;
	keepAlive = 0;
}

bool HttpResponse::isPathFounded()
{
	if (location == NULL)
		return (setHttpResError(404, "Not Found"), false);
	return (true);
}

bool HttpResponse::isMethodAllowed()
{
	if (strMethod == "GET")
		methode = GET;
	else if (strMethod == "POST")
		methode = POST;
	else if (strMethod == "DELETE")
		methode = DELETE;
	else
		methode = NONE;
	return (this->location->isMethodAllowed(this->methode));
}

int HttpResponse::autoIndexCooking()
{
	std::vector<std::string> dirContent;
	DIR *dirStream = opendir(fullPath.c_str());
	if (dirStream == NULL)
		return (setHttpResError(500, "Internal Server Error"), 0);
	struct dirent *_dir;
	while ((_dir = readdir(dirStream)) != NULL)
		dirContent.push_back(_dir->d_name);
	if (path[path.size() - 1] != '/')
		path += '/';
	autoIndexBody =
		"<!DOCTYPE html>\n"
		"<html lang=\"en\">\n"
		"	<head>\n"
		"		<title>YOUR DADDY</title>\n"
		"		<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		"	</head>\n"
		"	<body>\n";
	autoIndexBody += "<h1>Liste of files<h1>\n<h2>";
	for (size_t i = 0; i < dirContent.size(); i++)
	{
		autoIndexBody += "<a href=\"";
		autoIndexBody += path + dirContent[i] + "\">" + dirContent[i] + "</a><br>";
	}
	autoIndexBody +=
		"	</h2></body>\n"
		"</html>\n";
	return (closedir(dirStream), 1);
}

int HttpResponse::directoryHandler()
{
	const std::vector<std::string> &indexes = this->location->globalConfig.getIndexes();
	if (this->fullPath[fullPath.size() - 1] != '/')
		fullPath.push_back('/');
	for (size_t i = 0; i < indexes.size(); i++)
	{
		if (access((this->fullPath + indexes[i]).c_str(), F_OK | R_OK) != -1)
			return (bodyType = LOAD_FILE, (fullPath += indexes[i]), 1);
	}
	if (location->globalConfig.getAutoIndex())
		return (bodyType = AUTO_INDEX, autoIndexCooking());
	return (setHttpResError(403, "Forbidden"), 0);
}

int HttpResponse::pathChecking()
{
	size_t offset = location->globalConfig.getAliasOffset() ? this->location->getPath().size() : 0;
	this->fullPath = location->globalConfig.getRoot() + this->path.substr(offset);

	struct stat sStat;
	stat(fullPath.c_str(), &sStat);
	if (S_ISDIR(sStat.st_mode) && methode != POST)
		return (directoryHandler());
	if (access(fullPath.c_str(), F_OK | R_OK) != -1)
		return (bodyType = LOAD_FILE, 1);
	else
		return (state = ERROR, setHttpResError(404, "Not Found"), 0);
	return (1);
}

static int isValidHeaderChar(char c)
{
	return (std::isalpha(c) || std::isdigit(c) || c == '-' || c == ':');
}

int HttpResponse::parseCgiHaders(std::string str)
{
	size_t pos = str.find(':');
	std::string tmpHeaderName;
	std::string tmpHeaderVal;

	if (str.size() < 3)
		return (1);
	if (pos == std::string::npos || pos == 0 || str[str.size() - 1] != '\n')
		return (setHttpResError(502, "Bad Gateway"), 0);
	tmpHeaderName = str.substr(0, pos);
	for (size_t i = 0; i < tmpHeaderName.size(); i++)
	{
		if (!isValidHeaderChar(tmpHeaderName[i]))
			return (setHttpResError(502, "Bad Gateway"), 0);
	}
	tmpHeaderVal = str.substr(pos + 1);
	if (tmpHeaderVal.size() < 3 || tmpHeaderVal[0] != ' ')
		return (setHttpResError(502, "Bad Gateway"), 0);
	resHeaders[tmpHeaderName] = tmpHeaderVal.substr(0, tmpHeaderVal.size() - 2);
	return (1);
}

int HttpResponse::parseCgistatus()
{
	map_it it = resHeaders.find("Status");
	std::stringstream ss;

	if (it == resHeaders.end())
		return (1);
	if (it->second.size() < 5)
		return (0);
	ss << it->second;
	ss >> this->status.code;
	if (this->status.code > 599 || this->status.code < 100)
		return (0);
	this->status.description = ss.str().substr(5);
	return (1);
}

void HttpResponse::parseCgiOutput()
{
	std::string headers(CGIOutput.data(), CGIOutput.size());
	size_t pos = headers.find("\n");
	size_t strIt = 0;

	if (CGIOutput.size() == 0 || headers.find("\r\n\r\n") == std::string::npos)
		return setHttpResError(502, "Bad Gateway");
	while (pos != std::string::npos)
	{
		if (!parseCgiHaders(headers.substr(strIt, (pos - strIt + 1))))
			return;
		strIt = pos + 1;
		pos = headers.find("\n", strIt);
	}
	if (strIt < headers.size())
		setHttpResError(502, "Bad Gateway");
	if (!parseCgistatus())
		setHttpResError(502, "Bad Gateway");
}

void HttpResponse::writeCgiResponse()
{
	if (resHeaders.count("Connection") == 0)
	{
		if (keepAlive)
			resHeaders["Connection"] = "Keep-Alive";
		else
			resHeaders["Connection"] = "Close";
	}
	parseCgiOutput();
	if (state == ERROR)
		return;
	write2client(this->fd, getStatusLine().c_str(), getStatusLine().size());
	if ((resHeaders.find("Transfer-Encoding") == resHeaders.end() || resHeaders["Transfer-Encoding"] != "Chunked"))
		resHeaders["Content-Length"] = getContentLenght(bodyType);
	for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
	{
		write2client(this->fd, it->first.c_str(), it->first.size());
		write2client(this->fd, ": ", 1);
		write2client(this->fd, it->second.c_str(), it->second.size());
		write2client(fd, "\r\n", 2);
	}
	write2client(this->fd, "\r\n", 2);
	state = WRITE_BODY;
}

void HttpResponse::writeResponse()
{
	writeByte = 0;
	write2client(this->fd, getStatusLine().c_str(), getStatusLine().size());
	write2client(this->fd, getConnectionState().c_str(), getConnectionState().size());
	write2client(this->fd, getContentType().c_str(), getContentType().size());
	write2client(this->fd, getContentLenght(bodyType).c_str(), getContentLenght(bodyType).size());
	write2client(fd, getDate().c_str(), getDate().size());
	write2client(fd, "Server: Lwla-Dorouf\r\n", strlen("Server: Lwla-Dorouf\r\n"));
	for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
	{
		write2client(this->fd, it->first.c_str(), it->first.size());
		write2client(this->fd, ": ", 2);
		write2client(this->fd, it->second.c_str(), it->second.size());
		write2client(fd, "\r\n", 2);
	}
	write2client(this->fd, "\r\n", 2);
	if (methode == POST)
		write2client(this->fd, "Resource created successfully", 29);
	if (methode == GET)
		state = WRITE_BODY;
}

std::string HttpResponse::getStatusLine()
{
	std::ostringstream oss;
	oss << status.code;

	return ("HTTP/1.1 " + oss.str() + " " + status.description + "\r\n");
}

std::string HttpResponse::getConnectionState()
{
	if (keepAlive)
		return ("Connection: Keep-Alive\r\n");
	return ("Connection: Close\r\n");
}

std::string HttpResponse::getExtension(const std::string &str)
{
	int i = str.size() - 1;
	while (i >= 0)
	{
		if (str[i] == '.')
			return (str.substr(i + 1));
		i--;
	}
	return ("");
}

std::string HttpResponse::getContentType()
{
	if (bodyType == NO_TYPE)
		return ("Content-Type: text/plain\r\n");
	if (bodyType == AUTO_INDEX)
		return ("Content-Type: text/html\r\n");
	return ("Content-Type: " + ctx->getType(getExtension(fullPath)) + "\r\n");
}

std::string HttpResponse::getDate()
{
	time_t now = time(NULL);

	struct tm *timeinfo = localtime(&now);
	std::stringstream ss;
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	ss << days[timeinfo->tm_wday] << " " << months[timeinfo->tm_mon] << " " << std::setfill('0') << std::setw(2)
	   << timeinfo->tm_mday << " " << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":" << std::setfill('0')
	   << std::setw(2) << timeinfo->tm_min << ":" << std::setfill('0') << std::setw(2) << timeinfo->tm_sec << " "
	   << (1900 + timeinfo->tm_year);
	return ("date: " + std::string(ss.str()) + "\r\n");
}

int HttpResponse::sendBody(enum responseBodyType type)
{
	state = WRITE_BODY;
	static std::vector<char> buffer(BUFFER_SIZE + 1);
	if (type == CGI || type == LOAD_FILE)
	{
		if (cgiOutFilestram == NULL && type == CGI)
			cgiOutFilestram = new std::ifstream(cgiOutFile, std::ios::binary);
		else if (cgiOutFilestram == NULL && type == LOAD_FILE)
			cgiOutFilestram = new std::ifstream(fullPath.c_str(), std::ios::binary);
		if (!cgiOutFilestram->is_open())
			throw IOException("Read : " + std::string(strerror(errno)));

		if (fileSize <= 0)
		{
			state = END_BODY;
			writeByte = 0;
			return (1);
		}
		size_t readbuffer;
		readbuffer = std::min(BUFFER_SIZE,	eventByte - writeByte);
		cgiOutFilestram->read(buffer.data(), readbuffer);
		int size = cgiOutFilestram->gcount();
		if (size <= 0)
				throw IOException("Read : " + std::string(strerror(errno)));
		write2client(fd, buffer.data(), size);
		this->sendSize += size;
		if (this->sendSize >= fileSize || cgiOutFilestram->eof())
		{
			cgiOutFilestram->close();
			delete cgiOutFilestram;
			cgiOutFilestram = NULL;
			state = END_BODY;
		}
		writeByte = 0;
	}
	else if (type == AUTO_INDEX)
	{
		write2client(this->fd, autoIndexBody.c_str(), autoIndexBody.size());
		state = END_BODY;
	}
	return (1);
}

std::string HttpResponse::getContentLenght(enum responseBodyType type)
{
	(void)request;
	if (type == LOAD_FILE || type == CGI)
	{
		std::stringstream ss;
		struct stat s;
		if (type == CGI)
			stat(this->cgiOutFile.c_str(), &s);
		else
			stat(this->fullPath.c_str(), &s);
		ss << s.st_size;
		fileSize = s.st_size;
		if (type == CGI)
			return (ss.str());
		if (methode == POST && created)
			return ("Content-Length: 29\r\n");
		if (methode == POST)
			return ("Content-Length: 0\r\n");
		return ("Content-Length: " + ss.str() + "\r\n");
	}
	if (type == AUTO_INDEX)
	{
		std::ostringstream oss;
		oss << autoIndexBody.size();
		return ("Content-Length: " + oss.str() + "\r\n");
	}
	return ("Content-Length: 0\r\n");
}

int isHex(char c)
{
	std::string B = "0123456789ABCDEF";
	std::string b = "0123456789abcdef";

	if (b.find(c) == std::string::npos && B.find(c) == std::string::npos)
		return (0);
	return (1);
}

int HttpResponse::uploadFile()
{
	if (created)
	{
		status.code = 201;
		status.description = "Created";
	}
	writeResponse();
	this->state = END_BODY;
	return (1);
}

void HttpResponse::deleteMethodeHandler()
{
	if (bodyType == AUTO_INDEX)
		return (bodyType = NO_TYPE, setHttpResError(403, "Forbidden"));
	bodyType = NO_TYPE;
	std::remove(fullPath.c_str());
	status.code = 204;
	status.description = "No Content";
	writeResponse();
	this->state = END_BODY;
}

void HttpResponse::redirectionHandler()
{
	status.code = 300 + location->getRedirection().status[2] - 48;
	status.description = redirectionsStatus[location->getRedirection().status];
	resHeaders["Location"] = location->getRedirection().url;
	writeResponse();
	this->state = END_BODY;
}

void HttpResponse::responseCooking()
{
	if (!isPathFounded() || isCgi())
		return;
	else
	{
		if (!isMethodAllowed())
			return setHttpResError(405, "Method Not Allowed");
		if (location->HasRedirection())
			redirectionHandler();
		if (!pathChecking())
			return;
		if (methode == POST)
			uploadFile();
		if (methode == GET)
			writeResponse();
		if (methode == DELETE)
			deleteMethodeHandler();
	}
}

