CXX = g++
CXXFLAGS = -Wall -O2

# Directories
SRC_DIR = .
BIN_DIR = bin

# Targets
TARGETS = informer overseer server

# Default target
all: $(patsubst %, $(BIN_DIR)/%, $(TARGETS))

# Build rules
$(BIN_DIR)/informer: $(SRC_DIR)/Informer/informer.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN_DIR)/overseer: $(SRC_DIR)/Overseer/overseer.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN_DIR)/server: $(SRC_DIR)/Server/server.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Individual targets
informer: $(BIN_DIR)/informer
overseer: $(BIN_DIR)/overseer
server: $(BIN_DIR)/server

# Clean up
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean informer overseer server
