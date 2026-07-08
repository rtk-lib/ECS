CXX = clang++

CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -I./include -I./test -fno-rtti -O3 -march=native -flto=auto
LDFLAGS = -lcriterion -flto=auto

TEST_SRC = $(wildcard test/*.cpp)
TEST_OBJ = $(TEST_SRC:.cpp=.o)
TEST_NAME = unit_tests

SAFETY_DIR = test/test_assert
SAFETY_SRC = $(wildcard $(SAFETY_DIR)/*.cpp)
SAFETY_OBJ = $(SAFETY_SRC:.cpp=.o)

all: CXXFLAGS += -DNDEBUG
all: $(TEST_NAME)

$(TEST_NAME): $(TEST_OBJ)
	$(CXX) $(TEST_OBJ) -o $(TEST_NAME) $(LDFLAGS)

fullTest:
	@$(MAKE) fclean
	@$(MAKE) build_full

build_full: $(TEST_OBJ) $(SAFETY_OBJ)
	$(CXX) $(TEST_OBJ) $(SAFETY_OBJ) -o $(TEST_NAME) $(LDFLAGS)
	./$(TEST_NAME)

tests_run: all
	./$(TEST_NAME)

clean:
	rm -f $(TEST_OBJ) $(SAFETY_OBJ)

fclean: clean
	rm -f $(TEST_NAME)

re: fclean all

.PHONY: all tests_run clean fclean re fullTest build_full