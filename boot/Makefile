B_SRCS := $(shell find . -name "*.c" -o -name "*.S")

B_OBJS := $(patsubst ./%, $(OBJ_DIR)/boot/%, $(B_SRCS))
B_OBJS := $(B_OBJS:.c=.o)
B_OBJS := $(B_OBJS:.S=.o)

B_CFLAGS := $(CFLAGS) -mcmodel=kernel -mno-red-zone \
             -I. \
             -I$(BASE_DIR) \
             -I$(BASE_DIR)/lib \
             -I$(BASE_DIR)/kernel/include \
             -I$(BASE_DIR)/libc/include

.PHONY: all clean

all: $(B_OBJS)

$(OBJ_DIR)/boot/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CC) $(B_CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/boot/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "[AS] $<"
	@$(CC) $(B_CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)/boot