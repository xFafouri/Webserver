NAME = webserv
CXX = c++
CXXFLAGS =  -std=c++98  -Wall -Wextra -Werror 	#-fsanitize=address -g3
src = main.cpp ./server/server.cpp  ./server/cgi_handler.cpp ./server/methods.cpp ./server/handle_response.cpp ./server/start_server.cpp  ./config_file/Server.cpp ./config_file/parser.cpp ./config_file/Location.cpp ./config_file/Cgi.cpp 

OBJ = $(src:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

clean: 
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all
