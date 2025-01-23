CXX = g++
CXXFLAGS = -Wall -O2 -I$(SRC_DIR)/Server/include

# Directories
SRC_DIR = .
BIN_DIR = $(SRC_DIR)/bin

# Server source files
SERVER_SRC = $(SRC_DIR)/Server/main.cpp \
             $(SRC_DIR)/Server/Informer/informer.cpp \
             $(SRC_DIR)/Server/Overseer/overseer.cpp \
             $(SRC_DIR)/Server/Server/server.cpp \
             $(SRC_DIR)/Server/Utils/utils.cpp

# Targets
TARGETS = informer overseer server

# Default target
all: $(patsubst %, $(BIN_DIR)/%, $(TARGETS))

# Build rules
$(BIN_DIR)/informer: $(SRC_DIR)/Informer/informer.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN_DIR)/overseer: $(SRC_DIR)/Overseer/overseer.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN_DIR)/server: $(SERVER_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Individual targets
informer: $(BIN_DIR)/informer
overseer: $(BIN_DIR)/overseer
server: $(BIN_DIR)/server

# Clean up
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean informer overseer server

