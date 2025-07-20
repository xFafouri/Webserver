SRC = main.cpp Server.cpp parser.cpp Location.cpp

OBJ = $(SRC:.cpp=.o)

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -g3 -fsanitize=address -std=c++98
RM = rm -rf

NAME = server

all: $(NAME)

$(NAME) : $(OBJ)
		$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

clean: 
		$(RM) $(OBJ)

fclean: clean
		$(RM) $(NAME)

re: fclean all