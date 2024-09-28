# Compiler
CC = gcc

# Compiler flags
CFLAGS = -std=c11 -Os -s -Wall -Iinclude

# Linker flags
LDFLAGS = -lopengl32 -lgdi32 -lwinmm -lglu32

# Directories
SRC_DIR = src
BUILD_DIR = build
SHADER_DIR = shaders

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Executable name (in root directory)
EXECUTABLE = demo.exe

# Default target
all: $(BUILD_DIR) $(EXECUTABLE)

# Rule to create build directory
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

# Rule to build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) $(EXECUTABLE)

# Phony targets
.PHONY: all clean