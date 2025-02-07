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

#define MAX_EVENTS 128
#define MAX_CONNECTIONS_QUEUE 128
#define CONF_FILE_PATH "webserv.conf"

ServerContext *LoadConfig(const char *path)
{
	ServerContext *ctx = new ServerContext();;
	try
	{
		Tokenizer tokenizer;
		tokenizer.readConfig(path);
		tokenizer.CreateTokens();
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

static void ctrl_c(int )
{
	Event::Running(1);
}


void leak() {system("lsof -c webserv && leaks webserv");}

static void handelSignal()
{
	signal(SIGINT, ctrl_c);
	signal(SIGPIPE, SIG_IGN);
	std::srand(time(NULL));
}
int main(int ac, char **argv)
{

	atexit(leak);

	handelSignal();
	ServerContext *ctx = NULL;
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
