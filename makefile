EXE = woosh

SRC_DIR = src
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o) $(OBJ_DIR)/parse.o

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@

$(OBJ_DIR)/parse.o: $(SRC_DIR)/parse.c
	$(CXX) -c $< -o $@

$(SRC_DIR)/parse.c: $(SRC_DIR)/parse.l
	flex -o $(SRC_DIR)/parse.c $<

clean:
	rm -rf $(OBJ_DIR)/*.o

.PHONY: all