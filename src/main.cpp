#include <ctime>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include "CGIProcess.hpp"
#include "Event.hpp"
#include "Tokenizer.hpp"

#include <cstring>

#define MAX_EVENTS 32
#define MAX_CONNECTIONS_QUEUE 256
#define CONF_FILE_PATH "webserv.conf"

ServerContext *LoadConfig(const char *path)
{
	ServerContext *ctx = NULL;
	try
	{
		Tokenizer tokenizer;
		tokenizer.readConfig(path);
		tokenizer.CreateTokens();
		ctx = new ServerContext();
		tokenizer.parseConfig(ctx);
		ctx->init();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		delete ctx;
		return (NULL);
	}
	return (ctx);
}

void ctrl_c(int )
{
	Event::Running(1);
}


void leak() {system("leaks webserv");}
int main(int ac, char **argv)
{

	atexit(leak);

	ServerContext *ctx = NULL;
	signal(SIGINT, ctrl_c);
	std::srand(time(NULL));
	if (ac > 2)
	{
		std::cerr << "usage: webserv confile\n";
		return (1);
	}
	else if (ac == 2)
		ctx = LoadConfig(argv[1]);
	else
		ctx = LoadConfig(CONF_FILE_PATH);
	if (!ctx)
		return 1;
	try
	{
		Event event(MAX_CONNECTIONS_QUEUE,MAX_EVENTS, ctx);
		event.init();
		event.Listen();
		event.initIOmutltiplexing();
		event.eventLoop();
	}
	catch (const CGIProcess::ChildException &e)
	{
		delete ctx;
		return (1);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << "\n";
	}

	delete ctx;
}
