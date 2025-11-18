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
TEST_DIR = test
TEST_BIN_DIR = $(BIN_DIR)/test

# Libraries
LIBS = -lArchimedes -lDaedalus -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lcjson -lm

# Include paths
INCLUDES = -I/usr/include -I/usr/local/include -I$(INC_DIR)

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/scenes/*.c) $(wildcard $(SRC_DIR)/scenes/components/*.c) $(wildcard $(SRC_DIR)/scenes/sections/*.c) $(wildcard $(SRC_DIR)/tutorial/*.c) $(wildcard $(SRC_DIR)/terminal/*.c) $(wildcard $(SRC_DIR)/tween/*.c) $(wildcard $(SRC_DIR)/trinkets/*.c)
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

# ============================================================================
# TEST SUITE
# ============================================================================

# Test source files (excluding run_tests.c which is the main)
TEST_SOURCES = $(TEST_DIR)/test_structs.c $(TEST_DIR)/test_state.c $(TEST_DIR)/test_trinkets.c $(TEST_DIR)/test_targeting.c $(TEST_DIR)/test_doubled_integration.c $(TEST_DIR)/test_combat_start.c $(TEST_DIR)/test_stats.c $(TEST_DIR)/test_stubs.c
TEST_RUNNER = $(TEST_DIR)/run_tests.c

# Core source files needed by tests (excluding main.c)
CORE_SOURCES = $(SRC_DIR)/stateStorage.c \
               $(SRC_DIR)/cardTags.c \
               $(SRC_DIR)/statusEffects.c \
               $(SRC_DIR)/enemy.c \
               $(SRC_DIR)/player.c \
               $(SRC_DIR)/hand.c \
               $(SRC_DIR)/game.c \
               $(SRC_DIR)/state.c \
               $(SRC_DIR)/deck.c \
               $(SRC_DIR)/card.c \
               $(SRC_DIR)/trinket.c \
               $(SRC_DIR)/random.c \
               $(SRC_DIR)/stats.c \
               $(SRC_DIR)/tween/tween.c

# Test object files
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/test/%.o,$(TEST_SOURCES) $(TEST_RUNNER))
CORE_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CORE_SOURCES))

# Test executable
TEST_TARGET = $(TEST_BIN_DIR)/run_tests

# Compile test object files
$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling test: $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link test executable
$(TEST_TARGET): $(TEST_OBJECTS) $(CORE_OBJECTS)
	@mkdir -p $(TEST_BIN_DIR)
	@echo "Linking $(TEST_TARGET)..."
	@$(CC) $(TEST_OBJECTS) $(CORE_OBJECTS) -o $(TEST_TARGET) $(LIBS)
	@echo "Test build complete: $(TEST_TARGET)"

# Run tests
test: $(TEST_TARGET)
	@echo "Running tests..."
	@$(TEST_TARGET)

# ============================================================================
# ARCHITECTURE VERIFICATION
# ============================================================================

# Run all fitness functions
verify:
	@./verify_architecture.sh

# Alias for verify
fitness: verify

# Show help
help:
	@echo "Card Fifty-Two - Makefile targets:"
	@echo "  make          - Build the game (default)"
	@echo "  make run      - Build and run the game"
	@echo "  make test     - Build and run unit tests"
	@echo "  make verify   - Run architecture fitness functions"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make web      - Build for web (Emscripten)"
	@echo "  make serve    - Start local web server"
	@echo "  make valgrind - Run with memory leak detection"
	@echo "  make asan     - Build with Address Sanitizer"
	@echo "  make help     - Show this help message"

.PHONY: all debug clean run test verify fitness web serve valgrind asan help
