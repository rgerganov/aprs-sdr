SRC_DIR := src
BIN_DIR := bin

EXE := $(BIN_DIR)/aprs-sdr
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(SRC_DIR)/%.o)

CXXFLAGS = -O2 -Wall -Iinclude -std=c++11

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	    $(CXX) $^ -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)
