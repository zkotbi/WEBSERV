#ifndef CGIProcess_H
#define CGIProcess_H
#include <exception>
#include <string>
#include <vector>
#include "DataType.hpp"
#include "HttpResponse.hpp"

class CGIProcess
{
	private:
		HttpResponse *response;
		std::vector<std::string> env;
		std::string cgi_bin;
		std::string cgi_file;

		int chdir() ;
		int redirectPipe();
		void closePipe(int fd[2]);
		void child_process();
		void loadEnv();
		bool IsFileExist();

		int pipeOut[2];

	public:
		class ChildException: public std::exception
		{
			public:
				ChildException() throw();
				const char *what() const throw();
		};
		Proc RunCGIScript(HttpResponse &response);
};

#endif
