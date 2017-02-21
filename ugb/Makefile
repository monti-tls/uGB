### Global configuration

PROJECT = uGB

### Directories and extensions configuration

SRC_DIR = src
INC_DIR = inc
TMP_DIR = obj
BIN_DIR = bin

SRC_EXT = c

### Tools

CC = gcc
LD = gcc
AR = ar
RM = rm

### Compilation flags

CC_FLAGS  = -std=c99 -Wall
debug: CC_FLAGS += -g -ggdb -O0
release: CC_FLAGS += -O3 -fomit-frame-pointer

### Files

PROGRAM  = $(BIN_DIR)/$(PROJECT)

PROG_SRC = $(shell find $(SRC_DIR)/ -name *.$(SRC_EXT))
PROG_OBJ = $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.o,$(PROG_SRC))

### Generated compilation flags

CC_FLAGS += -fPIC -I$(INC_DIR)

### Top-level targets

all: debug

release debug: $(PROGRAM)

.PHONY: clean
clean:
	@$(RM) -rf $(TMP_DIR) $(BIN_DIR)

### Dependencies

DEPS = $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.d,$(PROG_SRC))
-include $(DEPS)

### Final products

$(PROGRAM): $(PROG_OBJ)
	@mkdir -p $(@D)
	@$(LD) $^ $(LD_FLAGS) -o $@
	@echo "(LD) $@"

### Translation rules

$(TMP_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT)
	@mkdir -p $(@D)
	@$(CC) -MMD $(CC_FLAGS) -c $< -o $@
	@echo "(CC) $<"
