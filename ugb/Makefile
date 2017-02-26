# This file is part of uGB
# Copyright (C) 2017  Alexandre Monti
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

CC_FLAGS  = -std=c11 -Wall
debug: CC_FLAGS += -g -ggdb -O0
release: CC_FLAGS += -O3 -fomit-frame-pointer
LD_FLAGS = -lreadline -lpthread -lSDL2

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
