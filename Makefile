# Directories
SRC_DIR = .
BIN_DIR = $(SRC_DIR)/bin
OVERSEER_DIR = $(SRC_DIR)/Overseer
BUILD_DIR = $(OVERSEER_DIR)/build

# Targets
TARGETS = informer overseer server

# Default target
all: $(BIN_DIR)/informer $(BIN_DIR)/server $(BIN_DIR)/overseer

# Build rules

# Build the informer target
$(BIN_DIR)/informer: $(SRC_DIR)/Informer/informer.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build the server target
$(BIN_DIR)/server: $(SRC_DIR)/Server/main.cpp \
                  $(SRC_DIR)/Server/Informer/informer.cpp \
                  $(SRC_DIR)/Server/Overseer/overseer.cpp \
                  $(SRC_DIR)/Server/Server/server.cpp \
                  $(SRC_DIR)/Server/Utils/utils.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build overseer target with CMake
$(BIN_DIR)/overseer: 
	@echo "Building Overseer with CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..
	@cd $(BUILD_DIR) && make
	@cp $(BUILD_DIR)/Overseer $(BIN_DIR)/

# Clean up
clean:
	rm -rf $(BIN_DIR) $(OVERSEER_DIR)/build

.PHONY: all clean informer overseer server

