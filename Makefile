NAME = webserv
CXX = c++
CXXFLAGS =  -fsanitize=address -g3 #-Wall -Wextra -Werror -std=c++98
src = main.cpp ./server/server.cpp ./server/start_server.cpp  ./config_file/Server.cpp ./config_file/parser.cpp ./config_file/Location.cpp ./config_file/Cgi.cpp 

OBJ = $(src:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

clean: 
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all
