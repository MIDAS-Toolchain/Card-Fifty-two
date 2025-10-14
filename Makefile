# Card Fifty-Two - Makefile
# A card game framework using Archimedes and Daedalus

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
DEBUG_FLAGS = -g -O0 -DDEBUG

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
OBJ_DIR = build

# Libraries
LIBS = -lArchimedes -lDaedalus -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lcjson -lm

# Include paths
INCLUDES = -I$(INC_DIR) -I/usr/include -I/usr/local/include

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/scenes/*.c) $(wildcard $(SRC_DIR)/scenes/components/*.c) $(wildcard $(SRC_DIR)/scenes/sections/*.c) $(wildcard $(SRC_DIR)/tutorial/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BIN_DIR)/card-fifty-two

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "Build complete: $(TARGET)"

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Debug build
debug: CFLAGS = $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Run the game
run: $(TARGET)
	@echo "Running $(TARGET)..."
	@./$(TARGET)

# Emscripten web build (future)
EMCC = emcc
EM_FLAGS = -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2
EM_FLAGS += -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY

web: $(SOURCES)
	@mkdir -p $(BIN_DIR)
	@echo "Building for web with Emscripten..."
	@$(EMCC) $(SOURCES) $(EM_FLAGS) $(INCLUDES) \
		-L lib/ -lDaedalus -lArchimedes \
		-o $(BIN_DIR)/index.html
	@echo "Web build complete: $(BIN_DIR)/index.html"

# Serve web build
serve:
	@echo "Starting local server on http://localhost:8000"
	@python3 -m http.server 8000

# Memory leak check with Valgrind
valgrind: $(TARGET)
	@echo "Running Valgrind memory check..."
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Address sanitizer build
asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean $(TARGET)

# Show help
help:
	@echo "Card Fifty-Two - Makefile targets:"
	@echo "  make          - Build the game (default)"
	@echo "  make run      - Build and run the game"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make web      - Build for web (Emscripten)"
	@echo "  make serve    - Start local web server"
	@echo "  make valgrind - Run with memory leak detection"
	@echo "  make asan     - Build with Address Sanitizer"
	@echo "  make help     - Show this help message"

.PHONY: all debug clean run web serve valgrind asan help
