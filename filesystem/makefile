CC := gcc
CFLAGS := -g -I"../shared/src" -Wall -w
LDFLAGS := -L"../shared/Debug"
LIBS := -lcommons -lshared

SRC_DIR := src
OBJ_DIR := Debug

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: filesystem

filesystem: create-Debug-dir $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

create-Debug-dir:
	mkdir -p $(OBJ_DIR)/$(SRC_DIR)

clean:
	rm -rf $(OBJ_DIR)
