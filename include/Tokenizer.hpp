#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "ServerContext.hpp"
#include <string>
#include <vector>

class Tokenizer
{
  private:
	std::string config;
	std::vector<std::string> tokens;
	std::string getNextToken();
	bool IsSpace(char c) const;

  public:

	class ParserException: public std::exception
	{
		private:
			std::string message;

		public:
			ParserException(std::string msg) ;
			ParserException();
			~ParserException() throw();
			virtual const char *what() const throw();
	};
	static bool IsId(char c);
	void parseConfig(ServerContext *context);
	void CreateTokens();
	void readConfig(const std::string path);
	Tokenizer();
	~Tokenizer();
};
#endif // !#ifndef TOKENIZER_HPP
