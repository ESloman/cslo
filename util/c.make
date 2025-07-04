# Makefile for building a single configuration of the C interpreter. It expects
# variables to be passed in for:
#
# MODE         "debug" or "release".
# NAME         Name of the output executable (and object file directory).
# SOURCE_DIR   Directory where source files and headers are found.

ifeq ($(CPP),true)
	# Ideally, we'd add -pedantic-errors, but the use of designated initializers
	# means cslo relies on some GCC/Clang extensions to compile as C++.
	CFLAGS := -std=c++11
	C_LANG := -x c++
else
	CFLAGS := -std=c99
endif

# CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -Wall -Wextra -Wno-unused-parameter

# include include directory for headers
CFLAGS += -Iinclude -Ithird_party

# If we're building at a point in the middle of a chapter, don't fail if there
# are functions that aren't used yet.
ifeq ($(SNIPPET),true)
	CFLAGS += -Wno-unused-function
endif

# Mode configuration.
ifeq ($(MODE),debug)
	CFLAGS += -O0 -DDEBUG=1 -g
	BUILD_DIR := build/debug
else
	CFLAGS += -O3 -flto
	BUILD_DIR := build/release
endif

# Recursive wildcard function
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

HEADERS := $(call rwildcard,include/,*.h)
SOURCES := $(call rwildcard,$(SOURCE_DIR)/,*.c)

# Files.
HEADERS := $(call rwildcard,$(SOURCE_DIR)/,*.h)
SOURCES := $(call rwildcard,$(SOURCE_DIR)/,*.c)
OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/$(NAME)/%.o,$(SOURCES))

# third party
LINENOISE_SRC := third_party/linenoise.c
LINENOISE_OBJ := $(BUILD_DIR)/$(NAME)/third_party/linenoise.o

SOURCES += $(LINENOISE_SRC)
OBJECTS += $(LINENOISE_OBJ)

# Targets ---------------------------------------------------------------------

# Link the interpreter.
build/$(NAME): $(OBJECTS)
	@ printf "%8s %-40s %s\n" $(CC) $@ "$(CFLAGS)"
	@ mkdir -p build
	@ $(CC) $(CFLAGS) $^ -o $@ -lm

# Compile object files.
$(BUILD_DIR)/$(NAME)/%.o: $(SOURCE_DIR)/%.c $(HEADERS)
	@ printf "%8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	mkdir -p $(dir $@)
	@ $(CC) -c $(C_LANG) $(CFLAGS) -o $@ $<

# Compile third_party object files.
$(BUILD_DIR)/$(NAME)/third_party/%.o: third_party/%.c $(HEADERS)
	@ printf "%8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	mkdir -p $(dir $@)
	@ $(CC) -c $(C_LANG) $(CFLAGS) -w -o $@ $<

.PHONY: default
