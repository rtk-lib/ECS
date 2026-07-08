CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -I./include -I./test -O3 -fno-rtti

TEST_SRC = $(wildcard test/*.cpp)
TEST_OBJ = $(TEST_SRC:.cpp=.o)
TEST_NAME = unit_tests

LDFLAGS = -lcriterion

all: $(TEST_NAME)

$(TEST_NAME): $(TEST_OBJ)
	$(CXX) $(TEST_OBJ) -o $(TEST_NAME) $(LDFLAGS)

tests_run: $(TEST_NAME)

clean:
	rm -f $(TEST_OBJ)

fclean: clean
	rm -f $(TEST_NAME)

re: fclean all

.PHONY: all tests_run clean fclean re
