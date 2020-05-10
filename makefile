# Compiler:
CC = gcc

# C flags:
CFLAGS = -g -Wall -pedantic -std=gnu11

# Directory with all the source files:
SRC = src
# Find all subdirectories inside the SRC directory. Remove ./ with subst.
VPATH = $(shell find $(SRC) -type d)

# Directory with where the compiled files go:
OBJ = build

# Find all .c files
SOURCES = $(subst ./,,$(shell find . -name "*.c"))
# Use $(notdir ...) to get only the filenames (ignore the directories)
# Delete file extensions with $(basename ...)
FILENAMES = $(basename $(notdir $(SOURCES)))
# All objects will be stored inside OBJ directory, without any sub-directories
# so, once we have the filenames we just simply add the .o suffix to them
# then, add the $(OBJ)/ prefix to put them inside the directory
OBJECTS = $(addprefix $(OBJ)/, $(addsuffix .o, $(FILENAMES)))

.PHONY: all
all: $(OBJECTS)
	$(CC) -o $(OBJ)/mastermind $(OBJECTS)

$(OBJECTS): | $(OBJ)/  # "Check" if the build directory exists

$(OBJ)/:  # Create a build (object) directory
	mkdir -p $@

$(OBJ)/%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: clean
clean:
	@echo "Deleting all compiled files..."
	rm -f $(OBJ)/*
	@echo "Done."
