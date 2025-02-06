#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "DataType.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "ServerContext.hpp"
#include <cstddef>
#include <fstream>
#include <string>


#define red "\x1B[31m"
#define _reset "\033[0m"
#define blue "\x1B[34m"
#define green "\x1B[32m"
#define yellow "\x1B[33m"

enum responseState 
{
	START,
	WRITE_BODY,
	ERROR,
	CGI_EXECUTING,
	WRITE_ERROR,
	UPLOAD_FILES,
	END_BODY,
	START_CGI_RESPONSE
};

class HttpResponse
{
	private:
		enum reqMethode
		{
			GET  = 0b1,
			POST = 0b10,
			DELETE = 0b100,
			NONE = 0
		};
		struct errorResponse 
		{
			std::string		statusLine;
			std::string		headers;
			std::string		connection;
			std::string		contentLen;

			std::string		bodyHead;
			std::string		title;
			std::string		body;
			std::string		htmlErrorId;
			std::string		bodyfoot;
		};
			
		enum reqMethode									methode;
		std::vector<char>								body;
		httpError										status;	
		bool											isCgiBool;
		errorResponse									errorRes;

		std::string										fullPath;
		std::string										autoIndexBody;
		ServerContext									*ctx;
		HttpRequest										*request;
		std::string										errorPage;	
		bool											isErrDef;	
		std::map<std::string, std::string>				redirectionsStatus;	

		int												parseCgistatus();
		void											redirectionHandler();
		bool											created;
	public:
		struct upload_data {
			size_t			 it;
			size_t		     fileIt;
			std::string		 fileName;
			int				 __fd;
		};
		enum responseBodyType 
		{
			LOAD_FILE,	
			NO_TYPE,
			AUTO_INDEX,
			CGI
		};
		std::vector<char>								CGIOutput;

		enum responseBodyType							bodyType;
		size_t											writeByte;
		size_t											eventByte;
		int												responseFd;

		size_t											fileSize;
		size_t											sendSize;
		int												fd;
		std::string										cgiOutFile;
		std::ifstream									*cgiOutFilestram;

		class IOException : public std::exception
		{
			private:
				std::string msg;
			public :
				IOException(const std::string &msg) throw();
				IOException() throw();
				~IOException() throw();
				virtual const char* what() const throw();
		};

		std::string											queryStr;
		std::string											path_info;
		std::string											server_name;
		int													server_port;
		std::string											bodyFileName;
		std::string											strMethod;
		bool												keepAlive;
		Location											*location;
		enum responseState									state;
		std::string											path;
		std::map<std::string, std::string>					headers;
		std::map<std::string, std::string>					resHeaders;

		HttpResponse(int fd, ServerContext *ctx, HttpRequest *request);
		HttpResponse&	operator=(const HttpRequest& req);
		void			clear();
		~HttpResponse();

		int													parseCgiHaders(std::string str);
		void												write2client(int fd, const char *str, size_t size);
		void												logResponse() const;
		void												responseCooking();
		bool												isCgi();

		bool												isPathFounded();
		bool												isMethodAllowed();
		int													pathChecking();
		void												setHttpResError(int code, const std::string &str);

		std::string											getErrorRes();
		std::string											getContentLenght();
		int													directoryHandler();

		void												writeResponse();

		std::string											getStatusLine();
		std::string											getConnectionState();
		std::string											getContentType();
		std::string											getDate();
		int													sendBody(enum responseBodyType type);
		std::string											getContentLenght(enum responseBodyType type);

		int													autoIndexCooking();
		static std::string									getExtension(const std::string &str);

		void												parseCgiOutput();
		void												writeCgiResponse();

		int													uploadFile();
		void												deleteMethodeHandler();
		std::string											getAutoIndexStyle();
};

int										isHex(char c);

#endif
