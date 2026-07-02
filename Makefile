CC      := gcc
CSTD    := -std=gnu11

SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin
INC_DIR := include

TARGET  := $(BIN_DIR)/main

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

CFLAGS  := $(CSTD) -Wall -Wextra -I$(INC_DIR) -MMD -MP
LDFLAGS :=
LDLIBS  :=

ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2
endif

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "Build complete: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

-include $(DEPS)

.PHONY: run
run: all
	./$(TARGET)

.PHONY: debug
debug:
	$(MAKE) DEBUG=1

.PHONY: gdb
gdb: debug
	gdb ./$(TARGET)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: fclean
fclean: clean
	rm -f *.log *.out

.PHONY: re
re: fclean all
