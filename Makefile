#################################################
# @file Makefile
# @author Charly Batista (carlbsb@gmail.com)
# @brief 
# @version 0.1
# @date 2023-04-25
# 
# Copyright © Percona LLC and/or its affiliates
#
# Hackathon Team #3
# 	- Abdul Sayeed
# 	- Agustin Gallego
# 	- Charly Batista
# 	- Jobin Augustine
# 	- Muhammad Usama
#
#################################################

# /// Executables
CC		:= gcc
MKDIR 	:= mkdir -p

# ///
TARGET_EXEC := pg_auto_tune

PROJ_DIR  := $(realpath $(CURDIR))
BUILD_DIR := build
BUILD_LIB := $(BUILD_DIR)/lib
SRC_DIRS  := src
INC_DIR   := include
LIB_DDIR  := lib

SRCS 	  := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS 	  := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS 	  := $(OBJS:.o=.d)

INC_DIRS  := $(shell find $(INC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I./$(LIB_DDIR)/iniparser/src

CFLAGS    := -fPIC -Wall -ggdb3 $(INC_FLAGS) -MMD -MP
LDFLAGS   := -shared
LIBS      := -L./$(BUILD_LIB)  -lm

$(BUILD_DIR)/$(TARGET_EXEC): mkdir $(OBJS)
	$(CC) $(OBJS) -o $@ $(LIBS)	

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Libraries
mkdir:
	$(MKDIR) $(BUILD_LIB)

# Clean
.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)
