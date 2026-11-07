NAME = ft_traceroute
CC = cc
CFLAGS = -Wall -Werror -Wextra

SRC_DIR = srcs
SRC = $(SRC_DIR)/main.c \
	  $(SRC_DIR)/parse.c \
	  $(SRC_DIR)/fqdn.c \


OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
INC_DIR = includes
INC = -I$(INC_DIR)

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) -lm

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/ft_traceroute.h
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
