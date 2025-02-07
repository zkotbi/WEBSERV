
SRC		= src/CGIProcess.cpp src/Client.cpp src/Connections.cpp \
		  src/DataTypeImpl.cpp src/HttpRequest.cpp src/HttpResponse.cpp\
		  src/Location.cpp src/ServerContext.cpp src/Tokenizer.cpp\
		  src/Trie.cpp src/VirtualServer.cpp src/event.cpp src/main.cpp 


OBJ		= $(SRC:.cpp=.o)

CC		= c++

CFLAGS	= -std=c++98 -Wall -Wextra  -Werror

INC		= include/CGIProcess.hpp include/Client.hpp\
		  include/Connections.hpp include/DataType.hpp \
		  include/Event.hpp include/HttpRequest.hpp \
		  include/HttpResponse.hpp include/Location.hpp \
		  include/ServerContext.hpp include/Tokenizer.hpp \
		  include/Trie.hpp include/VirtualServer.hpp

NAME	 = webserv


all : $(NAME) 


$(NAME) : $(OBJ)
	$(CC)  $(CFLAGS)  $(OBJ)  -o $(NAME) 

%.o: %.cpp $(INC)
	$(CC) $(CFLAGS) -Iinclude/  -c $< -o $@

clean :
	rm -f $(OBJ)

fclean : clean
	rm -f $(NAME)

re : fclean all

.PHONY: all clean fclean re
