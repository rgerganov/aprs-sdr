SRC_DIR := src
BIN_DIR := bin
WEB_DIR := web

EXE := $(BIN_DIR)/aprs-sdr
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(SRC_DIR)/%.o)

ifeq ($(CXX),emcc)
	EMFLAGS = -s EXPORTED_FUNCTIONS='["_gen_iq_s8","_malloc","_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall","getValue"]' -s ALLOW_MEMORY_GROWTH=1
	EXE := $(WEB_DIR)/aprs-sdr.js
endif
CXXFLAGS = -O2 -Wall -Iinclude -std=c++11

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $^ -o $@ $(EMFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)
