NAME	:= webserv
SRC		:= webserv.cpp Socket.cpp Message.cpp Request.cpp Response.cpp MimeTypes.cpp Config.cpp
SRC		:= $(addprefix src/,$(SRC))
FLAGS	:= -Wall -Wextra -Werror

all: $(NAME)

$(NAME):$(SRC)
	clang++ -g -fsanitize=address --std=c++98 $(SRC) -o $@ -Iinclude

parser: parser.cpp
	@clang++ --std=c++98 -glldb parser.cpp $(SRC) -Iinclude -o parser

clean:
	@rm -rf $(NAME)

fclean:clean

re: clean all
