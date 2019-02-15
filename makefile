EXE = woosh
TEST = wooshtest

SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include
TEST_DIR = test

SRC = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o, $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC)))
OBJ_TEST = $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR)/%.o, $(wildcard $(TEST_DIR)/*.cpp))

UNIT_TEST_LIB = -lboost_unit_test_framework

all: $(EXE)

$(EXE): $(OBJS) main.cpp
	$(CXX) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c* $(INCLUDE_DIR)/%.h
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $< -o $@
	
$(SRC_DIR)/parse.c: $(SRC_DIR)/parse.l
	flex -o $@ $<

test: $(TEST)
	./$(TEST)

$(TEST): $(OBJ_TEST) $(OBJS)
	$(CXX) -o $@ $(UNIT_TEST_LIB) $^

debug: $(OBJS) main.cpp
	$(CXX) $^ -g -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o

.PHONY: all test clean
