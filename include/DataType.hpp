#ifndef DataType_H
#define DataType_H

#include <map>
#include <string>
#include <vector>


typedef std::vector<std::string>::iterator Tokens;
#define CGI_BUFFER_SIZE 64L * 1024L

class Proc
{
	public:
		enum State
		{
			TIMEOUT,
			NONE
		};
		static std::string mktmpfileName();
		std::string output;
		std::string input;
		int			output_fd;
		std::vector<char> buffer;
		Proc();
		Proc(const Proc &other);
		void clean();
		void die();
		int writeBody(const char *ptr, int size);
		Proc &operator=(const Proc &other);
		int pid;
		int client;
		int fout;
		bool outToFile;
		int		offset;
		enum State state;
};

class GlobalConfig
{
	private:
		std::string root;
		int autoIndex;
		std::string upload_file_path;
		std::vector<std::string> indexes;
		bool IsAlias;
		std::map<std::string, std::string> errorPages;

	public:

		bool isValidStatusCode(std::string &str); 
		void setMethods(Tokens &token, Tokens &end);
		void setAlias(Tokens &token, Tokens &end);
		void validateOrFaild(Tokens &token, Tokens &end);
		void CheckIfEnd(Tokens &token, Tokens &end);
		std::string &consume(Tokens &token, Tokens &end);
		GlobalConfig &operator=(const GlobalConfig &globalParam);

		bool isMethodAllowed(int method) const;
		std::string getRoot() const;
		bool getAutoIndex() const;

		GlobalConfig();
		GlobalConfig(int autoIndex, const std::string &upload_file_path);
		~GlobalConfig();
		GlobalConfig(const GlobalConfig &other);

		bool parseTokens(Tokens &token, Tokens &end);

		static bool IsId(std::string &token);

		void setRoot(Tokens &token, Tokens &end);

		void setAutoIndex(Tokens &token, Tokens &end);

		std::vector<std::string> &getIndexes();
		void setIndexes(Tokens &token, Tokens &end);
		void setErrorPages(Tokens &token, Tokens &end);
		const std::string &getErrorPage(const std::string &StatusCode);
		bool getAliasOffset() const;
		void copy(const GlobalConfig &other);
};

#endif
