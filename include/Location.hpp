#ifndef Location_H
#define Location_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "DataType.hpp"

class Location
{
	private:
		typedef int http_method_t;
		enum methods_e
		{
			GET = 0b1,
			POST = 0b10,
			DELETE = 0b100,
		};

		struct Redirection
		{
			std::string status; //3xx
			std::string url;
		};
		Redirection redirect;

		http_method_t methods;
		bool isRedirection;
		std::string path;
		std::map<std::string, std::string> cgiMap;
		std::string upload_file_path;
		std::string alias;
		long maxBodySize;

	public:

		long getMaxBody() const;
		Location();
		Location &operator=(const Location &location);
		GlobalConfig globalConfig;

		bool isMethodAllowed(int method) const;
		void setUploadPath(Tokens &token, Tokens &end);
		void setPath(std::string &path);
		const std::string &getPath();
		void setRedirect(Tokens &token, Tokens &end);
		void parseTokens(Tokens &token, Tokens &end);
		static bool isValidStatusCode(std::string &str);
		bool HasRedirection() const;
		const Redirection &getRedirection() const;
		void setMethods(Tokens &token, Tokens &end);

		void setMaxBodySize(Tokens &token, Tokens &end);

		const std::string &getCGIPath(const std::string &ext);
		const std::string &geCGIext();
		void setCGI(Tokens &token, Tokens &end);
		const std::string &getFileUploadPath();
};
#endif
