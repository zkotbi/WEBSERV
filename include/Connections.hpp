#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP

#include "Client.hpp"
#include "ServerContext.hpp"
#include <map>

typedef std::map<int, Client>::iterator ClientsIter;

class Connections 
{
	private:
		ServerContext	*ctx;
		int				kqueueFd;
	public:
		std::map<int, Client> clients;

		Connections(ServerContext *ctx, int kqueueFd);
		~Connections();
		
		void		init(ServerContext *ctx, int kqueueFd);
		void		closeConnection(int	fd);
		void		addConnection(int	fd, int server);
		Client		*requestHandler(int	fd, int data);
		Client		*getClient(int fd);
};

#endif
