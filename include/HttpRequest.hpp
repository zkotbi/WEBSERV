#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <fstream>
#include <sys/event.h>
#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include "Location.hpp"

#define URI_MAX 2048
#define BUFFER_SIZE 1 * 1024 * 1024UL
#define SOCK_BUFFER_SIZE 1 * 1024 * 1024

typedef std::map<std::string, std::string>::iterator map_it;
typedef std::map<std::string, std::string> Headers;

enum reqMethode
{
	GET = 0b1,
	POST = 0b10,
	DELETE = 0b100,
	NONE = 0
};

enum reqState
{
	NEW,
	METHODE,
	PATH,
	HTTP_VERSION,
	REQUEST_LINE_FINISH,
	HEADER_NAME,
	HEADER_VALUE,
	HEADER_FINISH,
	BODY,
	BODY_FINISH,
	REQUEST_FINISH,
	REQ_ERROR,
	DEBUG
};

enum ChunkState
{
	SIZE,
	LINE,
	END_LINE
};

enum crlfState
{
	READING,
	NLINE,
	RETURN,
	LNLINE,
	LRETURN
};

typedef struct httpError
{
	int code;
	std::string description;
} httpError;

typedef struct methodeStr
{
	std::string tmpMethodeStr;
	std::string eqMethodeStr;

} MethodeStr;

enum reqBodyType
{
	MULTI_PART,
	TEXT_PLAIN,
	URL_ENCODED,
	NON
};

struct BodyHandler
{
	BodyHandler();
	~BodyHandler();

	int openNewFile(std::string uploadPath);
	int writeBody();
	int writeChunkedBody();

	bool isCgi;

	std::map<std::string, std::string> headers;

	std::ofstream*  uploadStream;
	std::ofstream*  bodyFstream;

	std::string bodyFile;
	std::string path_info;
	size_t bodyIt;
	std::vector<char> body; // raw body;
	size_t bodySize;
	std::string tmp;

	std::string header;
	std::string tmpBorder;

	bool created;
	bool isBodyInit;


	size_t fileBodyIt;
	size_t borderIt;
	std::vector<char> fileBody;
	std::vector<char> chunkeBody;
};

typedef struct data_s
{
	std::string path;
	std::string getPath();

	std::string queryStr;
	std::string path_info;

	bool isRequestLineValid;
	bool isRequestLineValidated();

	std::string strMethode;
	std::map<std::string, std::string> headers;

	httpError error;
	enum reqState state;

	BodyHandler bodyHandler;

} data_t;

class HttpRequest
{
private:
	enum multiPartState
	{
		_NEW,
		_ERROR,
		BORDER,
		MULTI_PART_HEADERS,
		BODY_CRLF,
		MULTI_PART_HEADERS_VAL,
		STORING,
		FINISHED,
	};

	enum multiPartState bodyState;
	struct kevent *ev;
	typedef std::map<std::string, std::string> Headers;
	ChunkState chunkState;
	size_t totalChunkSize;
	size_t chunkSize;
	size_t chunkIndex;
	std::string sizeStr;

	const int fd;
	enum crlfState crlfState;

	std::string currHeaderName;
	std::string currHeaderVal;

	std::vector<char> body;
	size_t bodySize;
	int reqSize;
	size_t reqBufferSize;
	size_t reqBufferIndex;
	std::vector<char> reqBuffer;

	httpError error;

	MethodeStr methodeStr;
	std::string httpVersion;

	bool isCGI();
	void handleNewBody();
	void handleMultiPartHeaders();
	void handleStoring();
	int checkMultiPartEnd();
	bool isMethodAllowed();
	int convertChunkSize();
	void chunkEnd();

	void parseMethod();
	void parsePath();
	void parseHttpVersion();
	void parseHeaderName();
	void parseHeaderVal();
	void parseBody();
	void crlfGetting();

	int firstHeadersCheck();

	int verifyUriChar(char c);
	void checkHttpVersion(int *state);

	void contentLengthBodyParsing();
	void chunkedBodyParsing();

	void returnHandle();
	void nLineHandle();
	int checkContentType();

	int parseMuliPartBody();
	void andNew();
	void addPathIndex();

public:
	std::vector<data_t *> data;
	reqBodyType reqBody;
	std::string bodyBoundary;
	enum reqState state;
	Location *location;
	bool eof;

	HttpRequest();
	HttpRequest(int fd);
	~HttpRequest();
	void clear();
	void clearData();

	void feed();
	void readRequest(int data);
	void setFd(int fd);

	void setHttpReqError(int code, std::string str);

	httpError getStatus() const;
	std::string getStrMethode() const;
	const std::string &getHost() const;
	bool validateRequestLine();
	int parseMultiPart();
	void decodingUrl();
	void splitingQuery();
	static int isNum(const std::string &str);
};

#endif
